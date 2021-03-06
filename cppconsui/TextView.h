/*
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
 * TextView class.
 *
 * @ingroup cppconsui
 */

#ifndef __TEXTVIEW_H__
#define __TEXTVIEW_H__

#include "Widget.h"

#include <deque>

namespace CppConsUI
{

/**
 * Line oriented text view widget.
 */
class TextView
: public Widget
{
public:
  TextView(int w, int h, bool autoscroll_ = false, bool scrollbar_ = false);
  virtual ~TextView();

  // Widget
  virtual void Draw();

  /**
   * Appends text after the last line.
   */
  virtual void Append(const char *text, int color = 0);
  /**
   * Inserts text before specified line number. Text can contain multiple
   * lines and should end with '\\n' character just in front of '\\0'
   * character.
   */
  virtual void Insert(size_t line_num, const char *text, int color = 0);
  /**
   * Removes a specified line.
   */
  virtual void Erase(size_t line_num);
  /**
   * Removes specified range of lines. Parameter end_line represents the
   * line after the last removed line, thus range of <start_line, end_line)
   * lines is removed.
   */
  virtual void Erase(size_t start_line, size_t end_line);
  /**
   * Removes all lines.
   */
  virtual void Clear();
  /**
   * Returns string for a specified line number.
   */
  virtual const char *GetLine(size_t line_num) const;
  /**
   * Returns count of all lines.
   */
  virtual size_t GetLinesNumber() const;

  virtual void SetAutoScroll(bool enabled);
  virtual bool GetAutoScroll() const { return autoscroll; }

  virtual void SetScrollBar(bool enabled);
  virtual bool GetScrollBar() const { return scrollbar; }

protected:
  /**
   * Struct Line saves a real line. All text added into TextView is split on
   * '\\n' character and stored into Line objects.
   */
  struct Line
  {
    /**
     * UTF-8 encoded text. Note: Newline character is not part of text.
     */
    char *text;
    /**
     * Text length in characters.
     */
    size_t length;
    /**
     * Color number.
     */
    int color;

    Line(const char *text_, size_t bytes, int color_);
    virtual ~Line();
  };

  /**
   * ScreenLine represents an on-screen line.
   */
  struct ScreenLine
  {
    /**
     * Parent Line that this ScreenLine shows.
     */
    Line *parent;
    /**
     * Pointer into parent's text where this ScreenLine starts.
     */
    const char *text;
    /**
     * Length in characters (Unicode).
     */
    int length;

    ScreenLine(Line &parent_, const char *text_, int length_);
  };

  typedef std::deque<Line*> Lines;
  typedef std::deque<ScreenLine> ScreenLines;

  size_t view_top;
  bool autoscroll;
  bool autoscroll_suspended;
  bool scrollbar;

  /**
   * Array of real lines.
   */
  Lines lines;
  /**
   * Array of on-screen lines.
   */
  ScreenLines screen_lines;

  virtual const char *ProceedLine(const char *text, int area_width,
      int *res_length) const;
  /**
   * Recalculates on-screen lines for a specified line number.
   */
  virtual size_t UpdateScreenLines(size_t line_num, size_t start = 0);
  /**
   * Recalculates all screen lines.
   */
  virtual void UpdateAllScreenLines();

  virtual size_t EraseScreenLines(size_t line_num, size_t start = 0,
      size_t *deleted = NULL);

private:
  TextView(const TextView &);
  TextView& operator=(const TextView&);

  void ActionScroll(int direction);

  void DeclareBindables();
};

} // namespace CppConsUI

#endif // __TEXTVIEW_H__

/* vim: set tabstop=2 shiftwidth=2 textwidth=78 expandtab : */
