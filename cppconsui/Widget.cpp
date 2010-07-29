/*
 * Copyright (C) 2007 by Mark Pustjens <pustjens@dds.nl>
 * Copyright (C) 2010 by CenterIM developers
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
 * Widget class implementation.
 *
 * @ingroup cppconsui
 */

#include "Widget.h"

#include "Container.h"

#include <string>

Widget::Widget(int w, int h)
: xpos(0), ypos(0), width(w), height(h), can_focus(false), has_focus(false)
, visible(true), area(NULL), parent(NULL), color_scheme(NULL)
{
}

Widget::~Widget()
{
	SetVisibility(false);

	if (area)
		delete area;
	if (color_scheme)
		g_free(color_scheme);
}

void Widget::MoveResize(int newx, int newy, int neww, int newh)
{
	Rect oldsize(xpos, ypos, width, height), newsize(newx, newy, neww, newh);

	xpos = newx;
	ypos = newy;
	width = neww;
	height = newh;

	if (parent)
		UpdateArea();

	signal_moveresize(*this, oldsize, newsize);
}

void Widget::UpdateArea()
{
	g_assert(parent);

	if (area)
		delete area;
	area = parent->GetSubPad(*this, xpos, ypos, width, height);
	signal_redraw(*this);
}

Widget *Widget::GetFocusWidget()
{
	if (can_focus)
		return this;
	return NULL;
}

void Widget::CleanFocus()
{
	if (!has_focus)
		return;

	has_focus = false;
	signal_focus(*this, false);
	signal_redraw(*this);
}

void Widget::RestoreFocus()
{
	GrabFocus();
}

void Widget::UngrabFocus()
{
	if (!parent || !has_focus)
		return;

	has_focus = false;
	signal_focus(*this, false);
	signal_redraw(*this);
}

bool Widget::GrabFocus()
{
	if (!parent || has_focus)
		return false;

	if (can_focus && visible && parent->SetFocusChild(*this)) {
		has_focus = true;
		signal_focus(*this, true);
		signal_redraw(*this);
		return true;
	}

	return false;
}

void Widget::SetVisibility(bool visible)
{
	if (this->visible != visible) {
		this->visible = visible;

		Container *t = GetTopContainer();
		if (t) {
			if (visible) {
				if (!t->GetFocusWidget()) {
					// there is no focused widget, try to grab it
					GrabFocus();
				}
			}
			else {
				Widget *focus = t->GetFocusWidget();
				if (focus && !focus->IsVisibleRecursive()) {
					// focused widget was hidden, move the focus
					t->MoveFocus(Container::FocusNext);
				}
			}
		}

		signal_visible(*this, visible);
		signal_redraw(*this);
	}
}

bool Widget::IsVisibleRecursive() const
{
	if (!parent || !visible)
		return false;

	return parent->IsWidgetVisible(*this);
}

void Widget::SetParent(Container& parent)
{
	// we don't support parent change
	g_assert(!this->parent);

	this->parent = &parent;

	if (!GetTopContainer()->GetFocusWidget()) {
		// there is no focused widget, try to grab it
		GrabFocus();
	}

	UpdateArea();
}

void Widget::SetColorScheme(const char *scheme)
{
	if (color_scheme)
		g_free(color_scheme);

	if (scheme)
		color_scheme = g_strdup(scheme);
	else
		color_scheme = NULL;
}

const char *Widget::GetColorScheme()
{
	if (color_scheme)
		return color_scheme;
	else if (parent)
		return parent->GetColorScheme();

	return NULL;
}

Container *Widget::GetTopContainer()
{
	Container *c, *top;

	top = NULL;
	c = parent;
	while (c) {
		top = c;
		c = c->GetParent();
	}
	return top;
}
