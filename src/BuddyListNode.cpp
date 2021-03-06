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

#include "BuddyListNode.h"

#include "BuddyList.h"
#include "Conversations.h"
#include "Utils.h"

#include <cppconsui/ConsuiCurses.h>
#include <cppconsui/ColorScheme.h>
#include <cppconsui/Keys.h>
#include "gettext.h"

BuddyListNode *BuddyListNode::CreateNode(PurpleBlistNode *node)
{
  if (PURPLE_BLIST_NODE_IS_BUDDY(node))
    return new BuddyListBuddy(node);
  if (PURPLE_BLIST_NODE_IS_CHAT(node))
    return new BuddyListChat(node);
  if (PURPLE_BLIST_NODE_IS_CONTACT(node))
    return new BuddyListContact(node);
  if (PURPLE_BLIST_NODE_IS_GROUP(node))
    return new BuddyListGroup(node);

  LOG->Warning(_("Unrecognized BuddyList node."));
  return NULL;
}

void BuddyListNode::SetParent(CppConsUI::Container& parent)
{
  Button::SetParent(parent);

  treeview = dynamic_cast<CppConsUI::TreeView*>(&parent);
  g_assert(treeview);
}

void BuddyListNode::Update()
{
  BuddyListNode *parent_node = GetParentNode();
  // the parent could have changed, so re-parent the node
  if (parent_node)
    treeview->SetNodeParent(ref, parent_node->GetRefNode());
}

void BuddyListNode::SortIn()
{
  CppConsUI::TreeView::NodeReference parent_ref;
  BuddyListNode *parent_node = GetParentNode();
  if (parent_node)
    parent_ref = parent_node->GetRefNode();
  else
    parent_ref = treeview->GetRootNode();

  /* Do the insertion sort. It should be fast enough here because nodes are
   * usually already sorted and only one node is in a wrong position, so it
   * kind of runs in O(n). */
  CppConsUI::TreeView::SiblingIterator i = --parent_ref.end();
  while (true) {
    // sref is a node that we want to sort in
    CppConsUI::TreeView::SiblingIterator sref = i;

    // calculate a stop condition
    bool stop_flag;
    if (i != parent_ref.begin()) {
      stop_flag = false;
      i--;
    }
    else
      stop_flag = true;

    BuddyListNode *swidget = dynamic_cast<BuddyListNode*>(sref->GetWidget());
    g_assert(swidget);
    CppConsUI::TreeView::SiblingIterator j = sref;
    j++;
    while (j != parent_ref.end()) {
      BuddyListNode *n = dynamic_cast<BuddyListNode*>(j->GetWidget());
      g_assert(n);

      if (swidget->LessOrEqual(*n)) {
        treeview->MoveNodeBefore(sref, j);
        break;
      }
      j++;
    }
    // the node is last in the list
    if (j == parent_ref.end())
      treeview->MoveNodeAfter(sref, --j);

    if (stop_flag)
      break;
  }
}

BuddyListNode *BuddyListNode::GetParentNode() const
{
  PurpleBlistNode *parent = node->parent;

  if (!parent)
    return NULL;

  return reinterpret_cast<BuddyListNode*>(
      purple_blist_node_get_ui_data(parent));
}

BuddyListNode::ContextMenu::ContextMenu(BuddyListNode& parent_node_)
: MenuWindow(parent_node_, AUTOSIZE, AUTOSIZE), parent_node(&parent_node_)
{
}

void BuddyListNode::ContextMenu::OnMenuAction(Button& /*activator*/,
    PurpleCallback callback, void *data)
{
  g_assert(callback);

  typedef void (*TypedCallback)(void *, void *);
  TypedCallback real_callback = reinterpret_cast<TypedCallback>(callback);
  real_callback(parent_node->GetPurpleBlistNode(), data);

  Close();
}

void BuddyListNode::ContextMenu::AppendMenuAction(MenuWindow& menu,
    PurpleMenuAction *act)
{
  if (!act) {
    menu.AppendSeparator();
    return;
  }

  if (!act->children) {
    if (act->callback)
      menu.AppendItem(act->label, sigc::bind(sigc::mem_fun(this,
            &BuddyListNode::ContextMenu::OnMenuAction), act->callback,
            act->data));
    else {
      // TODO display non-focusable widget?
    }
  }
  else {
    MenuWindow *submenu = new MenuWindow(0, 0, AUTOSIZE, AUTOSIZE);
    menu.AppendSubMenu(act->label, *submenu);

    for (GList *l = act->children; l; l = l->next) {
      PurpleMenuAction *act = reinterpret_cast<PurpleMenuAction*>(l->data);
      AppendMenuAction(*submenu, act);
    }

    // free memory associated with the children
    g_list_free(act->children);
    act->children = NULL;
  }

  // free the menu action
  purple_menu_action_free(act);
}

void BuddyListNode::ContextMenu::AppendProtocolMenu(PurpleConnection *gc)
{
  PurplePluginProtocolInfo *prpl_info
    = PURPLE_PLUGIN_PROTOCOL_INFO(purple_connection_get_prpl(gc));
  if (!prpl_info || !prpl_info->blist_node_menu)
    return;

  GList *ll = prpl_info->blist_node_menu(parent_node->GetPurpleBlistNode());
  for (GList *l = ll; l; l = l->next) {
    PurpleMenuAction *act = reinterpret_cast<PurpleMenuAction*>(l->data);
    AppendMenuAction(*this, act);
  }
  g_list_free(ll);

  if (ll) {
    // append a separator because there has been some items
    AppendSeparator();
  }
}

void BuddyListNode::ContextMenu::AppendExtendedMenu()
{
  GList *ll = purple_blist_node_get_extended_menu(
      parent_node->GetPurpleBlistNode());
  for (GList *l = ll; l; l = l->next) {
    PurpleMenuAction *act = reinterpret_cast<PurpleMenuAction*>(l->data);
    AppendMenuAction(*this, act);
  }
  g_list_free(ll);

  if (ll) {
    // append a separator because there has been some items
    AppendSeparator();
  }
}

BuddyListNode::BuddyListNode(PurpleBlistNode *node_)
: treeview(NULL), node(node_)
{
  purple_blist_node_set_ui_data(node, this);
  signal_activate.connect(sigc::mem_fun(this, &BuddyListNode::OnActivate));
  DeclareBindables();
}

BuddyListNode::~BuddyListNode()
{
  purple_blist_node_set_ui_data(node, NULL);
}

bool BuddyListNode::LessOrEqualByType(const BuddyListNode& other) const
{
  // group < contact < buddy < chat < other
  PurpleBlistNodeType t1 = purple_blist_node_get_type(node);
  PurpleBlistNodeType t2 = purple_blist_node_get_type(other.node);
  return t1 <= t2;
}

bool BuddyListNode::LessOrEqualByBuddySort(PurpleBuddy *left,
    PurpleBuddy *right) const
{
  BuddyList::BuddySortMode mode = BUDDYLIST->GetBuddySortMode();
  int a, b;

  switch (mode) {
    case BuddyList::BUDDY_SORT_BY_NAME:
      break;
    case BuddyList::BUDDY_SORT_BY_STATUS:
      a = GetBuddyStatusWeight(left);
      b = GetBuddyStatusWeight(right);
      if (a != b)
        return a > b;
      break;
    case BuddyList::BUDDY_SORT_BY_ACTIVITY:
      a = purple_blist_node_get_int(PURPLE_BLIST_NODE(left), "last_activity");
      b = purple_blist_node_get_int(PURPLE_BLIST_NODE(right), "last_activity");
      if (a != b)
        return a > b;
      break;
  }
  return g_utf8_collate(purple_buddy_get_alias(left),
      purple_buddy_get_alias(right)) <= 0;
}

const char *BuddyListNode::GetBuddyStatus(PurpleBuddy *buddy) const
{
  if (!purple_account_is_connected(purple_buddy_get_account(buddy)))
    return "";

  PurplePresence *presence = purple_buddy_get_presence(buddy);
  PurpleStatus *status = purple_presence_get_active_status(presence);
  return Utils::GetStatusIndicator(status);
}

int BuddyListNode::GetBuddyStatusWeight(PurpleBuddy *buddy) const
{
  if (!purple_account_is_connected(purple_buddy_get_account(buddy)))
    return 0;

  PurplePresence *presence = purple_buddy_get_presence(buddy);
  PurpleStatus *status = purple_presence_get_active_status(presence);
  PurpleStatusType *status_type = purple_status_get_type(status);
  PurpleStatusPrimitive prim = purple_status_type_get_primitive(status_type);

  switch (prim) {
    case PURPLE_STATUS_OFFLINE:
      return 0;
    default:
      return 1;
    case PURPLE_STATUS_UNSET:
      return 2;
    case PURPLE_STATUS_UNAVAILABLE:
      return 3;
    case PURPLE_STATUS_AWAY:
      return 4;
    case PURPLE_STATUS_EXTENDED_AWAY:
      return 5;
    case PURPLE_STATUS_MOBILE:
      return 6;
#if PURPLE_VERSION_CHECK(2, 7, 0)
    case PURPLE_STATUS_MOOD:
      return 7;
#endif
    case PURPLE_STATUS_TUNE:
      return 8;
    case PURPLE_STATUS_INVISIBLE:
      return 9;
    case PURPLE_STATUS_AVAILABLE:
      return 10;
  }
}

void BuddyListNode::UpdateFilterVisibility(const char *name)
{
  if (!IsVisible())
    return;

  const char *filter = BUDDYLIST->GetFilterString();
  if (!filter[0])
    return;

  // filtering is active
  SetVisibility(purple_strcasestr(name, filter));
}

void BuddyListNode::ActionOpenContextMenu()
{
  OpenContextMenu();
}

void BuddyListNode::DeclareBindables()
{
  DeclareBindable("buddylist", "contextmenu", sigc::mem_fun(this,
        &BuddyListNode::ActionOpenContextMenu),
      InputProcessor::BINDABLE_NORMAL);
}

bool BuddyListBuddy::LessOrEqual(const BuddyListNode& other) const
{
  const BuddyListBuddy *o = dynamic_cast<const BuddyListBuddy*>(&other);
  if (o)
    return LessOrEqualByBuddySort(buddy, o->buddy);
  return LessOrEqualByType(other);
}

void BuddyListBuddy::Update()
{
  BuddyListNode::Update();

  const char *status = GetBuddyStatus(buddy);
  const char *alias = purple_buddy_get_alias(buddy);
  if (status[0]) {
    char *text = g_strdup_printf("%s %s", status, alias);
    SetText(text);
    g_free(text);
  }
  else
    SetText(alias);

  SortIn();

  UpdateColorScheme();

  if (!purple_account_is_connected(purple_buddy_get_account(buddy))) {
    // hide if account is offline
    SetVisibility(false);
  }
  else
    SetVisibility(BUDDYLIST->GetShowOfflineBuddiesPref() || status[0]);

  UpdateFilterVisibility(alias);
}

void BuddyListBuddy::OnActivate(Button& /*activator*/)
{
  PurpleAccount *account = purple_buddy_get_account(buddy);
  const char *name = purple_buddy_get_name(buddy);
  PurpleConversation *conv = purple_find_conversation_with_account(
      PURPLE_CONV_TYPE_IM, name, account);

  if (!conv)
    conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, account, name);
  purple_conversation_present(conv);
}

const char *BuddyListBuddy::ToString() const
{
  return purple_buddy_get_alias(buddy);
}

BuddyListBuddy::BuddyContextMenu::BuddyContextMenu(
    BuddyListBuddy& parent_buddy_)
: ContextMenu(parent_buddy_), parent_buddy(&parent_buddy_)
{
  AppendProtocolMenu(purple_account_get_connection(
          purple_buddy_get_account(parent_buddy->GetPurpleBuddy())));
  AppendExtendedMenu();

  AppendItem(_("Alias..."), sigc::mem_fun(this,
        &BuddyListBuddy::BuddyContextMenu::OnChangeAlias));
  AppendItem(_("Delete..."), sigc::mem_fun(this,
        &BuddyListBuddy::BuddyContextMenu::OnRemove));
}

void BuddyListBuddy::BuddyContextMenu::ChangeAliasResponseHandler(
    CppConsUI::InputDialog& activator,
    CppConsUI::AbstractDialog::ResponseType response)
{
  switch (response) {
    case CppConsUI::AbstractDialog::RESPONSE_OK:
      break;
    default:
      return;
  }

  PurpleBuddy *buddy = parent_buddy->GetPurpleBuddy();
  purple_blist_alias_buddy(buddy, activator.GetText());
  serv_alias_buddy(buddy);

  // close context menu
  Close();
}

void BuddyListBuddy::BuddyContextMenu::OnChangeAlias(Button& /*activator*/)
{
  PurpleBuddy *buddy = parent_buddy->GetPurpleBuddy();
  CppConsUI::InputDialog *dialog = new CppConsUI::InputDialog(
      _("Alias"), purple_buddy_get_alias(buddy));
  dialog->signal_response.connect(sigc::mem_fun(this,
        &BuddyListBuddy::BuddyContextMenu::ChangeAliasResponseHandler));
  dialog->Show();
}

void BuddyListBuddy::BuddyContextMenu::RemoveResponseHandler(
    CppConsUI::MessageDialog& /*activator*/,
    CppConsUI::AbstractDialog::ResponseType response)
{
  switch (response) {
    case CppConsUI::AbstractDialog::RESPONSE_OK:
      break;
    default:
      return;
  }

  PurpleBuddy *buddy = parent_buddy->GetPurpleBuddy();
  purple_account_remove_buddy(purple_buddy_get_account(buddy), buddy,
      purple_buddy_get_group(buddy));

  /* Close the context menu before the buddy is deleted because its deletion
   * can lead to destruction of this object. */
  Close();

  purple_blist_remove_buddy(buddy);
}

void BuddyListBuddy::BuddyContextMenu::OnRemove(Button& /*activator*/)
{
  PurpleBuddy *buddy = parent_buddy->GetPurpleBuddy();
  char *msg = g_strdup_printf(
      _("Are you sure you want to delete buddy %s from the list?"),
      purple_buddy_get_alias(buddy));
  CppConsUI::MessageDialog *dialog = new CppConsUI::MessageDialog(
      _("Buddy deletion"), msg);
  g_free(msg);
  dialog->signal_response.connect(sigc::mem_fun(this,
        &BuddyListBuddy::BuddyContextMenu::RemoveResponseHandler));
  dialog->Show();
}

int BuddyListBuddy::GetColorPair(const char *widget, const char *property)
  const
{
  if (BUDDYLIST->GetColorizationMode() != BuddyList::COLOR_BY_ACCOUNT
      || strcmp(property, "normal"))
    return Button::GetColorPair(widget, property);

  PurpleAccount *account = purple_buddy_get_account(buddy);
  int fg = purple_account_get_ui_int(account, "centerim5",
      "buddylist-foreground-color", CppConsUI::Curses::Color::DEFAULT);
  int bg = purple_account_get_ui_int(account, "centerim5",
      "buddylist-background-color", CppConsUI::Curses::Color::DEFAULT);

  CppConsUI::ColorScheme::Color c(fg, bg);
  return COLORSCHEME->GetColorPair(c);
}

void BuddyListBuddy::OpenContextMenu()
{
  ContextMenu *w = new BuddyContextMenu(*this);
  w->Show();
}

BuddyListBuddy::BuddyListBuddy(PurpleBlistNode *node)
: BuddyListNode(node)
{
  SetColorScheme("buddylistbuddy");

  buddy = reinterpret_cast<PurpleBuddy*>(node);
}

void BuddyListBuddy::UpdateColorScheme()
{
  char *new_scheme;

  switch (BUDDYLIST->GetColorizationMode()) {
    case BuddyList::COLOR_BY_STATUS:
      new_scheme = Utils::GetColorSchemeString("buddylistbuddy", buddy);
      SetColorScheme(new_scheme);
      g_free(new_scheme);
      break;
    default:
      // note: COLOR_BY_ACCOUNT case is handled by BuddyListBuddy::Draw()
      SetColorScheme("buddylistbuddy");
      break;
  }
}

bool BuddyListChat::LessOrEqual(const BuddyListNode& other) const
{
  const BuddyListChat *o = dynamic_cast<const BuddyListChat*>(&other);
  if (o)
    return g_utf8_collate(purple_chat_get_name(chat),
        purple_chat_get_name(o->chat)) <= 0;
  return LessOrEqualByType(other);
}

void BuddyListChat::Update()
{
  BuddyListNode::Update();

  const char *name = purple_chat_get_name(chat);
  SetText(name);

  SortIn();

  // hide if account is offline
  SetVisibility(purple_account_is_connected(purple_chat_get_account(chat)));

  UpdateFilterVisibility(name);
}

void BuddyListChat::OnActivate(Button& /*activator*/)
{
  PurpleAccount *account = purple_chat_get_account(chat);
  PurplePluginProtocolInfo *prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(
      purple_find_prpl(purple_account_get_protocol_id(account)));
  GHashTable *components = purple_chat_get_components(chat);

  char *chat_name = NULL;
  if (prpl_info && prpl_info->get_chat_name)
    chat_name = prpl_info->get_chat_name(components);

  const char *name;
  if (chat_name)
    name = chat_name;
  else
    name = purple_chat_get_name(chat);

  PurpleConversation *conv = purple_find_conversation_with_account(
      PURPLE_CONV_TYPE_CHAT, name, account);
  if (conv)
    purple_conversation_present(conv);

  serv_join_chat(purple_account_get_connection(account), components);

  g_free(chat_name);
}

const char *BuddyListChat::ToString() const
{
  return purple_chat_get_name(chat);
}

BuddyListChat::ChatContextMenu::ChatContextMenu(BuddyListChat& parent_chat_)
: ContextMenu(parent_chat_), parent_chat(&parent_chat_)
{
  AppendProtocolMenu(purple_account_get_connection(
          purple_chat_get_account(parent_chat->GetPurpleChat())));
  AppendExtendedMenu();

  AppendItem(_("Alias..."), sigc::mem_fun(this,
        &BuddyListChat::ChatContextMenu::OnChangeAlias));
  AppendItem(_("Delete..."), sigc::mem_fun(this,
        &BuddyListChat::ChatContextMenu::OnRemove));
}

void BuddyListChat::ChatContextMenu::ChangeAliasResponseHandler(
    CppConsUI::InputDialog& activator,
    CppConsUI::AbstractDialog::ResponseType response)
{
  switch (response) {
    case CppConsUI::AbstractDialog::RESPONSE_OK:
      break;
    default:
      return;
  }

  PurpleChat *chat = parent_chat->GetPurpleChat();
  purple_blist_alias_chat(chat, activator.GetText());

  // close context menu
  Close();
}

void BuddyListChat::ChatContextMenu::OnChangeAlias(Button& /*activator*/)
{
  PurpleChat *chat = parent_chat->GetPurpleChat();
  CppConsUI::InputDialog *dialog = new CppConsUI::InputDialog(
      _("Alias"), purple_chat_get_name(chat));
  dialog->signal_response.connect(sigc::mem_fun(this,
        &BuddyListChat::ChatContextMenu::ChangeAliasResponseHandler));
  dialog->Show();
}

void BuddyListChat::ChatContextMenu::RemoveResponseHandler(
    CppConsUI::MessageDialog& /*activator*/,
    CppConsUI::AbstractDialog::ResponseType response)
{
  switch (response) {
    case CppConsUI::AbstractDialog::RESPONSE_OK:
      break;
    default:
      return;
  }

  PurpleChat *chat = parent_chat->GetPurpleChat();

  /* Close the context menu before the chat is deleted because its deletion
   * can lead to destruction of this object. */
  Close();

  purple_blist_remove_chat(chat);
}

void BuddyListChat::ChatContextMenu::OnRemove(Button& /*activator*/)
{
  PurpleChat *chat = parent_chat->GetPurpleChat();
  char *msg = g_strdup_printf(
      _("Are you sure you want to delete chat %s from the list?"),
      purple_chat_get_name(chat));
  CppConsUI::MessageDialog *dialog = new CppConsUI::MessageDialog(
      _("Chat deletion"), msg);
  g_free(msg);
  dialog->signal_response.connect(sigc::mem_fun(this,
        &BuddyListChat::ChatContextMenu::RemoveResponseHandler));
  dialog->Show();
}

void BuddyListChat::OpenContextMenu()
{
  ContextMenu *w = new ChatContextMenu(*this);
  w->Show();
}

BuddyListChat::BuddyListChat(PurpleBlistNode *node)
: BuddyListNode(node)
{
  SetColorScheme("buddylistchat");

  chat = reinterpret_cast<PurpleChat*>(node);
}

bool BuddyListContact::LessOrEqual(const BuddyListNode& other) const
{
  const BuddyListContact *o = dynamic_cast<const BuddyListContact*>(&other);
  if (o) {
    PurpleBuddy *left = purple_contact_get_priority_buddy(contact);
    PurpleBuddy *right = purple_contact_get_priority_buddy(o->contact);
    return LessOrEqualByBuddySort(left, right);
  }
  return LessOrEqualByType(other);
}

void BuddyListContact::Update()
{
  BuddyListNode::Update();

  PurpleBuddy *buddy = purple_contact_get_priority_buddy(contact);
  const char *status = GetBuddyStatus(buddy);
  const char *alias = purple_contact_get_alias(contact);
  if (status[0]) {
    char *text = g_strdup_printf("%s %s", status, alias);
    SetText(text);
    g_free(text);
  }
  else
    SetText(alias);

  SortIn();

  UpdateColorScheme();

  if (!purple_account_is_connected(purple_buddy_get_account(buddy))) {
    // hide if account is offline
    SetVisibility(false);
  }
  else
    SetVisibility(BUDDYLIST->GetShowOfflineBuddiesPref() || status[0]);

  UpdateFilterVisibility(alias);
}

void BuddyListContact::OnActivate(Button& activator)
{
  PurpleBuddy *buddy = purple_contact_get_priority_buddy(contact);
  gpointer ui_data = purple_blist_node_get_ui_data(PURPLE_BLIST_NODE(buddy));
  if (ui_data) {
    BuddyListNode *bnode = reinterpret_cast<BuddyListNode*>(ui_data);
    bnode->OnActivate(activator);
  }
}

const char *BuddyListContact::ToString() const
{
  return purple_contact_get_alias(contact);
}

void BuddyListContact::SetRefNode(CppConsUI::TreeView::NodeReference n)
{
  BuddyListNode::SetRefNode(n);
  treeview->SetNodeStyle(n, CppConsUI::TreeView::STYLE_VOID);
}

BuddyListContact::ContactContextMenu::ContactContextMenu(
    BuddyListContact& parent_contact_)
: ContextMenu(parent_contact_), parent_contact(&parent_contact_)
{
  AppendExtendedMenu();

  AppendItem(_("Alias..."), sigc::mem_fun(this,
        &BuddyListContact::ContactContextMenu::OnChangeAlias));
  AppendItem(_("Delete..."), sigc::mem_fun(this,
        &BuddyListContact::ContactContextMenu::OnRemove));

  CppConsUI::MenuWindow *groups = new CppConsUI::MenuWindow(*this, AUTOSIZE,
      AUTOSIZE);

  for (PurpleBlistNode *node = purple_blist_get_root(); node;
      node = purple_blist_node_get_sibling_next(node)) {
    if (!PURPLE_BLIST_NODE_IS_GROUP(node))
      continue;

    PurpleGroup *group = reinterpret_cast<PurpleGroup*>(node);

    CppConsUI::Button *button = groups->AppendItem(
        purple_group_get_name(group), sigc::bind(sigc::mem_fun(this,
            &BuddyListContact::ContactContextMenu::OnMoveTo), group));
    if (purple_contact_get_group(parent_contact->GetPurpleContact())
        == group)
      button->GrabFocus();
  }

  AppendSubMenu(_("Move to..."), *groups);
}

void BuddyListContact::ContactContextMenu::ChangeAliasResponseHandler(
    CppConsUI::InputDialog& activator,
    CppConsUI::AbstractDialog::ResponseType response)
{
  switch (response) {
    case CppConsUI::AbstractDialog::RESPONSE_OK:
      break;
    default:
      return;
  }

  PurpleContact *contact = parent_contact->GetPurpleContact();
  if (contact->alias)
    purple_blist_alias_contact(contact, activator.GetText());
  else {
    PurpleBuddy *buddy = purple_contact_get_priority_buddy(contact);
    purple_blist_alias_buddy(buddy, activator.GetText());
    serv_alias_buddy(buddy);
  }

  // close context menu
  Close();
}

void BuddyListContact::ContactContextMenu::OnChangeAlias(Button& /*activator*/)
{
  PurpleContact *contact = parent_contact->GetPurpleContact();
  CppConsUI::InputDialog *dialog = new CppConsUI::InputDialog(
      _("Alias"), purple_contact_get_alias(contact));
  dialog->signal_response.connect(sigc::mem_fun(this,
        &BuddyListContact::ContactContextMenu::ChangeAliasResponseHandler));
  dialog->Show();
}

void BuddyListContact::ContactContextMenu::RemoveResponseHandler(
    CppConsUI::MessageDialog& /*activator*/,
    CppConsUI::AbstractDialog::ResponseType response)
{
  switch (response) {
    case CppConsUI::AbstractDialog::RESPONSE_OK:
      break;
    default:
      return;
  }

  // based on gtkdialogs.c:pidgin_dialogs_remove_contact_cb()
  PurpleContact *contact = parent_contact->GetPurpleContact();
  PurpleBlistNode *cnode = reinterpret_cast<PurpleBlistNode*>(contact);
  PurpleGroup *group = reinterpret_cast<PurpleGroup*>(
      purple_blist_node_get_parent(cnode));

  for (PurpleBlistNode *bnode = purple_blist_node_get_first_child(cnode);
      bnode; bnode = purple_blist_node_get_sibling_next(bnode)) {
    PurpleBuddy *buddy = reinterpret_cast<PurpleBuddy*>(bnode);
    PurpleAccount *account = purple_buddy_get_account(buddy);
    if (purple_account_is_connected(account))
      purple_account_remove_buddy(account, buddy, group);
  }

  /* Close the context menu before the contact is deleted because its deletion
   * can lead to destruction of this object. */
  Close();

  purple_blist_remove_contact(contact);
}

void BuddyListContact::ContactContextMenu::OnRemove(Button& /*activator*/)
{
  PurpleContact *contact = parent_contact->GetPurpleContact();
  char *msg = g_strdup_printf(
      _("Are you sure you want to delete contact %s from the list?"),
      purple_buddy_get_alias(purple_contact_get_priority_buddy(contact)));
  CppConsUI::MessageDialog *dialog = new CppConsUI::MessageDialog(
      _("Contact deletion"), msg);
  g_free(msg);
  dialog->signal_response.connect(sigc::mem_fun(this,
        &BuddyListContact::ContactContextMenu::RemoveResponseHandler));
  dialog->Show();
}

void BuddyListContact::ContactContextMenu::OnMoveTo(Button& /*activator*/,
    PurpleGroup *group)
{
  purple_blist_add_contact(parent_contact->GetPurpleContact(), group, NULL);
}

int BuddyListContact::GetColorPair(const char *widget, const char *property)
  const
{
  if (BUDDYLIST->GetColorizationMode() != BuddyList::COLOR_BY_ACCOUNT
      || strcmp(property, "normal"))
    return Button::GetColorPair(widget, property);

  PurpleAccount *account =
    purple_buddy_get_account(purple_contact_get_priority_buddy(contact));
  int fg = purple_account_get_ui_int(account, "centerim5",
      "buddylist-foreground-color", CppConsUI::Curses::Color::DEFAULT);
  int bg = purple_account_get_ui_int(account, "centerim5",
      "buddylist-background-color", CppConsUI::Curses::Color::DEFAULT);

  CppConsUI::ColorScheme::Color c(fg,bg);
  return COLORSCHEME->GetColorPair(c);
}

void BuddyListContact::OpenContextMenu()
{
  ContextMenu *w = new ContactContextMenu(*this);
  w->Show();
}

BuddyListContact::BuddyListContact(PurpleBlistNode *node)
: BuddyListNode(node)
{
  SetColorScheme("buddylistcontact");

  contact = reinterpret_cast<PurpleContact*>(node);
}

void BuddyListContact::UpdateColorScheme()
{
  char *new_scheme;
  PurpleBuddy *buddy;

  switch (BUDDYLIST->GetColorizationMode()) {
    case BuddyList::COLOR_BY_STATUS:
      buddy = purple_contact_get_priority_buddy(contact);
      new_scheme = Utils::GetColorSchemeString("buddylistcontact", buddy);
      SetColorScheme(new_scheme);
      g_free(new_scheme);
      break;
    default:
      // note: COLOR_BY_ACCOUNT case is handled by BuddyListContact::Draw()
      SetColorScheme("buddylistcontact");
      break;
  }
}

bool BuddyListGroup::LessOrEqual(const BuddyListNode& other) const
{
  const BuddyListGroup *o = dynamic_cast<const BuddyListGroup*>(&other);
  if (o)
    return g_utf8_collate(purple_group_get_name(group),
        purple_group_get_name(o->group)) <= 0;
  return LessOrEqualByType(other);
}

void BuddyListGroup::Update()
{
  BuddyListNode::Update();

  SetText(purple_group_get_name(group));

  SortIn();

  bool vis = true;
  if (!BUDDYLIST->GetShowEmptyGroupsPref())
    vis = purple_blist_get_group_size(group, FALSE);

  SetVisibility(vis);
}

void BuddyListGroup::OnActivate(Button& /*activator*/)
{
  treeview->ToggleCollapsed(ref);
  purple_blist_node_set_bool(node, "collapsed", ref->GetCollapsed());
}

const char *BuddyListGroup::ToString() const
{
  return purple_group_get_name(group);
}

void BuddyListGroup::DelayedInit()
{
  /* This can't be done when the node is created because node settings are
   * unavailable at that time. */
  if (!purple_blist_node_get_bool(node, "collapsed"))
    treeview->SetCollapsed(ref, false);
}

BuddyListGroup::GroupContextMenu::GroupContextMenu(
    BuddyListGroup& parent_group_)
: ContextMenu(parent_group_), parent_group(&parent_group_)
{
  AppendExtendedMenu();

  AppendItem(_("Rename..."), sigc::mem_fun(this,
        &BuddyListGroup::GroupContextMenu::OnRename));
  AppendItem(_("Delete..."), sigc::mem_fun(this,
        &BuddyListGroup::GroupContextMenu::OnRemove));
}

void BuddyListGroup::GroupContextMenu::RenameResponseHandler(
    CppConsUI::InputDialog& activator,
    CppConsUI::AbstractDialog::ResponseType response)
{
  switch (response) {
    case CppConsUI::AbstractDialog::RESPONSE_OK:
      break;
    default:
      return;
  }

  const char *name = activator.GetText();
  PurpleGroup *group = parent_group->GetPurpleGroup();
  PurpleGroup *other = purple_find_group(name);
  if (other && !purple_utf8_strcasecmp(name, purple_group_get_name(group))) {
    LOG->Message(_("Specified group is already in the list."));
    /* TODO Add group merging. Note that purple_blist_rename_group() can do
     * the merging. */
  }
  else
    purple_blist_rename_group(group, name);

  // close context menu
  Close();
}

void BuddyListGroup::GroupContextMenu::OnRename(Button& /*activator*/)
{
  PurpleGroup *group = parent_group->GetPurpleGroup();
  CppConsUI::InputDialog *dialog = new CppConsUI::InputDialog(
      _("Rename"), purple_group_get_name(group));
  dialog->signal_response.connect(sigc::mem_fun(this,
        &BuddyListGroup::GroupContextMenu::RenameResponseHandler));
  dialog->Show();
}

void BuddyListGroup::GroupContextMenu::RemoveResponseHandler(
    CppConsUI::MessageDialog& /*activator*/,
    CppConsUI::AbstractDialog::ResponseType response)
{
  switch (response) {
    case CppConsUI::AbstractDialog::RESPONSE_OK:
      break;
    default:
      return;
  }

  // based on gtkdialogs.c:pidgin_dialogs_remove_group_cb()
  PurpleGroup *group = parent_group->GetPurpleGroup();
  PurpleBlistNode *cnode = purple_blist_node_get_first_child(
      reinterpret_cast<PurpleBlistNode*>(group));
  while (cnode) {
    if (PURPLE_BLIST_NODE_IS_CONTACT(cnode)) {
      PurpleBlistNode *bnode = purple_blist_node_get_first_child(cnode);
      cnode = purple_blist_node_get_sibling_next(cnode);
      while (bnode)
        if (PURPLE_BLIST_NODE_IS_BUDDY(bnode)) {
          PurpleBuddy *buddy = reinterpret_cast<PurpleBuddy*>(bnode);
          PurpleAccount *account = purple_buddy_get_account(buddy);
          bnode = purple_blist_node_get_sibling_next(bnode);
          if (purple_account_is_connected(account)) {
            purple_account_remove_buddy(account, buddy, group);
            purple_blist_remove_buddy(buddy);
          }
        }
        else
          bnode = purple_blist_node_get_sibling_next(bnode);
    }
    else if (PURPLE_BLIST_NODE_IS_CHAT(cnode)) {
      PurpleChat *chat = reinterpret_cast<PurpleChat*>(cnode);
      cnode = purple_blist_node_get_sibling_next(cnode);
      purple_blist_remove_chat(chat);
    }
    else
      cnode = purple_blist_node_get_sibling_next(cnode);
  }

  /* Close the context menu before the group is deleted because its deletion
   * can lead to destruction of this object. */
  Close();

  purple_blist_remove_group(group);
}

void BuddyListGroup::GroupContextMenu::OnRemove(Button& /*activator*/)
{
  PurpleGroup *group = parent_group->GetPurpleGroup();
  char *msg = g_strdup_printf(
      _("Are you sure you want to delete group %s from the list?"),
      purple_group_get_name(group));
  CppConsUI::MessageDialog *dialog
    = new CppConsUI::MessageDialog(_("Group deletion"), msg);
  g_free(msg);
  dialog->signal_response.connect(sigc::mem_fun(this,
        &BuddyListGroup::GroupContextMenu::RemoveResponseHandler));
  dialog->Show();
}

void BuddyListGroup::OpenContextMenu()
{
  ContextMenu *w = new GroupContextMenu(*this);
  w->Show();
}

BuddyListGroup::BuddyListGroup(PurpleBlistNode *node)
: BuddyListNode(node)
{
  SetColorScheme("buddylistgroup");

  group = reinterpret_cast<PurpleGroup*>(node);
}

/* vim: set tabstop=2 shiftwidth=2 textwidth=78 expandtab : */
