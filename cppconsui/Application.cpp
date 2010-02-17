/*
 * Copyright (C) 2009 by CenterIM developers
 *
 * This file is part of CenterIM.
 *
 * CenterIM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * CenterIM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * */

#include <cstring>
#include <libintl.h>
#include <cerrno>

#include "ConsuiCurses.h"
#include "Application.h"
#include "CppConsUIInternal.h"

Application::Application(void)
: windowmanager(NULL), converter((GIConv) -1), channel(NULL), channel_id(0)
{
	windowmanager = WindowManager::Instance();

	/* Application always needs to be the first resize handler because users
	 * usually want to recalculate sizes of all windows in ScreenResized
	 * (virtual method) first and then resize the windows appropriately. */
	resize = windowmanager->signal_resize.connect(sigc::mem_fun(this, &Application::ScreenResized));
	windowmanager->EnableResizing();

	// create a new loop
	gmainloop = Glib::MainLoop::create();
}

Application::~Application(void)
{
	resize.disconnect();

	windowmanager->Delete();
}

void Application::Run(void)
{
	windowmanager->ScreenResized();

	StdinInputInit();

	gmainloop->run();
}

void Application::Quit(void)
{
	gmainloop->quit();

	StdinInputUnInit();
}

gboolean Application::io_input_error(GIOChannel *source, GIOCondition cond)
{
	// log an error and bail out if we lost stdin
	g_critical("Stdin lost!\n");
	Quit();

	return TRUE;
}

gboolean Application::io_input(GIOChannel *source, GIOCondition cond)
{
	// TODO is this reasonable? (pasting needs a somewhat larger buffer to be efficient)
	gchar buf[64];
	gsize rd;
	GError *err = NULL;
	/* Buffer for saving a part of char from a previous reading, max char len
	 * in bytes (currently 6 bytes for UTF-8), size must be always
	 * <= sizeof(buf). */
	static gchar buf_part[6];
	static gsize buf_part_len;
	/* Every character in UTF-8 can be encoded with 6 bytes so this is enough
	 * room for any conversion. */
	gchar converted[6 * sizeof(buf) + 1];
	gsize converted_left = sizeof(converted);
	gchar *pbuf = buf;
	gchar *pconverted = converted;
	static std::string input;
	int eaten;

	if (buf_part_len) {
		memcpy(buf, buf_part, buf_part_len);
	}

	if (g_io_channel_read_chars(source, buf + buf_part_len,
				sizeof(buf) - buf_part_len, &rd, &err) != G_IO_STATUS_NORMAL) {
		if (err) {
			g_warning("Error reading chars from standard input (%s).\n", err->message);
			g_error_free(err);
		}
		else
			g_warning("Error reading chars from standard input.\n");
		return TRUE;
	}
	rd += buf_part_len;
	buf_part_len = 0;

	/* We don't need to care much about this, glib will notice us again that
	 * there are still bytes left. */
	if (sizeof(buf) == rd) {
		g_debug("Input buffer full.\n");
	}

	/* TODO Fix the input string.
	 * Some keys generate bytestrings which are different from the strings
	 * terminfo/ncurses expects
	 * */
	//keys->Refine(buf, rd);

	// convert data from user charset to UTF-8
	g_iconv(converter, NULL, NULL, NULL, NULL);
	errno = 0;
	if (g_iconv(converter, &pbuf, &rd, &pconverted, &converted_left) == (gsize) -1) {
		switch (errno) {
			case EILSEQ:
				g_critical(_("IConv error: %s\n"), g_strerror(errno));
				return TRUE;
			case EINVAL:
				/* Incomplete multibyte sequence is encountered in the input,
				 * save these bytes for further reading. */
				memcpy(buf_part, pbuf, rd);
				buf_part_len = rd;
				break;
			default:
				g_critical(_("Unexcepted IConv error: %s\n"), g_strerror(errno));
				return TRUE;
		}
	}
	*pconverted = '\0';

	/* Below this line we assume all input has been converted to UTF-8 encoded
	 * multibyte string
	 * */

	// too noisy even for debug level
	/*
	for (gchar *iter = converted; *iter != '\0'; iter = g_utf8_next_char(iter)) {
		g_debug("input: U+%04"G_GINT32_FORMAT"X\n", g_utf8_get_char(iter));
	}
	*/

	input.append(converted);

	while (input.size()) {
		eaten = ProcessInput(input.c_str(), input.size());
		if (eaten < 0) {
			return TRUE;
		} else if (eaten == 0) {
			//TODO find out if there is a more intelligent way
			//to discard input. eg: key_left is 6 bytes, so
			//remove 6 bytes (*must* be *fast*)
			eaten = 1;
		}
		input.erase(0, eaten);
	}

	return TRUE;
}

void Application::StdinInputInit(void)
{
	SetInputChild(windowmanager);

	// print the currently used character set
	const char *charset;
	g_get_charset(&charset);
	if ((converter = g_iconv_open("UTF-8", charset)) == (GIConv) -1) {
		g_critical(_("IConv initialization failed (%s)\n"), g_strerror(errno));
		// TODO !
	}

	channel = g_io_channel_unix_new(STDIN_FILENO);
	// set channel encoding to NULL so it can be unbuffered
	g_io_channel_set_encoding(channel, NULL, NULL);
	g_io_channel_set_buffered(channel, FALSE);
	g_io_channel_set_close_on_unref(channel, TRUE);

	channel_id = g_io_add_watch_full(channel, G_PRIORITY_HIGH,
			(GIOCondition)(G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_PRI), io_input_, this, NULL);

	g_io_add_watch_full(channel, G_PRIORITY_HIGH, (G_IO_NVAL), io_input_error_, this, NULL);

	g_io_channel_unref(channel);
}

void Application::StdinInputUnInit(void)
{
	g_source_remove(channel_id);
	channel_id = 0;
	g_io_channel_unref(channel);
	channel = NULL;

	g_iconv_close(converter);
}
