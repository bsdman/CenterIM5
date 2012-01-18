/*
 * Copyright (C) 2011 by CenterIM developers
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
 * SubMenu class implementation.
 *
 * @ingroup cppconsui
 */

#include "SubMenu.h"

namespace CppConsUI
{

SubMenu::SubMenu(int w, int h, const char *text)
: Button(w, h, text)
{
  submenu = new ExtMenuWindow(*this);
  signal_activate.connect(sigc::mem_fun(this, &SubMenu::OnActivate));
}

SubMenu::SubMenu(const char *text)
: Button(text)
{
  submenu = new ExtMenuWindow(*this);
  signal_activate.connect(sigc::mem_fun(this, &SubMenu::OnActivate));
}

SubMenu::~SubMenu()
{
  submenu->Close();
  delete submenu;
}

SubMenu::ExtMenuWindow::ExtMenuWindow(SubMenu& ref)
: FlowMenuWindow(ref, AUTOSIZE, AUTOSIZE)
{
  SetLeftShift(3);
}

void SubMenu::ExtMenuWindow::Close()
{
  signal_close(*this);
  Hide();
}

void SubMenu::OnActivate(Button& activator)
{
  if (!parent_window_close_conn.connected()) {
    FreeWindow *win = dynamic_cast<FreeWindow*>(GetTopContainer());
    g_assert(win);
    parent_window_close_conn = win->signal_close.connect(sigc::mem_fun(this,
          &SubMenu::OnParentWindowClose));
  }

  // make sure that the first widget in the focus chain is always focused
  submenu->CleanFocus();
  submenu->MoveFocus(Container::FOCUS_DOWN);

  submenu->Show();
}

void SubMenu::OnParentWindowClose(FreeWindow& activator)
{
  Close();
}

} // namespace CppConsUI