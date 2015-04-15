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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GTK+ Team and others 1997-2006.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __GTK_QUARTZ_EMBED_WIDGET_H__
#define __GTK_QUARTZ_EMBED_WIDGET_H__


#include <gtk/gtkwindow.h>
#include <gtk/gtkquartz.h>


G_BEGIN_DECLS

#define GTK_TYPE_QUARTZ_EMBED_WIDGET            (gtk_quartz_embed_widget_get_type ())
#define GTK_QUARTZ_EMBED_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_QUARTZ_EMBED_WIDGET, GtkQuartzEmbedWidget))
#define GTK_QUARTZ_EMBED_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_QUARTZ_EMBED_WIDGET, GtkQuartzEmbedWidgetClass))
#define GTK_IS_QUARTZ_EMBED_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_QUARTZ_EMBED_WIDGET))
#define GTK_IS_QUARTZ_EMBED_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_QUARTZ_EMBED_WIDGET))
#define GTK_QUARTZ_EMBED_WIDGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_QUARTZ_EMBED_WIDGET, GtkQuartzEmbedWidgetClass))


typedef struct _GtkQuartzEmbedWidget        GtkQuartzEmbedWidget;
typedef struct _GtkQuartzEmbedWidgetClass   GtkQuartzEmbedWidgetClass;


struct _GtkQuartzEmbedWidget
{
  GtkWindow  window;
  GdkWindow *parent_window;
};

struct _GtkQuartzEmbedWidgetClass
{
  GtkWindowClass parent_class;

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};


GType      gtk_quartz_embed_widget_get_type (void) G_GNUC_CONST;
GtkWidget* _gtk_quartz_embed_widget_new     (void *parent);


G_END_DECLS

#endif /* __GTK_QUARTZ_EMBED_WIDGET_H__ */
