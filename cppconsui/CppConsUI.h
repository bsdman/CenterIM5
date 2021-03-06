/*
 * Copyright (C) 2007 by Mark Pustjens <pustjens@dds.nl>
 * Copyright (C) 2010-2012 by CenterIM developers
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
 */

/**
 * @file
 * General classes, functions and enumerations.
 *
 * @ingroup cppconsui
 *
 * @todo CppConsUI namespace
 */

#ifndef __CPPCONSUI_H__
#define __CPPCONSUI_H__

namespace CppConsUI
{

enum WrapMode {
  WRAP_NONE,
  WRAP_CHAR,
  WRAP_WORD
};

class Point
{
public:
  Point() : x(0), y(0) {}
  Point(int x_, int y_) : x(x_), y(y_) {}

  int GetX() const { return x; }
  int GetY() const { return y; }

  int x, y;

protected:

private:
};

class Size
{
public:
  Size() : width(0), height(0) {}
  Size(int w, int h) : width(w), height(h) {}

  int GetWidth() const { return width; }
  int GetHeight() const { return height; }

  int width, height;

protected:

private:
};

class Rect: public Point
{
public:
  Rect() : width(0), height(0) {}
  Rect(int x, int y, int w, int h) : Point(x, y), width(w), height(h) {}

  int GetWidth() const { return width; }
  int GetHeight() const { return height; }
  int GetLeft() const { return x; }
  int GetTop() const { return y; }
  int GetRight() const { return x + width - 1; }
  int GetBottom() const { return y + height - 1; }

  int width, height;

protected:

private:
};

} // namespace CppConsUI

#endif // __CPPCONSUI_H__

/* vim: set tabstop=2 shiftwidth=2 textwidth=78 expandtab : */
