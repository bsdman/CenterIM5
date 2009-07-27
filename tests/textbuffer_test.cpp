/*
 * Copyright (C) 2008 by Mark Pustjens <pustjens@dds.nl>
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

#include <cppconsui/TextBuffer.cpp>

#include <iostream>
#include <string.h>

/*
#define STR11 "I do not feel obliged to believe that the\n"
#define STR12 "same God who has endowed us with sense, reason,\n"
#define STR13 "and intellect has intended us to forgo their\n"
#define STR14 "use.\n"
*/
#define STR11 "1.\n"
#define STR12 "2.\n"
#define STR13 "3.\n"
#define STR14 "4.\n"

#define STR2 "Just a test."
#define STR3 "Hello world, Καλημέρα κόσμε, コンニチハ"
#define STR4 "Ｊｕｓｔ ａ ｔｅｓｔ."

int main(int argc, char **argv)
{
	TextBuffer* buffer = NULL;
	TextBuffer::char_iterator iter, end;
	char c;

	/* Setup locale. */
	setlocale(LC_ALL, "");

	buffer = new TextBuffer();
	buffer->append(STR12, strlen(STR12));
	buffer->append(STR13, strlen(STR13));
	buffer->append(STR14, strlen(STR14));
	buffer->prefix(STR11, strlen(STR11));

	std::cout << "== Printing a 4 line buffer, no linebreaks ==" << std::endl;
	end = buffer->end();
	for (iter = buffer->begin(); iter != end; iter++) {
		c = *(*iter);
		printf("%c", c);
	}
	std::cout << "=============================================" << std::endl;

	delete buffer;
}
