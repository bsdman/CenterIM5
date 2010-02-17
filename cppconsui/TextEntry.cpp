/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

/*
 * Copyright (C) 2007 by Mark Pustjens <pustjens@dds.nl>
 * Ported to c++ and modified for cppconsui/centerim.
 */

/** @file TextEntry.cpp TextEntry class implementation
 * @ingroup cppconsui
 */

#include "TextEntry.h"
#include "Keys.h"
#include "CppConsUIInternal.h"

#include <glib.h>
#include <cstring>

#define CONTEXT_TEXTENTRY "textentry"

TextEntry::TextEntry(Widget &parent, int x_, int y_, int w_, int h_, const gchar *text_)
: Label(parent, x_, y_, w_, h_, text_)
, current_pos(0)
, editable(true)
, flags(0)
, text_max_length(MAX_SIZE)
{
	RecalculateLengths();
	current_pos = text_length;

	can_focus = true;
	DeclareBindables();
}

TextEntry::TextEntry(Widget &parent, int x_, int y_, const gchar *text_)
: Label(parent, x_, y_, text_)
, current_pos(0)
, editable(true)
, flags(0)
, text_max_length(MAX_SIZE)
{
	RecalculateLengths();
	current_pos = text_length;

	can_focus = true;
	DeclareBindables();
}

TextEntry::~TextEntry()
{
}

void TextEntry::DeclareBindables()
{
	// cursor movement
	DeclareBindable(CONTEXT_TEXTENTRY, "cursor-right",
			sigc::bind(sigc::mem_fun(this, &TextEntry::ActionMoveCursor), MOVE_LOGICAL_POSITIONS, 1),
			InputProcessor::Bindable_Override);

	DeclareBindable(CONTEXT_TEXTENTRY, "cursor-left",
			sigc::bind(sigc::mem_fun(this, &TextEntry::ActionMoveCursor), MOVE_LOGICAL_POSITIONS, -1),
			InputProcessor::Bindable_Override);

	DeclareBindable(CONTEXT_TEXTENTRY, "cursor-right-word",
			sigc::bind(sigc::mem_fun(this, &TextEntry::ActionMoveCursor), MOVE_WORDS, 1),
			InputProcessor::Bindable_Override);

	DeclareBindable(CONTEXT_TEXTENTRY, "cursor-left-word",
			sigc::bind(sigc::mem_fun(this, &TextEntry::ActionMoveCursor), MOVE_WORDS, -1),
			InputProcessor::Bindable_Override);

	DeclareBindable(CONTEXT_TEXTENTRY, "cursor-end",
			sigc::bind(sigc::mem_fun(this, &TextEntry::ActionMoveCursor), MOVE_BUFFER_ENDS, 1),
			InputProcessor::Bindable_Override);

	DeclareBindable(CONTEXT_TEXTENTRY, "cursor-begin",
			sigc::bind(sigc::mem_fun(this, &TextEntry::ActionMoveCursor), MOVE_BUFFER_ENDS, -1),
			InputProcessor::Bindable_Override);

	// deleting text
	DeclareBindable(CONTEXT_TEXTENTRY, "delete-char",
			sigc::bind(sigc::mem_fun(this, &TextEntry::ActionDelete), DELETE_CHARS, 1),
			InputProcessor::Bindable_Override);

	DeclareBindable(CONTEXT_TEXTENTRY, "backspace",
			sigc::bind(sigc::mem_fun(this, &TextEntry::ActionDelete), DELETE_CHARS, -1),
			InputProcessor::Bindable_Override);
	/*
	DeclareBindable(CONTEXT_TEXTENTRY, "delete-word-end",
			sigc::bind(sigc::mem_fun(this, &TextEntry::ActionDelete), DELETE_WORD_ENDS, 1),
			InputProcessor::Bindable_Override);

	DeclareBindable(CONTEXT_TEXTENTRY, "delete-word-begin",
			sigc::bind(sigc::mem_fun(this, &TextEntry::ActionDelete), DELETE_WORD_ENDS, -1),
			InputProcessor::Bindable_Override);

	// overwrite
	DeclareBindable(CONTEXT_TEXTENTRY, "toggle-overwrite",
			sigc::mem_fun(this, &TextEntry::ActionToggleOverwrite),
			InputProcessor::Bindable_Override);
	*/

	// non text editing bindables
	DeclareBindable(CONTEXT_TEXTENTRY, "activate", sigc::mem_fun(this, &TextEntry::OnActivate),
			InputProcessor::Bindable_Override);
}

DEFINE_SIG_REGISTERKEYS(TextEntry, RegisterKeys);
bool TextEntry::RegisterKeys()
{
	RegisterKeyDef(CONTEXT_TEXTENTRY, "cursor-right", _("Move the cursor to the right."), KEYS->Key_right());
	RegisterKeyDef(CONTEXT_TEXTENTRY, "cursor-left", _("Move the cursor to the left."), KEYS->Key_left());
	RegisterKeyDef(CONTEXT_TEXTENTRY, "cursor-right-word", _("Move the cursor to the right by one word."), KEYS->Key_ctrl_right());
	RegisterKeyDef(CONTEXT_TEXTENTRY, "cursor-left-word", _("Move the cursor to the left by one word."), KEYS->Key_ctrl_left());
	RegisterKeyDef(CONTEXT_TEXTENTRY, "cursor-end", _("Move the cursor to the end of the text."), KEYS->Key_end());
	RegisterKeyDef(CONTEXT_TEXTENTRY, "cursor-begin", _("Move the cursor to the beginning of the text."), KEYS->Key_home());
	RegisterKeyDef(CONTEXT_TEXTENTRY, "delete-char",  _("Delete character under cursor."), KEYS->Key_del());
	RegisterKeyDef(CONTEXT_TEXTENTRY, "backspace", _("Delete character before cursor."), KEYS->Key_backspace());

	/// @todo enable
	/*
	RegisterKeyDef(CONTEXT_TEXTENTRY, "delete-word-end", _("Delete text until the end of the word at the cursor."), KEYS->Key_ctrl_del());
	RegisterKeyDef(CONTEXT_TEXTENTRY, "delete-word-begin", _("Delete text until the beginning of the word at the cursor."), KEYS->Key_ctrl_backspace());
	RegisterKeyDef(CONTEXT_TEXTENTRY, "toggle-overwrite", _("Enable/Disable overwrite mode."), KEYS->Key_ins());
	*/

	RegisterKeyDef(CONTEXT_TEXTENTRY, "activate", _("Accept input and move focus."), KEYS->Key_enter());
	return true;
}

void TextEntry::Draw()
{
	if (!area)
		return;

	Label::Draw();

	/// @todo we can do better than this
	/// @todo cursor blinking

	if (has_focus) {
		int realw = area->getmaxx();
		gchar *ptr = g_utf8_offset_to_pointer(text, current_pos);
		int i = width(text, ptr);
		int l = i / realw;
		i -= l * realw;
		colorscheme->SetColor(area, i, l, 1, ColorScheme::Focus);
	}
}

int TextEntry::ProcessInputText(const char *input, const int bytes)
{
	gunichar c;
	gchar *new_input;
	int new_bytes;

	/* We don't insert control characters in the text, we just skip the input
	 * in this case. Note that this does not handle control characters
	 * occurring anywhere else than in the first byte. This means that pasting
	 * text with embedded control characters will not work. */
	/// @todo also skip function keys
	c = g_utf8_get_char_validated(input, bytes);
	if (g_unichar_iscntrl(c))
		return 0;

	/// @todo filter out invalid chars

	// filter out unwanted input
	if (flags) {
		new_input = g_new0(gchar, bytes);
		gchar *next;
		const char *str = input;
		const char *end = str + bytes;
		new_bytes = 0;

		for (str = input; str < end; str = next) {
			c = g_utf8_get_char_validated(str, end - str);
			next = g_utf8_next_char(str);

			if (!(flags & FlagAlphabetic) && g_unichar_isalpha(c)) {
				// we dont want this character in the string, so skip it
			}
			else if (!(flags & FlagNumeric) && g_unichar_isdigit(c)) {
				// don't want it
			}
			else if (!(flags & FlagNoSpace) && g_unichar_isspace(c)) {
				// don't want it
			}
			else if (!(flags & FlagNoPunctuation) && g_unichar_ispunct(c)) {
				// don't want it
			}
			else {
				// we want this character, so copy it to the new input string

				// copy the single character
				g_utf8_strncpy(new_input + new_bytes, str, 1);
				// increase the number of bytes in the input
				new_bytes += next - str;
			}
		}
	}
	else {
		new_input = g_strndup(input, bytes);
		new_bytes = bytes;
	}

	if (editable)
		InsertTextAtCursor(new_input, new_bytes);

	// don't forget to free the new input string
	g_free(new_input);

	return bytes;
}

void TextEntry::SetFlags(int flags_)
{
	flags = flags_;

	/// @todo validate text using new flags?
}

void TextEntry::SetText(const gchar *text_)
{
	Label::SetText(text);

	RecalculateLengths();
	current_pos = text_length;
}

void TextEntry::MoveCursor(CursorMovement step, int direction)
{
	switch (step) {
		case MOVE_LOGICAL_POSITIONS:
			current_pos = MoveLogically(current_pos, direction);
			break;
		case MOVE_WORDS:
			if (direction > 0)
				current_pos = MoveForwardWord(current_pos);
			else if (direction < 0)
				current_pos = MoveBackwardWord(current_pos);
			break;
		case MOVE_BUFFER_ENDS:
			current_pos = direction < 0 ? 0 : text_length;
			break;
		default:
			g_assert_not_reached();
	}
}

void TextEntry::InsertTextAtCursor(const gchar *new_text, int new_text_bytes)
{
	int index;
	int n_chars;

	g_assert(new_text);
	g_assert(new_text_bytes >= 0);

	n_chars = g_utf8_strlen(new_text, new_text_bytes);

	/* Test if adding new text wouldn't overflow text_max_length and if so
	 * then add only characters that can still fit in. */
	if (n_chars + text_length > text_max_length) {
		/// @todo flash/blink display, or just beep? (a la screen)
		Curses::beep();

		n_chars = text_max_length - text_length;
		new_text_bytes = g_utf8_offset_to_pointer(new_text, n_chars) - new_text;
	}

	// extend text allocated memory if necessary
	if (new_text_bytes + text_bytes + 1 > text_size) {
		int prev_size = text_size;

		while (new_text_bytes + text_bytes + 1 > text_size) {
			if (text_size == 0)
				text_size = MIN_SIZE;
			else {
				if (2 * text_size < MAX_SIZE)
					text_size *= 2;
				else {
					text_size = MAX_SIZE;
					if (new_text_bytes > text_size - text_bytes - 1) {
						/// @todo flash/blink display, or just beep? (a la screen)
						Curses::beep();

						new_text_bytes = text_size - text_bytes - 1;
						new_text_bytes = g_utf8_find_prev_char(new_text, new_text + new_text_bytes + 1) - new_text;
						n_chars = g_utf8_strlen(new_text, new_text_bytes);
					}
					break;
				}
			}
		}

		// allocate a new buffer and copy old data into it
		gchar *et_new = (gchar *) g_malloc(text_size);
		memcpy(et_new, text, prev_size);

		// overwrite a memory that might contain sensitive information
		gchar *varea = text;
		while (prev_size-- > 0)
			*varea++ = 0;

		g_free(text);
		text = et_new;
	}

	index = g_utf8_offset_to_pointer(text, current_pos) - text;

	g_memmove(text + index + new_text_bytes, text + index, text_bytes - index);
	memcpy(text + index, new_text, new_text_bytes);

	text_bytes += new_text_bytes;
	text_length += n_chars;

	// NUL terminate for safety and convenience
	text[text_bytes] = '\0';

	current_pos = current_pos + n_chars;

	Redraw();

	signal_text_changed();
}

void TextEntry::DeleteText(int start_pos, int end_pos)
{
	g_assert(start_pos >= 0 && start_pos <= text_length);
	g_assert(end_pos >= 0 && end_pos <= text_length);

	if (start_pos < end_pos) {
		int start_index = g_utf8_offset_to_pointer(text, start_pos) - text;
		int end_index = g_utf8_offset_to_pointer(text, end_pos) - text;

		g_memmove(text + start_index, text + end_index, text_bytes + 1 - end_index);
		text_length -= end_pos - start_pos;
		text_bytes -= end_index - start_index;

		if (current_pos > start_pos)
			current_pos -= MIN(current_pos, end_pos) - start_pos;

		signal_text_changed();
	}
}

void TextEntry::RecalculateLengths()
{
	text_size = strlen(text);
	text_bytes = text_size;
	text_length = g_utf8_strlen(text, text_size);
}

void TextEntry::SetPosition(int position)
{
	if (position < 0)
		position = 0;
	else if (position > text_length)
		position = text_length;

	current_pos = position;
}

void TextEntry::DeleteFromCursor(DeleteType type, int direction)
{
	int start_pos = current_pos;
	int end_pos = current_pos;

	if (!editable)
		return;

	switch (type) {
		case DELETE_CHARS:
			end_pos = MoveLogically(current_pos, direction);
			DeleteText(MIN(start_pos, end_pos), MAX(start_pos, end_pos));
			break;
		case DELETE_WORD_ENDS:
			if (direction > 0)
				end_pos = MoveForwardWord(end_pos);
			else if (direction < 0)
				start_pos = MoveBackwardWord(start_pos);
			DeleteText(start_pos, end_pos);
			break;
		default:
			g_assert_not_reached();
	}
}

int TextEntry::MoveLogically(int start, int direction)
{
	if (direction > 0 && start < text_length)
		return start + 1;
	else if (direction < 0 && start > 0)
		return start - 1;
	return start;
}

int TextEntry::MoveForwardWord(int start)
{
	int new_pos = start;
	gchar *cur = g_utf8_offset_to_pointer(text, start);
	bool white = false;

	// search for the first nonwhite character after white characters
	while (new_pos < text_length) {
		if (g_unichar_type(g_utf8_get_char(cur)) == G_UNICODE_SPACE_SEPARATOR)
			white = true;
		else if (white)
			break;
		new_pos++;
		cur = g_utf8_next_char(cur);
	}

	return new_pos;
}

int TextEntry::MoveBackwardWord(int start)
{
	int new_pos = start;
	gchar *cur = g_utf8_offset_to_pointer(text, start);
	bool nonwhite = false;

	if (start <= 0)
		return 0;

	// always move at least one character back
	cur = g_utf8_prev_char(cur);
	new_pos--;

	// search for the first white character before nonwhite characters
	while (new_pos >= 0) {
		if (g_unichar_type(g_utf8_get_char(cur)) != G_UNICODE_SPACE_SEPARATOR)
			nonwhite = true;
		else if (nonwhite)
			break;

		if (new_pos > 0)
			cur = g_utf8_prev_char(cur);
		new_pos--;
	}

	return ++new_pos;
}

void TextEntry::ToggleOverwrite()
{
	overwrite_mode = !overwrite_mode;
}

void TextEntry::ActionMoveCursor(CursorMovement step, int direction)
{
	MoveCursor(step, direction);
	Redraw();
}

void TextEntry::ActionDelete(DeleteType type, int direction)
{
	DeleteFromCursor(type, direction);
	Redraw();
}

void TextEntry::ActionToggleOverwrite()
{
	ToggleOverwrite();
}

//TODO custom handlers?
void TextEntry::OnActivate()
{
	if (parent)
		parent->MoveFocus(FocusNext);
}
