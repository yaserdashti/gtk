/*
 * gdkmonitor.c
 *
 * Copyright 2016 Red Hat, Inc.
 *
 * Matthias Clasen <mclasen@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gdkmonitorprivate.h"
#include "gdkdisplay.h"
#include "gdkenumtypes.h"

/*
 * TODO:
 * - monitor type (laptop, projector, ...)
 * - consider vfuncs instead of baseclass storage
 * - provide a persistent id (if the backend allows)
 * - x11: hidpi scaling
 * - convert win32
 * - convert quartz
 */
enum {
  PROP_0,
  PROP_DISPLAY,
  PROP_MANUFACTURER,
  PROP_MODEL,
  PROP_SCALE_FACTOR,
  PROP_GEOMETRY,
  PROP_WORKAREA,
  PROP_WIDTH_MM,
  PROP_HEIGHT_MM,
  PROP_REFRESH_RATE,
  PROP_SUBPIXEL_LAYOUT,
  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { NULL, };

G_DEFINE_TYPE (GdkMonitor, gdk_monitor, G_TYPE_OBJECT)

static void
gdk_monitor_init (GdkMonitor *monitor)
{
  monitor->scale_factor = 1;
}

static void
gdk_monitor_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  GdkMonitor *monitor = GDK_MONITOR (object);

  switch (prop_id)
    {
    case PROP_DISPLAY:
      g_value_set_object (value, monitor->display);
      break;

    case PROP_MANUFACTURER:
      g_value_set_string (value, monitor->manufacturer);
      break;

    case PROP_MODEL:
      g_value_set_string (value, monitor->model);
      break;

    case PROP_SCALE_FACTOR:
      g_value_set_int (value, monitor->scale_factor);
      break;

    case PROP_GEOMETRY:
      g_value_set_boxed (value, &monitor->geometry);
      break;

    case PROP_WORKAREA:
      {
        GdkRectangle workarea;
        gdk_monitor_get_workarea (monitor, &workarea);
        g_value_set_boxed (value, &workarea);
      }
      break;

    case PROP_WIDTH_MM:
      g_value_set_int (value, monitor->width_mm);
      break;

    case PROP_HEIGHT_MM:
      g_value_set_int (value, monitor->height_mm);
      break;

    case PROP_REFRESH_RATE:
      g_value_set_boolean (value, monitor->refresh_rate);
      break;

    case PROP_SUBPIXEL_LAYOUT:
      g_value_set_enum (value, monitor->subpixel_layout);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gdk_monitor_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  GdkMonitor *monitor = GDK_MONITOR (object);

  switch (prop_id)
    {
    case PROP_DISPLAY:
      monitor->display = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gdk_monitor_finalize (GObject *object)
{
  GdkMonitor *monitor = GDK_MONITOR (object);

  g_free (monitor->manufacturer);
  g_free (monitor->model);

  G_OBJECT_CLASS (gdk_monitor_parent_class)->finalize (object);
}

static void
gdk_monitor_class_init (GdkMonitorClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = gdk_monitor_finalize;
  object_class->get_property = gdk_monitor_get_property;
  object_class->set_property = gdk_monitor_set_property;

  props[PROP_DISPLAY] =
    g_param_spec_object ("display",
                         "Display",
                         "The display of the monitor",
                         GDK_TYPE_DISPLAY,
                         G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY);
  props[PROP_MANUFACTURER] =
    g_param_spec_string ("manufacturer",
                         "Manufacturer",
                         "The manufacturer name",
                         NULL,
                         G_PARAM_READABLE);
  props[PROP_MODEL] =
    g_param_spec_string ("model",
                         "Model",
                         "The model name",
                         NULL,
                         G_PARAM_READABLE);
  props[PROP_SCALE_FACTOR] =
    g_param_spec_int ("scale-factor",
                      "Scale factor",
                      "The scale factor",
                      0, G_MAXINT,
                      1,
                      G_PARAM_READABLE);
  props[PROP_GEOMETRY] =
    g_param_spec_boxed ("geometry",
                        "Geometry",
                        "The geometry of the monitor",
                        GDK_TYPE_RECTANGLE,
                        G_PARAM_READABLE);
  props[PROP_WORKAREA] =
    g_param_spec_boxed ("workarea",
                        "Workarea",
                        "The workarea of the monitor",
                        GDK_TYPE_RECTANGLE,
                        G_PARAM_READABLE);
  props[PROP_WIDTH_MM] =
    g_param_spec_int ("width-mm",
                      "Physical width",
                      "The width of the monitor, in millimeters",
                      0, G_MAXINT,
                      0,
                      G_PARAM_READABLE);
  props[PROP_HEIGHT_MM] =
    g_param_spec_int ("height-mm",
                      "Physical height",
                      "The height of the monitor, in millimeters",
                      0, G_MAXINT,
                      0,
                      G_PARAM_READABLE);
  props[PROP_REFRESH_RATE] =
    g_param_spec_int ("refresh-rate",
                      "Refresh rate",
                      "The refresh rate, in millihertz",
                      0, G_MAXINT,
                      0,
                      G_PARAM_READABLE);
  props[PROP_SUBPIXEL_LAYOUT] =
    g_param_spec_enum ("subpixel-layout",
                       "Subpixel layout",
                       "The subpixel layout",
                       GDK_TYPE_SUBPIXEL_LAYOUT,
                       GDK_SUBPIXEL_LAYOUT_UNKNOWN,
                       G_PARAM_READABLE);

  g_object_class_install_properties (object_class, LAST_PROP, props);
}

GdkDisplay *
gdk_monitor_get_display (GdkMonitor *monitor)
{
  g_return_val_if_fail (GDK_IS_MONITOR (monitor), NULL);

  return monitor->display;
}

void
gdk_monitor_get_geometry (GdkMonitor   *monitor,
                          GdkRectangle *geometry)
{
  g_return_if_fail (GDK_IS_MONITOR (monitor));
  g_return_if_fail (geometry != NULL);

  *geometry = monitor->geometry;
}

void
gdk_monitor_get_workarea (GdkMonitor   *monitor,
                          GdkRectangle *workarea)
{
  g_return_if_fail (GDK_IS_MONITOR (monitor));
  g_return_if_fail (workarea != NULL);

  if (GDK_MONITOR_GET_CLASS (monitor)->get_workarea)
    GDK_MONITOR_GET_CLASS (monitor)->get_workarea (monitor, workarea);
  else
    *workarea = monitor->geometry;
}

int
gdk_monitor_get_width_mm (GdkMonitor *monitor)
{
  g_return_val_if_fail (GDK_IS_MONITOR (monitor), 0);

  return monitor->width_mm;
}

int
gdk_monitor_get_height_mm (GdkMonitor *monitor)
{
  g_return_val_if_fail (GDK_IS_MONITOR (monitor), 0);

  return monitor->height_mm;
}

const char *
gdk_monitor_get_manufacturer (GdkMonitor *monitor)
{
  g_return_val_if_fail (GDK_IS_MONITOR (monitor), NULL);

  return monitor->manufacturer;
}

const char *
gdk_monitor_get_model (GdkMonitor *monitor)
{
  g_return_val_if_fail (GDK_IS_MONITOR (monitor), NULL);

  return monitor->model;
}

int
gdk_monitor_get_scale_factor (GdkMonitor *monitor)
{
  g_return_val_if_fail (GDK_IS_MONITOR (monitor), 0);

  return monitor->scale_factor;
}

int
gdk_monitor_get_refresh_rate (GdkMonitor *monitor)
{
  g_return_val_if_fail (GDK_IS_MONITOR (monitor), 0);

  return monitor->refresh_rate;
}

GdkSubpixelLayout
gdk_monitor_get_subpixel_layout (GdkMonitor *monitor)
{
  g_return_val_if_fail (GDK_IS_MONITOR (monitor), GDK_SUBPIXEL_LAYOUT_UNKNOWN);

  return monitor->subpixel_layout;
}

gboolean
gdk_monitor_is_primary (GdkMonitor *monitor)
{
  g_return_val_if_fail (GDK_IS_MONITOR (monitor), FALSE);

  return monitor == gdk_display_get_primary_monitor (monitor->display);
}

GdkMonitor *
gdk_monitor_new (GdkDisplay *display)
{
  return GDK_MONITOR (g_object_new (GDK_TYPE_MONITOR,
                                    "display", display,
                                    NULL));
}

void
gdk_monitor_set_manufacturer (GdkMonitor *monitor,
                              const char *manufacturer)
{
  g_free (monitor->manufacturer);
  monitor->manufacturer = g_strdup (manufacturer);

  g_object_notify (G_OBJECT (monitor), "manufacturer");
}

void
gdk_monitor_set_model (GdkMonitor *monitor,
                       const char *model)
{
  g_free (monitor->model);
  monitor->model = g_strdup (model);

  g_object_notify (G_OBJECT (monitor), "model");
}

void
gdk_monitor_set_position (GdkMonitor *monitor,
                          int         x,
                          int         y)
{
  g_object_freeze_notify (G_OBJECT (monitor));

  if (monitor->geometry.x != x)
    {
      monitor->geometry.x = x;
      g_object_notify (G_OBJECT (monitor), "geometry");
    }

  if (monitor->geometry.y != y)
    {
      monitor->geometry.y = y;
      g_object_notify (G_OBJECT (monitor), "geometry");
    }

  g_object_thaw_notify (G_OBJECT (monitor));
}

void
gdk_monitor_set_size (GdkMonitor *monitor,
                      int         width,
                      int         height)
{
  g_object_freeze_notify (G_OBJECT (monitor));

  if (monitor->geometry.width != width)
    {
      monitor->geometry.width = width;
      g_object_notify (G_OBJECT (monitor), "geometry");
    }

  if (monitor->geometry.height != height)
    {
      monitor->geometry.height = height;
      g_object_notify (G_OBJECT (monitor), "geometry");
    }

  g_object_thaw_notify (G_OBJECT (monitor));
}

void
gdk_monitor_set_physical_size (GdkMonitor *monitor,
                               int         width_mm,
                               int         height_mm)
{
  g_object_freeze_notify (G_OBJECT (monitor));

  if (monitor->width_mm != width_mm)
    {
      monitor->width_mm = width_mm;
      g_object_notify (G_OBJECT (monitor), "width-mm");
    }

  if (monitor->height_mm != height_mm)
    {
      monitor->height_mm = height_mm;
      g_object_notify (G_OBJECT (monitor), "height-mm");
    }

  g_object_thaw_notify (G_OBJECT (monitor));
}

void
gdk_monitor_set_scale_factor (GdkMonitor *monitor,
                              int         scale_factor)
{
  if (monitor->scale_factor == scale_factor)
    return;

  monitor->scale_factor = scale_factor;

  g_object_notify (G_OBJECT (monitor), "scale-factor");
}

void
gdk_monitor_set_refresh_rate (GdkMonitor *monitor,
                              int         refresh_rate)
{
  if (monitor->refresh_rate == refresh_rate)
    return;

  monitor->refresh_rate = refresh_rate;

  g_object_notify (G_OBJECT (monitor), "refresh-rate");
}

void
gdk_monitor_set_subpixel_layout (GdkMonitor        *monitor,
                                 GdkSubpixelLayout  subpixel_layout)
{
  if (monitor->subpixel_layout == subpixel_layout)
    return;

  monitor->subpixel_layout = subpixel_layout;

  g_object_notify (G_OBJECT (monitor), "subpixel-layout");
}