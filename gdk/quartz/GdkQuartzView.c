/* GdkQuartzView.m
 *
 * Copyright (C) 2005-2007 Imendio AB
 * Copyright (C) 2011 Hiroyuki Yamamoto
 * Copyright (C) 2015 Kirk A. Baker, Camera Bits, Inc.
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

#import "GdkQuartzView.h"
#include "gdkquartzwindow.h"
#include "gdkprivate-quartz.h"
#include "gdkquartz.h"

#include <gdk/gdkdisplayprivate.h>

#include "gdkscreen.h"
#include "gdkkeysyms.h"
#include "gdkdnd-quartz.h"
#include "gdkquartzdisplay.h"
#include "gdkquartzdevicemanager-core.h"

@implementation GdkQuartzView

-(id)initWithFrame: (NSRect)frameRect
{
  if ((self = [super initWithFrame: frameRect]))
    {
      markedRange = NSMakeRange (NSNotFound, 0);
      selectedRange = NSMakeRange (NSNotFound, 0);
      shouldTrackCursor = YES;
      cursorIsInside = NO;
      haveBeenAddedToWindow = NO;
    }
  return self;
}

-(BOOL)acceptsFirstResponder
{
  GDK_NOTE (EVENTS, g_print ("acceptsFirstResponder\n"));
  return YES;
}

-(BOOL)becomeFirstResponder
{
  GDK_NOTE (EVENTS, g_print ("becomeFirstResponder\n"));
  return YES;
}

-(BOOL)resignFirstResponder
{
  GDK_NOTE (EVENTS, g_print ("resignFirstResponder\n"));
  return YES;
}

- (BOOL)canBecomeKeyView
{
  GDK_NOTE (EVENTS, g_print ("canBecomeKeyView\n"));
  return YES;
}

-(void) postKeyEvent:(NSEvent *)theEvent
{
  GdkWindow * window = _gdk_find_toplevel_for_keyboard_event(theEvent);
  
  if (window != NULL)
    {
      GdkEventType type;

      type = _gdk_quartz_keys_event_type (theEvent);
      if (type == GDK_NOTHING)
        return;
      else if (type == GDK_KEY_PRESS || type == GDK_KEY_RELEASE)
        {
          GdkEvent  *event;
          GList *node;

          event = gdk_event_new (GDK_NOTHING);

          event->any.window = NULL;
          event->any.send_event = FALSE;

          node = _gdk_event_queue_append (_gdk_display, event);
          _gdk_quartz_fill_key_event (window, event, theEvent, type);
          if (event->any.window)
            g_object_ref (event->any.window);
          _gdk_windowing_got_event (_gdk_display, node, event, 0);
          gtk_main_iteration();
        }
    }
}

-(void) keyDown: (NSEvent *) theEvent
{
  GDK_NOTE (EVENTS, g_print ("keyDown\n"));

  [self interpretKeyEvents:[NSArray arrayWithObject: theEvent]];
  [self postKeyEvent:theEvent];
}

-(void) keyUp: (NSEvent *) theEvent
{
  GDK_NOTE (EVENTS, g_print ("keyUp\n"));
  [self postKeyEvent:theEvent];
}

-(void)flagsChanged: (NSEvent *) theEvent
{
}

-(NSUInteger)characterIndexForPoint: (NSPoint)aPoint
{
  GDK_NOTE (EVENTS, g_print ("characterIndexForPoint\n"));
  return 0;
}

-(NSRect)firstRectForCharacterRange: (NSRange)aRange actualRange: (NSRangePointer)actualRange
{
  GDK_NOTE (EVENTS, g_print ("firstRectForCharacterRange\n"));
  gint ns_x, ns_y;
  GdkRectangle *rect;

  rect = g_object_get_data (G_OBJECT (gdk_window), GIC_CURSOR_RECT);
  if (rect)
    {
      _gdk_quartz_window_gdk_xy_to_xy (rect->x, rect->y + rect->height,
				       &ns_x, &ns_y);

      return NSMakeRect (ns_x, ns_y, rect->width, rect->height);
    }
  else
    {
      return NSMakeRect (0, 0, 0, 0);
    }
}

-(NSArray *)validAttributesForMarkedText
{
  GDK_NOTE (EVENTS, g_print ("validAttributesForMarkedText\n"));
  return [NSArray arrayWithObjects: NSUnderlineStyleAttributeName, nil];
}

-(NSAttributedString *)attributedSubstringForProposedRange: (NSRange)aRange actualRange: (NSRangePointer)actualRange
{
  GDK_NOTE (EVENTS, g_print ("attributedSubstringForProposedRange\n"));
  return nil;
}

-(BOOL)hasMarkedText
{
  GDK_NOTE (EVENTS, g_print ("hasMarkedText\n"));
  return markedRange.location != NSNotFound && markedRange.length != 0;
}

-(NSRange)markedRange
{
  GDK_NOTE (EVENTS, g_print ("markedRange\n"));
  return markedRange;
}

-(NSRange)selectedRange
{
  GDK_NOTE (EVENTS, g_print ("selectedRange\n"));
  return selectedRange;
}

-(void)unmarkText
{
  GDK_NOTE (EVENTS, g_print ("unmarkText\n"));
  gchar *prev_str;
  markedRange = selectedRange = NSMakeRange (NSNotFound, 0);

  g_object_set_data_full (G_OBJECT (gdk_window), TIC_MARKED_TEXT, NULL, g_free);
}

-(void)setMarkedText: (id)aString selectedRange: (NSRange)newSelection replacementRange: (NSRange)replacementRange
{
  GDK_NOTE (EVENTS, g_print ("setMarkedText\n"));
  const char *str;
  gchar *prev_str;

  if (replacementRange.location == NSNotFound)
    {
      markedRange = NSMakeRange (newSelection.location, [aString length]);
      selectedRange = NSMakeRange (newSelection.location, newSelection.length);
    }
  else {
      markedRange = NSMakeRange (replacementRange.location, [aString length]);
      selectedRange = NSMakeRange (replacementRange.location + newSelection.location, newSelection.length);
    }

  if ([aString isKindOfClass: [NSAttributedString class]])
    {
      str = [[aString string] UTF8String];
    }
  else {
      str = [aString UTF8String];
    }

  g_object_set_data_full (G_OBJECT (gdk_window), TIC_MARKED_TEXT, g_strdup (str), g_free);
  g_object_set_data (G_OBJECT (gdk_window), TIC_SELECTED_POS,
		     GUINT_TO_POINTER (selectedRange.location));
  g_object_set_data (G_OBJECT (gdk_window), TIC_SELECTED_LEN,
		     GUINT_TO_POINTER (selectedRange.length));

  GDK_NOTE (EVENTS, g_print ("setMarkedText: set %s (%p, nsview %p): %s\n",
			     TIC_MARKED_TEXT, gdk_window, self,
			     str ? str : "(empty)"));

  /* handle text input changes by mouse events */
  if (!GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (gdk_window),
                                            TIC_IN_KEY_DOWN)))
    {
//      _gdk_quartz_synthesize_null_key_event(gdk_window);
    }
}

-(void)doCommandBySelector: (SEL)aSelector
{
  GDK_NOTE (EVENTS, g_print ("doCommandBySelector\n"));
  if ([self respondsToSelector: aSelector])
    [self performSelector: aSelector];
}

-(void)insertText: (id)aString replacementRange: (NSRange)replacementRange
{
  GDK_NOTE (EVENTS, g_print ("insertText\n"));
  const char *str;
  NSString *string;
  gchar *prev_str;

  if ([self hasMarkedText])
    [self unmarkText];

  if ([aString isKindOfClass: [NSAttributedString class]])
      string = [aString string];
  else
      string = aString;

  NSCharacterSet *ctrlChars = [NSCharacterSet controlCharacterSet];
  NSCharacterSet *wsnlChars = [NSCharacterSet whitespaceAndNewlineCharacterSet];
  if ([string rangeOfCharacterFromSet:ctrlChars].length &&
      [string rangeOfCharacterFromSet:wsnlChars].length == 0)
    {
      /* discard invalid text input with Chinese input methods */
      str = "";
      [self unmarkText];
      NSInputManager *currentInputManager = [NSInputManager currentInputManager];
      [currentInputManager markedTextAbandoned:self];
    }
  else
   {
      str = [string UTF8String];
   }

  g_object_set_data_full (G_OBJECT (gdk_window), TIC_INSERT_TEXT, g_strdup (str), g_free);
  GDK_NOTE (EVENTS, g_print ("insertText: set %s (%p, nsview %p): %s\n",
			     TIC_INSERT_TEXT, gdk_window, self,
			     str ? str : "(empty)"));
//  NSLog(@"insertText:%@", string);

  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_FILTERED));

  /* handle text input changes by mouse events */
  if (!GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (gdk_window),
                                            TIC_IN_KEY_DOWN)))
    {
//      _gdk_quartz_synthesize_null_key_event(gdk_window);
    }
}

-(void)deleteBackward: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("deleteBackward\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)deleteForward: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("deleteForward\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)deleteToBeginningOfLine: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("deleteToBeginningOfLine\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)deleteToEndOfLine: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("deleteToEndOfLine\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)deleteWordBackward: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("deleteWordBackward\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)deleteWordForward: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("deleteWordForward\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)insertBacktab: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("insertBacktab\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)insertNewline: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("insertNewline\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY, GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)insertTab: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("insertTab\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveBackward: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("moveBackward\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveBackwardAndModifySelection: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("moveBackwardAndModifySelection\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveDown: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("moveDown\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveDownAndModifySelection: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("moveDownAndModifySelection\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveForward: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("moveForward\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveForwardAndModifySelection: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("moveForwardAndModifySelection\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveLeft: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("moveLeft\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveLeftAndModifySelection: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("moveLeftAndModifySelection\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveRight: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("moveRight\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveRightAndModifySelection: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("moveRightAndModifySelection\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveToBeginningOfDocument: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("moveToBeginningOfDocument\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveToBeginningOfDocumentAndModifySelection: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("moveToBeginningOfDocumentAndModifySelection\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveToBeginningOfLine: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("moveToBeginningOfLine\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveToBeginningOfLineAndModifySelection: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("moveToBeginningOfLineAndModifySelection\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveToEndOfDocument: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("moveToEndOfDocument\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveToEndOfDocumentAndModifySelection: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("moveToEndOfDocumentAndModifySelection\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveToEndOfLine: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("moveToEndOfLine\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveToEndOfLineAndModifySelection: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("moveToEndOfLineAndModifySelection\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveUp: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("moveUp\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveUpAndModifySelection: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("moveUpAndModifySelection\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveWordBackward: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("moveWordBackward\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveWordBackwardAndModifySelection: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("moveWordBackwardAndModifySelection\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveWordForward: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("moveWordForward\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveWordForwardAndModifySelection: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("moveWordForwardAndModifySelection\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveWordLeft: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("moveWordLeft\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveWordLeftAndModifySelection: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("moveWordLeftAndModifySelection\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveWordRight: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("moveWordRight\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveWordRightAndModifySelection: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("moveWordRightAndModifySelection\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)pageDown: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("pageDown\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)pageDownAndModifySelection: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("pageDownAndModifySelection\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)pageUp: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("pageUp\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)pageUpAndModifySelection: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("pageUpAndModifySelection\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)selectAll: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("selectAll\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)selectLine: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("selectLine\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)selectWord: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("selectWord\n"));
  g_object_set_data (G_OBJECT (gdk_window), GIC_FILTER_KEY,
		     GUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)cut: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("cut\n"));
}

-(void)copy: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("copy\n"));
}

-(void)paste: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("paste\n"));
}

-(void)noop: (id)sender
{
  GDK_NOTE (EVENTS, g_print ("noop\n"));
}

/* --------------------------------------------------------------- */

-(void)dealloc
{
  [super dealloc];
}

-(void)setGdkWindow: (GdkWindow *)window
{
  gdk_window = window;
}

-(GdkWindow *)gdkWindow
{
  return gdk_window;
}

- (void)setShouldTrackCursor: (BOOL)flag;
{
  shouldTrackCursor = flag;
}

-(BOOL)isFlipped
{
  return YES;
}

-(BOOL)isOpaque
{
  if (GDK_WINDOW_DESTROYED (gdk_window))
    return YES;

  /* A view is opaque if its GdkWindow doesn't have the RGBA visual */
  return gdk_window_get_visual (gdk_window) !=
    gdk_screen_get_rgba_visual (_gdk_screen);
}

-(void)drawRect: (NSRect)rect
{
  GdkRectangle gdk_rect;
  GdkWindowImplQuartz *impl = GDK_WINDOW_IMPL_QUARTZ (gdk_window->impl);
  const NSRect *drawn_rects;
  NSInteger count;
  int i;
  cairo_region_t *region;

//  GDK_NOTE (DRAW, g_print ("GdkQuartzView drawRect:\n"));

  if (GDK_WINDOW_DESTROYED (gdk_window))
    return;

  if (! (gdk_window->event_mask & GDK_EXPOSURE_MASK))
    return;

  if (NSEqualRects (rect, NSZeroRect))
    return;

  if (!GDK_WINDOW_IS_MAPPED (gdk_window))
    {
      /* If the window is not yet mapped, clip_region_with_children
       * will be empty causing the usual code below to draw nothing.
       * To not see garbage on the screen, we draw an aesthetic color
       * here. The garbage would be visible if any widget enabled
       * the NSView's CALayer in order to add sublayers for custom
       * native rendering.
       */
      [NSGraphicsContext saveGraphicsState];

      [[NSColor windowBackgroundColor] setFill];
      [NSBezierPath fillRect: rect];

      [NSGraphicsContext restoreGraphicsState];

      return;
    }

  /* Clear our own bookkeeping of regions that need display */
  if (impl->needs_display_region)
    {
      cairo_region_destroy (impl->needs_display_region);
      impl->needs_display_region = NULL;
    }

  [self getRectsBeingDrawn: &drawn_rects count: &count];
  region = cairo_region_create ();

  for (i = 0; i < count; i++)
    {
      gdk_rect.x = drawn_rects[i].origin.x;
      gdk_rect.y = drawn_rects[i].origin.y;
      gdk_rect.width = drawn_rects[i].size.width;
      gdk_rect.height = drawn_rects[i].size.height;

      cairo_region_union_rectangle (region, &gdk_rect);
    }

  impl->in_paint_rect_count++;
  _gdk_window_process_updates_recurse (gdk_window, region);
  impl->in_paint_rect_count--;

  cairo_region_destroy (region);

  if (needsInvalidateShadow)
    {
      [[self window] invalidateShadow];
      needsInvalidateShadow = NO;
    }
}

-(void)setNeedsInvalidateShadow: (BOOL)invalidate
{
  needsInvalidateShadow = invalidate;
}

extern void
fill_crossing_event (GdkWindow       *toplevel,
                     GdkEvent        *event,
                     NSEvent         *nsevent,
                     gint             x,
                     gint             y,
                     gint             x_root,
                     gint             y_root,
                     GdkEventType     event_type,
                     GdkCrossingMode  mode,
                     GdkNotifyType    detail);

static void
queue_tracking_event (GdkWindow * gdk_window, NSEvent * nsevent, NSPoint local_point, NSPoint screen_point, gboolean entered)
{
  GdkEvent *event = NULL;
  GList *node = NULL;
  gint   x, y, x_root, y_root;

  if (_gdk_display == NULL)
      return;

  _gdk_quartz_window_nspoint_to_gdk_xy (screen_point, &x_root, &y_root);

  x = local_point.x;
  y = gdk_window->height - local_point.y;

  event = gdk_event_new (GDK_NOTHING);

  node = _gdk_event_queue_append (_gdk_display, event);

  fill_crossing_event (gdk_window, event, nsevent,
                       x, y,
                       x_root, y_root,
                       entered? GDK_ENTER_NOTIFY : GDK_LEAVE_NOTIFY,
                       GDK_CROSSING_NORMAL,
                       GDK_NOTIFY_NONLINEAR);

  if (event->any.window)
    g_object_ref (event->any.window);
  if (((event->any.type == GDK_ENTER_NOTIFY) ||
       (event->any.type == GDK_LEAVE_NOTIFY)) &&
       (event->crossing.subwindow != NULL))
    g_object_ref (event->crossing.subwindow);
  GDK_NOTE (EVENTS, g_print ("sent %s\n", entered? "GDK_ENTER_NOTIFY" : "GDK_LEAVE_NOTIFY"));
  _gdk_windowing_got_event (_gdk_display, node, event, 0);
}

GdkWindow *
get_grab_toplevel (GdkWindow * toplevel, GdkEventMask event_mask)
{
  GdkDeviceGrabInfo *grab;
  GdkDisplay *display = gdk_window_get_display (toplevel);

  if (display == NULL)
    return NULL;

  /* From the docs for XGrabPointer:
   *
   * If owner_events is true and if a generated pointer event
   * would normally be reported to this client, it is reported
   * as usual. Otherwise, the event is reported with respect to
   * the grab_window and is reported only if selected by
   * event_mask. For either value of owner_events, unreported
   * events are discarded.
   */
  grab = _gdk_display_get_last_device_grab (display,
                                            display->core_pointer);
  if (grab != NULL)
    {
      /* check the grab window. */
      GdkWindow *grab_toplevel;

      grab_toplevel = gdk_window_get_effective_toplevel (grab->window);

      return grab_toplevel;
    }
  return NULL;
}

- (void)trackMouseMovement:(NSPoint)screen_pt event:(NSEvent *)nsevent hitView:(NSView *)hit_view
{
  NSWindow * win = [self window];
  NSPoint local_pt;
  NSRect boundsRect = [self bounds];
  BOOL in_view = NO;
  BOOL entered = NO, exited = NO;
  GdkWindow * grab_window = NULL;
  GdkEventMask event_mask = 0;

  if ((!shouldTrackCursor) || (win == nil))
    return;

  /* convert screen coordinates to window coordinates */
  local_pt = [win convertScreenToBase:screen_pt];
  /* convert window coordinates to view coordinates: */
  local_pt = [self convertPoint:local_pt fromView:nil];
  /* hit test local point */
  in_view = NSPointInRect(local_pt, boundsRect);
#if 1
  GDK_NOTE (EVENTS, g_print ("trackMouseMovement: window:%p (%d,%d) [%d, %d, %d, %d]\n", gdk_window,
                             (int)local_pt.x, (int)local_pt.y,
                             (int)boundsRect.origin.x, (int)boundsRect.origin.y, (int)boundsRect.size.width, (int)boundsRect.size.height));
#endif
  if (in_view != cursorIsInside)
    {
      entered = in_view;
      exited = !in_view;
    }
  cursorIsInside = in_view && self == hit_view;

  if (entered)
    {
      if ((!cursorIsInside) || !(gdk_window->event_mask & GDK_ENTER_NOTIFY_MASK))
        {
//          GDK_NOTE (EVENTS, g_print ("trackMouseMovement: entered but not inside for window:%p self:%p hit_view:%p\n", gdk_window, self, hit_view));
          return;
        }
      event_mask = GDK_ENTER_NOTIFY_MASK;
    }
  else if (exited)
    {
      if (cursorIsInside || !(gdk_window->event_mask & GDK_LEAVE_NOTIFY_MASK))
        {
//          GDK_NOTE (EVENTS, g_print ("trackMouseMovement: exited but not outside for window:%p self:%p hit_view:%p\n", gdk_window, self, hit_view));
          return;
        }
      event_mask = GDK_LEAVE_NOTIFY_MASK;
    }

  if (!entered && !exited)
    {
//      GDK_NOTE (EVENTS, g_print ("trackMouseMovement: neither entered or exited, inside:%d in_view:%d for window:%p self:%p hit_view:%p\n", cursorIsInside, in_view, gdk_window, self, hit_view));
      return;
    }

//  GDK_NOTE (EVENTS, g_print ("trackMouseMovement: status changed enter:%d exit:%d inside:%d for window:%p self:%p hit_view:%p ", entered, exited, cursorIsInside, gdk_window, self, hit_view));

  /* check for grabs first, if grab window is active and our window
     isn't the grab window, then don't notify Gdk. */
  grab_window = get_grab_toplevel(gdk_window, event_mask);

  if (grab_window != NULL)
    {
      if (grab_window != gdk_window)
        {
//          GDK_NOTE (EVENTS, g_print ("grab in place, grab_window:%p != gdk_window\n", grab_window));
          return;  // don't notify
        }
      else
        {
//          GDK_NOTE (EVENTS, g_print ("grab in place, grab_window == gdk_window "));
        }
    }
  else
    {
//      GDK_NOTE (EVENTS, g_print ("no grab in place "));
    }
  queue_tracking_event(gdk_window, nsevent, local_pt, screen_pt, in_view);
}

-(BOOL)shouldRespondToWindowNotifications
{
  if ([[self window] isKindOfClass:[GdkQuartzNSWindow class]] &&
      [[self window] contentView] == self)
    return NO;
  return YES;
}

-(void)viewDidMoveToWindow
{
  GdkWindowImplQuartz *impl = GDK_WINDOW_IMPL_QUARTZ (gdk_window->impl);
  impl->toplevel = [self window];	/* nil is OK here, if the view doesn't have a window
  									   then its impl shouldn't either. */

  if (![self window]) /* We have no top-level window at this time */
    return;

  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(windowDidBecomeKey:)
                                               name:NSWindowDidBecomeKeyNotification
                                             object:[self window]];
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(windowDidResignKey:)
                                               name:NSWindowDidResignKeyNotification
                                             object:[self window]];
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(windowDidBecomeMain:)
                                               name:NSWindowDidBecomeMainNotification
                                             object:[self window]];
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(windowDidResignMain:)
                                               name:NSWindowDidResignMainNotification
                                             object:[self window]];
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(windowDidMove:)
                                               name:NSWindowDidMoveNotification
                                             object:[self window]];
  if ((!haveBeenAddedToWindow) && [self shouldRespondToWindowNotifications])
    {
      NSApplication * nsApp = [NSApplication sharedApplication];

      if ([self window] == [nsApp mainWindow])
        [self windowDidBecomeMain:nil];
      else
        [self windowDidResignMain:nil];

      if ([self window] == [nsApp keyWindow])
        [self windowDidBecomeKey:nil];
      else
        [self windowDidResignKey:nil];
      [self windowDidMove:nil];
    }
  
  haveBeenAddedToWindow = YES;
}

-(void)windowDidBecomeKey:(NSNotification *)notification
{
  if ([self shouldRespondToWindowNotifications])
    {
      GDK_NOTE (EVENTS, g_print ("windowDidBecomeKey %p, gdk_window:%p\n", self, gdk_window));
      _gdk_quartz_events_update_focus_window (gdk_window, TRUE);
    }
}

-(void)windowDidResignKey:(NSNotification *)notification
{
  if ([self shouldRespondToWindowNotifications])
    {
      GDK_NOTE (EVENTS, g_print ("windowDidResignKey %p, gdk_window:%p\n", self, gdk_window));
      _gdk_quartz_events_update_focus_window (gdk_window, FALSE);
    }
}

-(void)windowDidBecomeMain:(NSNotification *)notification
{
  if (![self shouldRespondToWindowNotifications])
    return;
  if (![[self window] isVisible])
    {
      /* Note: This is a hack needed because for unknown reasons, hidden
       * windows get shown when clicking the dock icon when the application
       * is not already active.
       */
      [[self window] orderOut:nil];
      return;
    }

  GDK_NOTE (EVENTS, g_print ("windowDidBecomeMain %p, gdk_window:%p\n", self, gdk_window));
  _gdk_quartz_window_did_become_main (gdk_window);
}

-(void)windowDidResignMain:(NSNotification *)notification
{
  if ([self shouldRespondToWindowNotifications])
    {
      GDK_NOTE (EVENTS, g_print ("windowDidBecomeMain %p, gdk_window:%p\n", self, gdk_window));
      _gdk_quartz_window_did_resign_main (gdk_window);
    }
}

-(void)windowDidMove:(NSNotification *)aNotification
{
  GdkWindow *window = gdk_window;
  GdkEvent *event;

  if (![self shouldRespondToWindowNotifications])
    return;

  GDK_NOTE (EVENTS, g_print ("windowDidMove %p, gdk_window:%p\n", self, gdk_window));
  _gdk_quartz_window_update_position (window);

  /* Synthesize a configure event */
  event = gdk_event_new (GDK_CONFIGURE);
  event->configure.window = g_object_ref (window);
  event->configure.x = window->x;
  event->configure.y = window->y;
  event->configure.width = window->width;
  event->configure.height = window->height;

  _gdk_event_queue_append (gdk_display_get_default (), event);
}

-(void)viewWillMoveToWindow: (NSWindow *)newWindow
{
  haveBeenAddedToWindow = NO;
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

-(void)setFrame: (NSRect)frame
{
  GdkWindow *window = gdk_window;
  GdkEvent *event;

  [super setFrame: frame];

  _gdk_quartz_window_update_position (window);
  _gdk_window_update_size (window);

  /* Synthesize a configure event */
  event = gdk_event_new (GDK_CONFIGURE);
  event->configure.window = g_object_ref (window);
  event->configure.x = window->x;
  event->configure.y = window->y;
  event->configure.width = window->width;
  event->configure.height = window->height;

  _gdk_event_queue_append (gdk_display_get_default (), event);
}

- (void)adjustDragLocation:(NSPoint *)point
{
  /* Fix-up coordinates of <point> which are in GDK screen coordinates.
     Make them into window-relative coordinates. */
  gint    ns_x, ns_y, gdk_x, gdk_y;
  NSPoint scrn_point, win_point;

  gdk_x = point->x;
  gdk_y = point->y;

  _gdk_quartz_window_gdk_xy_to_xy(gdk_x, gdk_y, &ns_x, &ns_y);
  scrn_point.x = ns_x;
  scrn_point.y = ns_y;
  win_point = [[self window] convertScreenToBase:scrn_point];
  point->x = win_point.x;
  point->y = win_point.y;
}


static GdkDragContext *current_context = NULL;

static GdkDragAction
drag_operation_to_drag_action (NSDragOperation operation)
{
  GdkDragAction result = 0;

  /* GDK and Quartz drag operations do not map 1:1.
   * This mapping represents about the best that we
   * can come up.
   *
   * Note that NSDragOperationPrivate and GDK_ACTION_PRIVATE
   * have almost opposite meanings: the GDK one means that the
   * destination is solely responsible for the action; the Quartz
   * one means that the source and destination will agree
   * privately on the action. NSOperationGeneric is close in meaning
   * to GDK_ACTION_PRIVATE but there is a problem: it will be
   * sent for any ordinary drag, and likely not understood
   * by any intra-widget drag (since the source & dest are the
   * same).
   */

  if (operation & NSDragOperationGeneric)
    result |= GDK_ACTION_MOVE;
  if (operation & NSDragOperationCopy)
    result |= GDK_ACTION_COPY;
  if (operation & NSDragOperationMove)
    result |= GDK_ACTION_MOVE;
  if (operation & NSDragOperationLink)
    result |= GDK_ACTION_LINK;

  return result;
}

static NSDragOperation
drag_action_to_drag_operation (GdkDragAction action)
{
  NSDragOperation result = 0;

  if (action & GDK_ACTION_COPY)
    result |= NSDragOperationCopy;
  if (action & GDK_ACTION_LINK)
    result |= NSDragOperationLink;
  if (action & GDK_ACTION_MOVE)
    result |= NSDragOperationMove;

  return result;
}

static void
update_context_from_dragging_info (id <NSDraggingInfo> sender)
{
  g_assert (current_context != NULL);

  GDK_QUARTZ_DRAG_CONTEXT (current_context)->dragging_info = sender;
  current_context->suggested_action = drag_operation_to_drag_action ([sender draggingSourceOperationMask]);
  current_context->actions = current_context->suggested_action;
}

/* NSDraggingDestination protocol implementation start */

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender
{
//NSLog(@"draggingEntered:%@", sender);
  GdkDeviceManager *device_manager;
  GdkEvent *event;
  GdkWindow *window;

  if (current_context)
    g_object_unref (current_context);

  current_context = g_object_new (GDK_TYPE_QUARTZ_DRAG_CONTEXT, NULL);
  update_context_from_dragging_info (sender);

  window = [self gdkWindow];

  device_manager = gdk_display_get_device_manager (gdk_display_get_default ());
  gdk_drag_context_set_device (current_context,
                               gdk_device_manager_get_client_pointer (device_manager));

  event = gdk_event_new (GDK_DRAG_ENTER);
  event->dnd.window = g_object_ref (window);
  event->dnd.send_event = FALSE;
  event->dnd.context = g_object_ref (current_context);
  event->dnd.time = GDK_CURRENT_TIME;

  gdk_event_set_device (event, gdk_drag_context_get_device (current_context));

  _gdk_event_emit (event);

  gdk_event_free (event);

  return NSDragOperationNone;
}

- (void)draggingEnded:(id <NSDraggingInfo>)sender
{
//NSLog(@"draggingEnded:%@", sender);
  /* leave a note for the source about what action was taken */
  if (_gdk_quartz_drag_source_context && current_context)
   _gdk_quartz_drag_source_context->action = current_context->action;

  if (current_context)
    g_object_unref (current_context);
  current_context = NULL;
}

- (void)draggingExited:(id <NSDraggingInfo>)sender
{
//NSLog(@"draggingExited:%@", sender);
  GdkEvent *event;

  event = gdk_event_new (GDK_DRAG_LEAVE);
  event->dnd.window = g_object_ref ([self gdkWindow]);
  event->dnd.send_event = FALSE;
  event->dnd.context = g_object_ref (current_context);
  event->dnd.time = GDK_CURRENT_TIME;

  gdk_event_set_device (event, gdk_drag_context_get_device (current_context));

  _gdk_event_emit (event);

  gdk_event_free (event);

  g_object_unref (current_context);
  current_context = NULL;
}

- (NSDragOperation)draggingUpdated:(id <NSDraggingInfo>)sender
{
//NSLog(@"draggingUpdated:%@", sender);
  NSPoint point = [sender draggingLocation];
  NSPoint screen_point = [[self window] convertBaseToScreen:point];
  GdkEvent *event;
  int gx, gy;
  NSDragOperation operation;

  update_context_from_dragging_info (sender);
  _gdk_quartz_window_nspoint_to_gdk_xy (screen_point, &gx, &gy);

  event = gdk_event_new (GDK_DRAG_MOTION);
  event->dnd.window = g_object_ref ([self gdkWindow]);
  event->dnd.send_event = FALSE;
  event->dnd.context = g_object_ref (current_context);
  event->dnd.time = GDK_CURRENT_TIME;
  event->dnd.x_root = gx;
  event->dnd.y_root = gy;

  gdk_event_set_device (event, gdk_drag_context_get_device (current_context));

  _gdk_event_emit (event);

  gdk_event_free (event);

  operation = drag_action_to_drag_operation (current_context->action);

//NSLog(@"draggingUpdated: operation is %lu", (unsigned long)operation);
  return operation;
}

- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender
{
//NSLog(@"performDragOperation:%@", sender);
  NSPoint point = [sender draggingLocation];
  NSPoint screen_point = [[self window] convertBaseToScreen:point];
  GdkEvent *event;
  int gy, gx;

  update_context_from_dragging_info (sender);
  _gdk_quartz_window_nspoint_to_gdk_xy (screen_point, &gx, &gy);

  event = gdk_event_new (GDK_DROP_START);
  event->dnd.window = g_object_ref ([self gdkWindow]);
  event->dnd.send_event = FALSE;
  event->dnd.context = g_object_ref (current_context);
  event->dnd.time = GDK_CURRENT_TIME;
  event->dnd.x_root = gx;
  event->dnd.y_root = gy;

  gdk_event_set_device (event, gdk_drag_context_get_device (current_context));

  _gdk_event_emit (event);

  gdk_event_free (event);

  g_object_unref (current_context);
  current_context = NULL;

  return YES;
}

- (BOOL)wantsPeriodicDraggingUpdates
{
  return NO;
}
/* NSDraggingDestination protocol implementation end */

/* NSDraggingSource protocol implementation start (10.6 SDK compatible) */
- (void)draggedImage:(NSImage *)image beganAt:(NSPoint)screenPoint
{
}

- (void)draggedImage:(NSImage *)image movedTo:(NSPoint)screenPoint
{
}

- (void)draggedImage:(NSImage *)image endedAt:(NSPoint)screenPoint operation:(NSDragOperation)operation;
{
  GdkEvent *event;

  g_assert (_gdk_quartz_drag_source_context != NULL);

  event = gdk_event_new (GDK_DROP_FINISHED);
  event->dnd.window = g_object_ref ([self gdkWindow]);
  event->dnd.send_event = FALSE;
  event->dnd.context = g_object_ref (_gdk_quartz_drag_source_context);

  gdk_event_set_device (event,
                        gdk_drag_context_get_device (_gdk_quartz_drag_source_context));

  _gdk_event_emit (event);

  gdk_event_free (event);

  g_object_unref (_gdk_quartz_drag_source_context);
  _gdk_quartz_drag_source_context = NULL;
}

- (BOOL)ignoreModifierKeysWhileDragging;
{
  return NO;
}
/* NSDraggingSource protocol implementation end */

@end
