/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 2015 Kirk A. Baker/Bill W. Kelly, Camera Bits, Inc.
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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.Free
 */

/*
 * Modified by the GTK+ Team and others 1997-2006.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#include "config.h"

#include "gtkmain.h"
#include "gtkmarshalers.h"
#include "gtksizerequest.h"
#include "gtkquartzembedwidget.h"
#include "gtkdebug.h"
#include "gtkintl.h"
#include "gtkprivate.h"
#include "gtkwindowprivate.h"
#include "gtkwidgetprivate.h"
#include "gtkcontainerprivate.h"


static void            gtk_quartz_embed_widget_realize               (GtkWidget        *widget);
static void            gtk_quartz_embed_widget_unrealize             (GtkWidget        *widget);
static void            gtk_quartz_embed_widget_show                  (GtkWidget        *widget);
static void            gtk_quartz_embed_widget_hide                  (GtkWidget        *widget);
static void            gtk_quartz_embed_widget_map                   (GtkWidget        *widget);
static void            gtk_quartz_embed_widget_unmap                 (GtkWidget        *widget);
static void            gtk_quartz_embed_widget_size_allocate         (GtkWidget        *widget,
								     GtkAllocation    *allocation);
static void            gtk_quartz_embed_widget_set_focus             (GtkWindow        *window,
								     GtkWidget        *focus);
static gboolean        gtk_quartz_embed_widget_focus                 (GtkWidget        *widget,
								     GtkDirectionType  direction);
static void            gtk_quartz_embed_widget_check_resize          (GtkContainer     *container);

static GtkBinClass *bin_class = NULL;

G_DEFINE_TYPE (GtkQuartzEmbedWidget, gtk_quartz_embed_widget, GTK_TYPE_WINDOW)

static void
gtk_quartz_embed_widget_class_init (GtkQuartzEmbedWidgetClass *class)
{
  GtkWidgetClass *widget_class = (GtkWidgetClass *)class;
  GtkWindowClass *window_class = (GtkWindowClass *)class;
  GtkContainerClass *container_class = (GtkContainerClass *)class;

  bin_class = g_type_class_peek (GTK_TYPE_BIN);

  widget_class->realize = gtk_quartz_embed_widget_realize;
  widget_class->unrealize = gtk_quartz_embed_widget_unrealize;

  widget_class->show = gtk_quartz_embed_widget_show;
  widget_class->hide = gtk_quartz_embed_widget_hide;
  widget_class->map = gtk_quartz_embed_widget_map;
  widget_class->unmap = gtk_quartz_embed_widget_unmap;
  widget_class->size_allocate = gtk_quartz_embed_widget_size_allocate;

  widget_class->focus = gtk_quartz_embed_widget_focus;
  
  container_class->check_resize = gtk_quartz_embed_widget_check_resize;

  window_class->set_focus = gtk_quartz_embed_widget_set_focus;
}

static void
gtk_quartz_embed_widget_init (GtkQuartzEmbedWidget *embed_widget)
{
  _gtk_widget_set_is_toplevel (GTK_WIDGET (embed_widget), TRUE);
  gtk_container_set_resize_mode (GTK_CONTAINER (embed_widget), GTK_RESIZE_QUEUE);
}

GtkWidget*
_gtk_quartz_embed_widget_new (void *parent)
{
  GtkQuartzEmbedWidget *embed_widget;

  embed_widget = g_object_new (GTK_TYPE_QUARTZ_EMBED_WIDGET, NULL);
  
  embed_widget->parent_window =
      gdk_quartz_window_foreign_new_for_display (gdk_display_get_default (),
						 parent);
  gtk_widget_register_window (embed_widget, embed_widget->parent_window);

  return GTK_WIDGET (embed_widget);
}

static void
gtk_quartz_embed_widget_unrealize (GtkWidget *widget)
{
  GtkQuartzEmbedWidget *embed_widget = GTK_QUARTZ_EMBED_WIDGET (widget);

  g_clear_object (&embed_widget->parent_window);

  GTK_WIDGET_CLASS (gtk_quartz_embed_widget_parent_class)->unrealize (widget);
}

static void
gtk_quartz_embed_widget_realize (GtkWidget *widget)
{
  GtkWindow *window = GTK_WINDOW (widget);
  GtkQuartzEmbedWidget *embed_widget = GTK_QUARTZ_EMBED_WIDGET (widget);
  GtkAllocation allocation;
  GdkWindow *gdk_window;
  GdkWindowAttr attributes;
  gint attributes_mask;

  gtk_widget_get_allocation (widget, &allocation);

  /* ensure widget tree is properly size allocated */
  if (allocation.x == -1 && allocation.y == -1 &&
      allocation.width == 1 && allocation.height == 1)
    {
      GtkRequisition requisition;
      GtkAllocation allocation = { 0, 0, 200, 200 };

      gtk_widget_get_preferred_size (widget, &requisition, NULL);
      if (requisition.width || requisition.height)
	{
	  /* non-empty window */
	  allocation.width = requisition.width;
	  allocation.height = requisition.height;
	}
      gtk_widget_size_allocate (widget, &allocation);
      
      _gtk_container_queue_resize (GTK_CONTAINER (widget));

      g_return_if_fail (!gtk_widget_get_realized (widget));
    }

  gtk_widget_set_realized (widget, TRUE);

  gtk_widget_get_allocation (widget, &allocation);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.title = (gchar *) gtk_window_get_title (window);
  _gtk_window_get_wmclass (window, &attributes.wmclass_name, &attributes.wmclass_class);
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;

  /* this isn't right - we should match our parent's visual/colormap.
   * though that will require handling "foreign" colormaps */
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= (GDK_EXPOSURE_MASK |
			    GDK_KEY_PRESS_MASK |
			    GDK_KEY_RELEASE_MASK |
			    GDK_ENTER_NOTIFY_MASK |
			    GDK_LEAVE_NOTIFY_MASK |
			    GDK_STRUCTURE_MASK |
			    GDK_FOCUS_CHANGE_MASK);

  attributes_mask = GDK_WA_VISUAL;
  attributes_mask |= (attributes.title ? GDK_WA_TITLE : 0);
  attributes_mask |= (attributes.wmclass_name ? GDK_WA_WMCLASS : 0);

  gdk_window = gdk_window_new (embed_widget->parent_window,
                               &attributes, attributes_mask);

  GTK_NOTE (MISC,
	    g_print ("gtk_quartz_embed_widget_realize() window: %p for widget: %p\n", gdk_window, widget));
  gtk_widget_set_window (widget, gdk_window);
  gtk_widget_register_window (widget, gdk_window);

  gtk_style_context_set_background (gtk_widget_get_style_context (widget),
                                    gdk_window);
}

static void
gtk_quartz_embed_widget_show (GtkWidget *widget)
{
  _gtk_widget_set_visible_flag (widget, TRUE);
  
  gtk_widget_realize (widget);
  gtk_container_check_resize (GTK_CONTAINER (widget));
  gtk_widget_map (widget);
}

static void
gtk_quartz_embed_widget_hide (GtkWidget *widget)
{
  _gtk_widget_set_visible_flag (widget, FALSE);
  gtk_widget_unmap (widget);
}

static void
gtk_quartz_embed_widget_map (GtkWidget *widget)
{
  GtkBin    *bin = GTK_BIN (widget);
  GtkWidget *child;

  gtk_widget_set_mapped (widget, TRUE);

  child = gtk_bin_get_child (bin);
  if (child &&
      gtk_widget_get_visible (child) &&
      !gtk_widget_get_mapped (child))
    gtk_widget_map (child);

  gdk_window_show (gtk_widget_get_window (widget));
}

static void
gtk_quartz_embed_widget_unmap (GtkWidget *widget)
{
  gtk_widget_set_mapped (widget, FALSE);
  gdk_window_hide (gtk_widget_get_window (widget));
}

static void
gtk_quartz_embed_widget_size_allocate (GtkWidget     *widget,
				      GtkAllocation *allocation)
{
  GtkBin    *bin = GTK_BIN (widget);
  GtkWidget *child;
  
  gtk_widget_set_allocation (widget, allocation);
  
  if (gtk_widget_get_realized (widget))
    gdk_window_move_resize (gtk_widget_get_window (widget),
			    allocation->x, allocation->y,
			    allocation->width, allocation->height);

  child = gtk_bin_get_child (bin);
  if (child && gtk_widget_get_visible (child))
    {
      GtkAllocation child_allocation;
      
      child_allocation.x = gtk_container_get_border_width (GTK_CONTAINER (widget));
      child_allocation.y = child_allocation.x;
      child_allocation.width =
	MAX (1, (gint)allocation->width - child_allocation.x * 2);
      child_allocation.height =
	MAX (1, (gint)allocation->height - child_allocation.y * 2);
      
      gtk_widget_size_allocate (child, &child_allocation);
    }
}

static void
gtk_quartz_embed_widget_check_resize (GtkContainer *container)
{
  GTK_CONTAINER_CLASS (bin_class)->check_resize (container);
}

static gboolean
gtk_quartz_embed_widget_focus (GtkWidget        *widget,
			      GtkDirectionType  direction)
{
  GtkBin *bin = GTK_BIN (widget);
  GtkQuartzEmbedWidget *embed_widget = GTK_QUARTZ_EMBED_WIDGET (widget);
  GtkWindow *window = GTK_WINDOW (widget);
  GtkContainer *container = GTK_CONTAINER (widget);
  GtkWidget *old_focus_child = gtk_container_get_focus_child (container);
  GtkWidget *parent;
  GtkWidget *child;

  /* We override GtkWindow's behavior, since we don't want wrapping here.
   */
  if (old_focus_child)
    {
      if (gtk_widget_child_focus (old_focus_child, direction))
	return TRUE;

      if (gtk_window_get_focus (window))
	{
	  /* Wrapped off the end, clear the focus setting for the toplevel */
	  parent = gtk_widget_get_parent (gtk_window_get_focus (window));
	  while (parent)
	    {
	      gtk_container_set_focus_child (GTK_CONTAINER (parent), NULL);
	      parent = gtk_widget_get_parent (GTK_WIDGET (parent));
	    }
	  
	  gtk_window_set_focus (GTK_WINDOW (container), NULL);
	}
    }
  else
    {
      /* Try to focus the first widget in the window */
      child = gtk_bin_get_child (bin);
      if (child && gtk_widget_child_focus (child, direction))
        return TRUE;
    }

  if (!gtk_container_get_focus_child (GTK_CONTAINER (window)))
    {
      int backwards = FALSE;

      if (direction == GTK_DIR_TAB_BACKWARD ||
	  direction == GTK_DIR_LEFT)
	backwards = TRUE;
      /* quartz-specific code? */      
    }

  return FALSE;
}

static void
gtk_quartz_embed_widget_set_focus (GtkWindow *window,
				  GtkWidget *focus)
{
  GTK_WINDOW_CLASS (gtk_quartz_embed_widget_parent_class)->set_focus (window, focus);

  gdk_window_focus (gtk_widget_get_window (GTK_WIDGET(window)), 0);
}
