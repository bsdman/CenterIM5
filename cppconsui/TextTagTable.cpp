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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

//#include "config.h"
#include "TextTagTable.h"
//#include "gtkmarshalers.h"
#include "TextBuffer.h" /* just for the lame notify_will_remove_tag hack */
//#include "gtkintl.h"
//#include "gtkalias.h"

//#include <stdlib.h>

enum {
  TAG_CHANGED,
  TAG_ADDED,
  TAG_REMOVED,
  LAST_SIGNAL
};

enum {
  LAST_ARG
};

/*
static void gtk_text_tag_table_finalize     (GObject              *object);
static void gtk_text_tag_table_set_property (GObject              *object,
                                             guint                 prop_id,
                                             const GValue         *value,
                                             GParamSpec           *pspec);
static void gtk_text_tag_table_get_property (GObject              *object,
                                             guint                 prop_id,
                                             GValue               *value,
                                             GParamSpec           *pspec);
*/

static guint signals[LAST_SIGNAL] = { 0 };

//G_DEFINE_TYPE (GtkTextTagTable, gtk_text_tag_table, G_TYPE_OBJECT)

/*
static void
gtk_text_tag_table_class_init (GtkTextTagTableClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gtk_text_tag_table_set_property;
  object_class->get_property = gtk_text_tag_table_get_property;
  
  object_class->finalize = gtk_text_tag_table_finalize;
  
  signals[TAG_CHANGED] =
    g_signal_new (I_("tag-changed"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GtkTextTagTableClass, tag_changed),
                  NULL, NULL,
                  _gtk_marshal_VOID__OBJECT_BOOLEAN,
                  G_TYPE_NONE,
                  2,
                  GTK_TYPE_TEXT_TAG,
                  G_TYPE_BOOLEAN);  

  signals[TAG_ADDED] =
    g_signal_new (I_("tag-added"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GtkTextTagTableClass, tag_added),
                  NULL, NULL,
                  _gtk_marshal_VOID__OBJECT,
                  G_TYPE_NONE,
                  1,
                  GTK_TYPE_TEXT_TAG);

  signals[TAG_REMOVED] =
    g_signal_new (I_("tag-removed"),  
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GtkTextTagTableClass, tag_removed),
                  NULL, NULL,
                  _gtk_marshal_VOID__OBJECT,
                  G_TYPE_NONE,
                  1,
                  GTK_TYPE_TEXT_TAG);
}*/

TextTagTable::TextTagTable()
{
  hash = g_hash_table_new (g_str_hash, g_str_equal);
}

TextTagTable::~TextTagTable()
{
	foreach (TextTagTable::foreach_unref, NULL);

  g_hash_table_destroy (hash);
  //g_slist_free (anonymous);

  g_slist_free (buffers);
}

/**
 * gtk_text_tag_table_new:
 * 
 * Creates a new #GtkTextTagTable. The table contains no tags by
 * default.
 * 
 * Return value: a new #GtkTextTagTable
 **/
/*GtkTextTagTable*
gtk_text_tag_table_new (void)
{
  GtkTextTagTable *table;

  table = g_object_new (GTK_TYPE_TEXT_TAG_TABLE, NULL);

  return table;
}*/

void TextTagTable::foreach_unref (TextTag *tag, gpointer data)
{
  GSList *tmp;
  
  /* We don't want to emit the remove signal here; so we just unparent
   * and unref the tag.
   */

  tmp = tag->table->buffers;
  while (tmp != NULL)
    {
      ((TextBuffer*)tmp->data)->notify_will_remove_tag ( tag);
      
      tmp = tmp->next;
    }
  
  tag->table = NULL;
  //TODOg_object_unref (tag);
}

/*
static void gtk_text_tag_table_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  switch (prop_id)
    {

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
gtk_text_tag_table_get_property (GObject      *object,
                                 guint         prop_id,
                                 GValue       *value,
                                 GParamSpec   *pspec)
{
  switch (prop_id)
    {

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}*/

/**
 * gtk_text_tag_table_add:
 * @table: a #GtkTextTagTable
 * @tag: a #GtkTextTag
 *
 * Add a tag to the table. The tag is assigned the highest priority
 * in the table.
 *
 * @tag must not be in a tag table already, and may not have
 * the same name as an already-added tag.
 **/
void
TextTagTable::add_tag (TextTag      *tag)
{
  guint size;

 // g_return_if_fail (GTK_IS_TEXT_TAG_TABLE (table));
 //TODO g_return_if_fail (GTK_IS_TEXT_TAG (tag));
  g_return_if_fail (tag->table == NULL);

  if (tag->name && g_hash_table_lookup (hash, tag->name))
    {
      g_warning ("A tag named '%s' is already in the tag table.",
                 tag->name);
      return;
    }
  
  //TODOg_object_ref (tag);

  if (tag->name)
    g_hash_table_insert (hash, tag->name, tag);
  else
    {
	anonymous.push_front(tag);
      //anonymous = g_slist_prepend (anonymous, tag);
      anon_count += 1;
    }

  tag->table = this;

  /* We get the highest tag priority, as the most-recently-added
     tag. Note that we do NOT use gtk_text_tag_set_priority,
     as it assumes the tag is already in the table. */
  size = get_size ();
  g_assert (size > 0);
  tag->priority = size - 1;

  //TODOg_signal_emit (table, signals[TAG_ADDED], 0, tag);
}

/**
 * gtk_text_tag_table_lookup:
 * @table: a #GtkTextTagTable 
 * @name: name of a tag
 * 
 * Look up a named tag.
 * 
 * Return value: The tag, or %NULL if none by that name is in the table.
 **/
TextTag* TextTagTable::lookup ( const gchar     *name)
{
 // g_return_val_if_fail (GTK_IS_TEXT_TAG_TABLE (table), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  return (TextTag*) g_hash_table_lookup (hash, name);
}

/**
 * gtk_text_tag_table_remove:
 * @table: a #GtkTextTagTable
 * @tag: a #GtkTextTag
 * 
 * Remove a tag from the table. This will remove the table's
 * reference to the tag, so be careful - the tag will end
 * up destroyed if you don't have a reference to it.
 **/
void TextTagTable::delete_tag (TextTag *tag)
{
  GSList *tmp;
  
 // g_return_if_fail (GTK_IS_TEXT_TAG_TABLE (table));
  //TODOg_return_if_fail (GTK_IS_TEXT_TAG (tag));
  g_return_if_fail (tag->table == this);

  /* Our little bad hack to be sure buffers don't still have the tag
   * applied to text in the buffer
   */
  tmp = buffers;
  while (tmp != NULL)
    {
      ((TextBuffer*)tmp->data)->notify_will_remove_tag ( tag);
      
      tmp = tmp->next;
    }
  
  /* Set ourselves to the highest priority; this means
     when we're removed, there won't be any gaps in the
     priorities of the tags in the table. */
  tag->set_priority (get_size () - 1);

  tag->table = NULL;

  if (tag->name)
    g_hash_table_remove (hash, tag->name);
  else
    {
	    anonymous.remove(tag);//TODO g_slist_remove removes ony 1, std::list::remove() removes all with the value.
      //anonymous = g_slist_remove (anonymous, tag);
      anon_count -= 1;
    }

  //TODOg_signal_emit (table, signals[TAG_REMOVED], 0, tag);

  //TODOg_object_unref (tag);
}

struct ForeachData
{
  TextTagTableForeach func;
  gpointer data;
};

void
TextTagTable::hash_foreach (gpointer key, gpointer value, gpointer data)
{
  struct ForeachData *fd = (ForeachData*)data;

  //TODOg_return_if_fail (GTK_IS_TEXT_TAG (value));

  (* fd->func) ((TextTag*)value, fd->data);
}

void
TextTagTable::list_foreach (gpointer data, gpointer user_data)
{
  struct ForeachData *fd = (ForeachData*)user_data;

  //TODOg_return_if_fail (GTK_IS_TEXT_TAG (data));

  (* fd->func) ((TextTag*)data, fd->data);
}

/**
 * gtk_text_tag_table_foreach:
 * @table: a #GtkTextTagTable
 * @func: a function to call on each tag
 * @data: user data
 *
 * Calls @func on each tag in @table, with user data @data.
 * Note that the table may not be modified while iterating 
 * over it (you can't add/remove tags).
 **/
void TextTagTable::foreach  (
			      TextTagTableForeach  func,
			      gpointer                data)
{
  struct ForeachData d;
  std::list<TextTag*>::iterator i;

  //TODOg_return_if_fail (GTK_IS_TEXT_TAG_TABLE (table));
  g_return_if_fail (func != NULL);

  d.func = func;
  d.data = data;

  g_hash_table_foreach (hash, hash_foreach, &d);
  //g_slist_foreach (anonymous, list_foreach, &d);
  for(i = anonymous.begin(); i != anonymous.end(); i++)
	  func(*i, data);
}

/**
 * gtk_text_tag_table_get_size:
 * @table: a #GtkTextTagTable
 * 
 * Returns the size of the table (number of tags)
 * 
 * Return value: number of tags in @table
 **/
gint TextTagTable::get_size ()
{
 // g_return_val_if_fail (GTK_IS_TEXT_TAG_TABLE (table), 0);

  return g_hash_table_size (hash) + anon_count;
}

void TextTagTable::add_buffer ( gpointer         buffer)
{
 // g_return_if_fail (GTK_IS_TEXT_TAG_TABLE (table));

  buffers = g_slist_prepend (buffers, buffer);
}

void TextTagTable::foreach_remove_tag (TextTag *tag, gpointer data)
{
  TextBuffer *buffer;

  buffer = (TextBuffer*)data;

  buffer->notify_will_remove_tag (tag);
}

void TextTagTable::remove_buffer ( gpointer         buffer)
{
 // g_return_if_fail (GTK_IS_TEXT_TAG_TABLE (table));

  foreach (TextTagTable::foreach_remove_tag, buffer);
  
  buffers = g_slist_remove (buffers, buffer);
}

void
TextTagTable::listify_foreach (TextTag *tag, gpointer user_data)
{
  GSList** listp = (GSList**)user_data;

  *listp = g_slist_prepend (*listp, tag);
}

 GSList* TextTagTable::list_of_tags (void)
{
  GSList *list = NULL;

  TextTagTable::foreach (TextTagTable::listify_foreach, &list);

  return list;
}
