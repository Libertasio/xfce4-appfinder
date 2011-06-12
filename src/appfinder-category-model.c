/*
 * Copyright (C) 2011 Nick Schermer <nick@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <libxfce4util/libxfce4util.h>

#include <src/appfinder-model.h>
#include <src/appfinder-category-model.h>
#include <src/appfinder-private.h>



static void               xfce_appfinder_category_model_tree_model_init       (GtkTreeModelIface        *iface);
static void               xfce_appfinder_category_model_finalize              (GObject                  *object);
static GtkTreeModelFlags  xfce_appfinder_category_model_get_flags             (GtkTreeModel             *tree_model);
static gint               xfce_appfinder_category_model_get_n_columns         (GtkTreeModel             *tree_model);
static GType              xfce_appfinder_category_model_get_column_type       (GtkTreeModel             *tree_model,
                                                                               gint                      column);
static gboolean           xfce_appfinder_category_model_get_iter              (GtkTreeModel             *tree_model,
                                                                               GtkTreeIter              *iter,
                                                                               GtkTreePath              *path);
static GtkTreePath       *xfce_appfinder_category_model_get_path              (GtkTreeModel             *tree_model,
                                                                               GtkTreeIter              *iter);
static void               xfce_appfinder_category_model_get_value             (GtkTreeModel             *tree_model,
                                                                               GtkTreeIter              *iter,
                                                                               gint                      column,
                                                                               GValue                   *value);
static gboolean           xfce_appfinder_category_model_iter_next             (GtkTreeModel             *tree_model,
                                                                               GtkTreeIter              *iter);
static gboolean           xfce_appfinder_category_model_iter_children         (GtkTreeModel             *tree_model,
                                                                               GtkTreeIter              *iter,
                                                                               GtkTreeIter              *parent);
static gboolean           xfce_appfinder_category_model_iter_has_child        (GtkTreeModel             *tree_model,
                                                                               GtkTreeIter              *iter);
static gint               xfce_appfinder_category_model_iter_n_children       (GtkTreeModel             *tree_model,
                                                                               GtkTreeIter              *iter);
static gboolean           xfce_appfinder_category_model_iter_nth_child        (GtkTreeModel             *tree_model,
                                                                               GtkTreeIter              *iter,
                                                                               GtkTreeIter              *parent,
                                                                               gint                      n);
static gboolean           xfce_appfinder_category_model_iter_parent           (GtkTreeModel             *tree_model,
                                                                               GtkTreeIter              *iter,
                                                                               GtkTreeIter              *child);



struct _XfceAppfinderCategoryModelClass
{
  GObjectClass __parent__;
};

struct _XfceAppfinderCategoryModel
{
  GObject              __parent__;
  gint                 stamp;

  GSList              *categories;

  GarconMenuDirectory *all_applications;
};



G_DEFINE_TYPE_WITH_CODE (XfceAppfinderCategoryModel, xfce_appfinder_category_model, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL, xfce_appfinder_category_model_tree_model_init))



static void
xfce_appfinder_category_model_class_init (XfceAppfinderCategoryModelClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = xfce_appfinder_category_model_finalize;
}



static void
xfce_appfinder_category_model_init (XfceAppfinderCategoryModel *model)
{
  /* generate a unique stamp */
  model->stamp = g_random_int ();
  model->all_applications = g_object_new (GARCON_TYPE_MENU_DIRECTORY,
                                          "name", _("All Applications"),
                                          "icon-name", "applications-other", NULL);
}



static void
xfce_appfinder_category_model_tree_model_init (GtkTreeModelIface *iface)
{
  iface->get_flags = xfce_appfinder_category_model_get_flags;
  iface->get_n_columns = xfce_appfinder_category_model_get_n_columns;
  iface->get_column_type = xfce_appfinder_category_model_get_column_type;
  iface->get_iter = xfce_appfinder_category_model_get_iter;
  iface->get_path = xfce_appfinder_category_model_get_path;
  iface->get_value = xfce_appfinder_category_model_get_value;
  iface->iter_next = xfce_appfinder_category_model_iter_next;
  iface->iter_children = xfce_appfinder_category_model_iter_children;
  iface->iter_has_child = xfce_appfinder_category_model_iter_has_child;
  iface->iter_n_children = xfce_appfinder_category_model_iter_n_children;
  iface->iter_nth_child = xfce_appfinder_category_model_iter_nth_child;
  iface->iter_parent = xfce_appfinder_category_model_iter_parent;
}



static void
xfce_appfinder_category_model_finalize (GObject *object)
{
  XfceAppfinderCategoryModel *model = XFCE_APPFINDER_CATEGORY_MODEL (object);
  GSList                     *li;
  guint                       n;

  /* clear the first three items */
  for (li = model->categories, n = 0; li != NULL && n < 3; li = li->next, n++)
    xfce_appfinder_model_category_free (li->data);
  g_slist_free (model->categories);

  g_object_unref (G_OBJECT (model->all_applications));

  (*G_OBJECT_CLASS (xfce_appfinder_category_model_parent_class)->finalize) (object);
}



static GtkTreeModelFlags
xfce_appfinder_category_model_get_flags (GtkTreeModel *tree_model)
{
  return GTK_TREE_MODEL_ITERS_PERSIST | GTK_TREE_MODEL_LIST_ONLY;
}



static gint
xfce_appfinder_category_model_get_n_columns (GtkTreeModel *tree_model)
{
  return XFCE_APPFINDER_CATEGORY_MODEL_N_COLUMNS;
}



static GType
xfce_appfinder_category_model_get_column_type (GtkTreeModel *tree_model,
                                               gint          column)
{
  switch (column)
    {
    case XFCE_APPFINDER_CATEGORY_MODEL_COLUMN_NAME:
      return G_TYPE_STRING;

    case XFCE_APPFINDER_CATEGORY_MODEL_COLUMN_ICON:
      return GDK_TYPE_PIXBUF;

    case XFCE_APPFINDER_CATEGORY_MODEL_COLUMN_DIRECTORY:
      return GARCON_TYPE_MENU_DIRECTORY;

    default:
      g_assert_not_reached ();
      return G_TYPE_INVALID;
    }
}



static gboolean
xfce_appfinder_category_model_get_iter (GtkTreeModel *tree_model,
                                        GtkTreeIter  *iter,
                                        GtkTreePath  *path)
{
  XfceAppfinderCategoryModel *model = XFCE_APPFINDER_CATEGORY_MODEL (tree_model);

  g_return_val_if_fail (XFCE_IS_APPFINDER_CATEGORY_MODEL (model), FALSE);
  g_return_val_if_fail (gtk_tree_path_get_depth (path) > 0, FALSE);

  iter->stamp = model->stamp;
  iter->user_data = g_slist_nth (model->categories, gtk_tree_path_get_indices (path)[0]);

  return (iter->user_data != NULL);
}



static GtkTreePath*
xfce_appfinder_category_model_get_path (GtkTreeModel *tree_model,
                                        GtkTreeIter  *iter)
{
  XfceAppfinderCategoryModel *model = XFCE_APPFINDER_CATEGORY_MODEL (tree_model);
  gint                        idx;

  g_return_val_if_fail (XFCE_IS_APPFINDER_CATEGORY_MODEL (model), NULL);
  g_return_val_if_fail (iter->stamp == model->stamp, NULL);

  /* determine the index of the iter */
  idx = g_slist_position (model->categories, iter->user_data);
  if (G_UNLIKELY (idx < 0))
    return NULL;

  return gtk_tree_path_new_from_indices (idx, -1);
}



static void
xfce_appfinder_category_model_get_value (GtkTreeModel *tree_model,
                                         GtkTreeIter  *iter,
                                         gint          column,
                                         GValue       *value)
{
  XfceAppfinderCategoryModel *model = XFCE_APPFINDER_CATEGORY_MODEL (tree_model);
  CategoryItem               *item;
  const gchar                *icon_name;

  g_return_if_fail (XFCE_IS_APPFINDER_CATEGORY_MODEL (model));
  g_return_if_fail (iter->stamp == model->stamp);

  item = ITER_GET_DATA (iter);
  g_return_if_fail (item->directory == NULL || GARCON_IS_MENU_DIRECTORY (item->directory));

  switch (column)
    {
    case XFCE_APPFINDER_CATEGORY_MODEL_COLUMN_NAME:
      g_value_init (value, G_TYPE_STRING);
      if (item->directory != NULL)
        g_value_set_static_string (value, garcon_menu_directory_get_name (item->directory));
      break;

    case XFCE_APPFINDER_CATEGORY_MODEL_COLUMN_ICON:
      if (item->pixbuf == NULL
          && item->directory != NULL)
        {
          icon_name = garcon_menu_directory_get_icon_name (item->directory);
          item->pixbuf = xfce_appfinder_model_load_pixbuf (icon_name, 24);
        }

      g_value_init (value, GDK_TYPE_PIXBUF);
      g_value_set_object (value, item->pixbuf);
      break;

    case XFCE_APPFINDER_CATEGORY_MODEL_COLUMN_DIRECTORY:
      g_value_init (value, G_TYPE_OBJECT);
      /* return null for all applications */
      if (item->directory != model->all_applications)
        g_value_set_object (value, item->directory);
      break;

    default:
      g_assert_not_reached ();
      break;
    }
}



static gboolean
xfce_appfinder_category_model_iter_next (GtkTreeModel *tree_model,
                                         GtkTreeIter  *iter)
{
  g_return_val_if_fail (XFCE_IS_APPFINDER_CATEGORY_MODEL (tree_model), FALSE);
  g_return_val_if_fail (iter->stamp == XFCE_APPFINDER_CATEGORY_MODEL (tree_model)->stamp, FALSE);

  iter->user_data = g_slist_next (iter->user_data);
  return (iter->user_data != NULL);
}



static gboolean
xfce_appfinder_category_model_iter_children (GtkTreeModel *tree_model,
                                             GtkTreeIter  *iter,
                                             GtkTreeIter  *parent)
{
  XfceAppfinderCategoryModel *model = XFCE_APPFINDER_CATEGORY_MODEL (tree_model);

  g_return_val_if_fail (XFCE_IS_APPFINDER_CATEGORY_MODEL (model), FALSE);

  if (G_LIKELY (parent == NULL && model->categories != NULL))
    {
      iter->stamp = model->stamp;
      iter->user_data = model->categories;
      return TRUE;
    }

  return FALSE;
}



static gboolean
xfce_appfinder_category_model_iter_has_child (GtkTreeModel *tree_model,
                                              GtkTreeIter  *iter)
{
  return FALSE;
}



static gint
xfce_appfinder_category_model_iter_n_children (GtkTreeModel *tree_model,
                                               GtkTreeIter  *iter)
{
  XfceAppfinderCategoryModel *model = XFCE_APPFINDER_CATEGORY_MODEL (tree_model);

  g_return_val_if_fail (XFCE_IS_APPFINDER_CATEGORY_MODEL (model), 0);

  return (iter == NULL) ? g_slist_length (model->categories) : 0;
}



static gboolean
xfce_appfinder_category_model_iter_nth_child (GtkTreeModel *tree_model,
                                              GtkTreeIter  *iter,
                                              GtkTreeIter  *parent,
                                              gint          n)
{
  XfceAppfinderCategoryModel *model = XFCE_APPFINDER_CATEGORY_MODEL (tree_model);

  g_return_val_if_fail (XFCE_IS_APPFINDER_CATEGORY_MODEL (model), FALSE);

  if (G_LIKELY (parent != NULL))
    {
      iter->stamp = model->stamp;
      iter->user_data = g_slist_nth (model->categories, n);
      return (iter->user_data != NULL);
    }

  return FALSE;
}



static gboolean
xfce_appfinder_category_model_iter_parent (GtkTreeModel *tree_model,
                                           GtkTreeIter  *iter,
                                           GtkTreeIter  *child)
{
  return FALSE;
}



XfceAppfinderCategoryModel *
xfce_appfinder_category_model_new (void)
{
  return g_object_new (XFCE_TYPE_APPFINDER_CATEGORY_MODEL, NULL);
}



void
xfce_appfinder_category_model_set_categories (XfceAppfinderCategoryModel *model,
                                              GSList                     *categories)
{
  CategoryItem *item;
  GSList       *li, *lnext;
  gint          idx;
  GtkTreePath  *path;
  GtkTreeIter   iter;

  APPFINDER_DEBUG ("insert %d categories", g_slist_length (categories));

  /* remove items from the model */
  for (li = model->categories, idx = 0; li != NULL; li = lnext, idx++)
    {
      lnext = li->next;

      model->categories = g_slist_delete_link (model->categories, li);

      /* remove the items we own */
      if (idx < 3)
        xfce_appfinder_model_category_free (li->data);

      path = gtk_tree_path_new_from_indices (idx, -1);
      gtk_tree_model_row_deleted (GTK_TREE_MODEL (model), path);
      gtk_tree_path_free (path);
    }

  g_assert (model->categories == NULL);

  /* separator and the main categories */
  item = g_slice_new0 (CategoryItem);
  model->categories = g_slist_prepend (model->categories, item);

  item = g_slice_new0 (CategoryItem);
  item->directory = xfce_appfinder_model_get_command_category ();
  model->categories = g_slist_prepend (model->categories, item);

  item = g_slice_new0 (CategoryItem);
  item->directory = g_object_ref (G_OBJECT (model->all_applications));
  model->categories = g_slist_prepend (model->categories, item);

  /* move the categories online */
  model->categories = g_slist_concat (model->categories, g_slist_copy (categories));

  path = gtk_tree_path_new_first ();
  for (li = model->categories; li != NULL; li = li->next)
    {
      /* remember the next item */
      lnext = li->next;
      li->next = NULL;

      /* emit the "row-inserted" signal */
      ITER_INIT (iter, model->stamp, li);
      gtk_tree_model_row_inserted (GTK_TREE_MODEL (model), path, &iter);

      /* advance the path */
      gtk_tree_path_next (path);

      /* reset the next item */
      li->next = lnext;
    }
  gtk_tree_path_free (path);
}



gboolean
xfce_appfinder_category_model_row_separator_func (GtkTreeModel *tree_model,
                                                  GtkTreeIter  *iter,
                                                  gpointer      user_data)
{
  CategoryItem *item = ITER_GET_DATA (iter);

  g_return_val_if_fail (XFCE_IS_APPFINDER_CATEGORY_MODEL (tree_model), FALSE);

  return (item->directory == NULL);
}



void
xfce_appfinder_category_model_icon_theme_changed (XfceAppfinderCategoryModel *model)
{
  CategoryItem *item;
  GSList       *li;
  gint          idx;
  GtkTreeIter   iter;
  GtkTreePath  *path;

  g_return_if_fail (XFCE_IS_APPFINDER_CATEGORY_MODEL (model));

  for (li = model->categories, idx = 0; li != NULL; li = li->next, idx++)
    {
      item = li->data;

      if (item->pixbuf != NULL)
        {
          g_object_unref (G_OBJECT (item->pixbuf));
          item->pixbuf = NULL;

          path = gtk_tree_path_new_from_indices (idx, -1);
          ITER_INIT (iter, model->stamp, li);
          gtk_tree_model_row_changed (GTK_TREE_MODEL (model), path, &iter);
          gtk_tree_path_free (path);
        }
    }
}
