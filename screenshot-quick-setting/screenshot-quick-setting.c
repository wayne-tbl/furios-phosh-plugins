/*
 * Copyright (C) 2026 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "screenshot-quick-setting.h"

#include <glib/gi18n.h>

/**
 * PhoshScreenshotQuickSetting:
 *
 * Take a screenshot from the drawer, after a delay.
 *
 * Tapping the tile folds the drawer away and starts the countdown. It does not
 * shoot immediately because a screenshot taken from the settings drawer is a
 * picture of the settings drawer, which is never what was wanted; the delay is
 * what buys time for the drawer to leave and for the user to bring up whatever
 * they meant to capture.
 *
 * How long that delay is comes from `screenshot-delay` in Settings, so the tile
 * has no status page and therefore no arrow: it does one thing when tapped.
 *
 * The timer deliberately does not live here. It belongs to the shell's
 * screenshot manager, because this widget is inside the very drawer that has
 * to close for the shot to be worth taking, and a timer owned by a widget that
 * is going away is a timer that may not fire. Both the fold and the countdown
 * are therefore one call over the shell's own D-Bus interface rather than
 * anything linked against it.
 *
 * There is no check for a screenshot backend: the shell takes screenshots
 * itself over wlr-screencopy, so if phosh is running the tile can work.
 */

#define SHELL_SCHEMA_ID "io.furios.phosh.shell"
#define SCREENSHOT_DELAY_KEY "screenshot-delay"

#define SCREENSHOT_BUS_NAME    "org.gnome.Shell.Screenshot"
#define SCREENSHOT_OBJECT_PATH "/org/gnome/Shell/Screenshot"
#define FURIOS_SHELL_INTERFACE "io.furios.Shell"

struct _PhoshScreenshotQuickSetting {
  PhoshQuickSetting parent;

  PhoshStatusIcon  *info;

  GSettings        *settings;
  GDBusConnection  *bus;
  GCancellable     *cancel;
};

G_DEFINE_TYPE (PhoshScreenshotQuickSetting,
               phosh_screenshot_quick_setting,
               PHOSH_TYPE_QUICK_SETTING);


static void
on_screenshot_delayed_done (GObject *source, GAsyncResult *res, gpointer user_data)
{
  g_autoptr (GVariant) ret = NULL;
  g_autoptr (GError) err = NULL;

  ret = g_dbus_connection_call_finish (G_DBUS_CONNECTION (source), res, &err);
  if (ret == NULL) {
    if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED))
      return;
    g_warning ("Failed to take a screenshot: %s", err->message);
  }
}


static void
on_bus_got (GObject *source, GAsyncResult *res, gpointer user_data)
{
  PhoshScreenshotQuickSetting *self;
  g_autoptr (GDBusConnection) bus = NULL;
  g_autoptr (GError) err = NULL;

  bus = g_bus_get_finish (res, &err);
  if (bus == NULL) {
    if (!g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED))
      g_warning ("Failed to connect to the session bus: %s", err->message);
    return;
  }

  self = PHOSH_SCREENSHOT_QUICK_SETTING (user_data);
  self->bus = g_steal_pointer (&bus);
}


static void
on_clicked (PhoshScreenshotQuickSetting *self)
{
  guint seconds;

  g_assert (PHOSH_IS_SCREENSHOT_QUICK_SETTING (self));

  if (self->bus == NULL) {
    g_warning ("No session bus, cannot take a screenshot");
    return;
  }

  seconds = g_settings_get_int (self->settings, SCREENSHOT_DELAY_KEY);

  /* The shell folds the drawer away itself as part of this call: the delay
   * only buys time for the drawer to leave, so nothing here should wait for
   * the user to close it themselves. */
  g_dbus_connection_call (self->bus,
                          SCREENSHOT_BUS_NAME,
                          SCREENSHOT_OBJECT_PATH,
                          FURIOS_SHELL_INTERFACE,
                          "ScreenshotDelayed",
                          g_variant_new ("(u)", seconds),
                          NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          self->cancel,
                          on_screenshot_delayed_done,
                          NULL);
}


static void
phosh_screenshot_quick_setting_finalize (GObject *object)
{
  PhoshScreenshotQuickSetting *self = PHOSH_SCREENSHOT_QUICK_SETTING (object);

  g_cancellable_cancel (self->cancel);
  g_clear_object (&self->cancel);
  g_clear_object (&self->bus);
  g_clear_object (&self->settings);

  G_OBJECT_CLASS (phosh_screenshot_quick_setting_parent_class)->finalize (object);
}


static void
phosh_screenshot_quick_setting_class_init (PhoshScreenshotQuickSettingClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = phosh_screenshot_quick_setting_finalize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/plugins/screenshot-quick-setting/qs.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshScreenshotQuickSetting, info);
  gtk_widget_class_bind_template_callback (widget_class, on_clicked);
}


static void
phosh_screenshot_quick_setting_init (PhoshScreenshotQuickSetting *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->settings = g_settings_new (SHELL_SCHEMA_ID);
  self->cancel = g_cancellable_new ();
  g_bus_get (G_BUS_TYPE_SESSION, self->cancel, on_bus_got, self);

  g_object_set (self->info,
                "icon-name", "screenshot-portrait-symbolic",
                "info", _("Screenshot"),
                NULL);
}
