/*
 * Copyright (C) 2007 by Mark Pustjens <pustjens@dds.nl>
 * Copyright (C) 2010-2011 by CenterIM developers
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

/**
 * @file
 * MenuWindow class.
 *
 * @ingroup cppconsui
 */

#ifndef __MENUWINDOW_H__
#define __MENUWINDOW_H__

#include "ListBox.h"
#include "Window.h"

#define MENU_WINDOW_WISH_WIDTH 40

namespace CppConsUI
{

class MenuWindow
: public Window
{
public:
  enum Flag {
    FLAG_HIDE_ON_CLOSE = 1 << 0, /* MenuWindow will hide instead of close. */
  };

  MenuWindow(int x, int y, int w, int h, const char *title = NULL);
  MenuWindow(Widget& ref_, int w, int h, const char *title = NULL);
  virtual ~MenuWindow() {}

  virtual Button *InsertItem(size_t pos, const char *title,
      const sigc::slot<void, Button&>& callback)
    { return listbox->InsertItem(pos, title, callback); }
  virtual Button *AppendItem(const char *title,
      const sigc::slot<void, Button&>& callback)
    { return listbox->AppendItem(title, callback); }
  virtual AbstractLine *InsertSeparator(size_t pos)
    { return listbox->InsertSeparator(pos); }
  virtual AbstractLine *AppendSeparator()
    { return listbox->AppendSeparator(); }
  virtual Button *AppendSubMenu(const char *title,
      MenuWindow& submenu);
  virtual void InsertWidget(size_t pos, Widget& widget)
    { listbox->InsertWidget(pos, widget); }
  virtual void AppendWidget(Widget& widget)
    { listbox->AppendWidget(widget); }

  virtual void Draw();

  virtual void Close();

  virtual void SetFlags(int new_flags);
  virtual int GetFlags() const { return flags; }

  virtual void SetRef(Widget *ref_);

  virtual int GetLeftShift() const { return xshift; }
  virtual int GetTopShift() const { return yshift; }

  virtual void SetLeftShift(int x);
  virtual void SetTopShift(int y);


protected:
  ListBox *listbox;
  int wish_height;

  Widget *ref;
  int xshift, yshift;
  sigc::connection ref_visible_conn;

  int flags;

  // Container
  virtual void AddWidget(Widget& widget, int x, int y);

  // FreeWindow
  virtual void OnScreenResizedInternal();

  /**
   * Recalculates desired on-screen position and size of this window. This
   * mainly handles autosize magic.
   */
  virtual void UpdateSmartPositionAndSize();

  virtual void OnChildrenHeightChange(ListBox& activator, int new_height);

  virtual void OnRefVisible(Widget& activator, bool visible);

private:
  MenuWindow(const MenuWindow&);
  MenuWindow& operator=(const MenuWindow&);
};

} // namespace CppConsUI

#endif // __MENUWINDOW_H__

/* vim: set tabstop=2 shiftwidth=2 expandtab : */
