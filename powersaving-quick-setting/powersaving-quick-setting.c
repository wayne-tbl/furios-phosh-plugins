/*
 * Copyright (C) 2026 Bardia Moshiri
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Bardia Moshiri <bardia@furilabs.com>
 */

#include "powersaving-quick-setting.h"

#include "quick-setting.h"
#include "status-icon.h"

#include <glib/gi18n.h>
#include <gio/gio.h>

#define PP_NAME "net.hadess.PowerProfiles"
#define PP_PATH "/net/hadess/PowerProfiles"
#define PP_IFACE "net.hadess.PowerProfiles"

/**
 * PhoshPowersavingQuickSetting:
 *
 * Enable or disable powersaving mode
 */
struct _PhoshPowersavingQuickSetting {
  PhoshQuickSetting        parent;

  PhoshStatusIcon         *info;
  GDBusProxy              *proxy;
};

G_DEFINE_TYPE (PhoshPowersavingQuickSetting, phosh_powersaving_quick_setting, PHOSH_TYPE_QUICK_SETTING);

static void
on_clicked (PhoshPowersavingQuickSetting *self)
{
  gboolean active = phosh_quick_setting_get_active (PHOSH_QUICK_SETTING (self));

  if (!active) {
    phosh_status_icon_set_icon_name (self->info, "battery-low-symbolic");
    phosh_status_icon_set_info (self->info, _("Powersave Enabled"));
    phosh_quick_setting_set_active (PHOSH_QUICK_SETTING (self), TRUE);

    g_dbus_proxy_call (self->proxy,
                       "org.freedesktop.DBus.Properties.Set",
                       g_variant_new ("(ssv)", PP_IFACE, "ActiveProfile", g_variant_new_string ("power-saver")),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL,
                       NULL,
                       NULL);
  } else {
    phosh_status_icon_set_icon_name (self->info, "battery-full-charged-symbolic");
    phosh_status_icon_set_info (self->info, _("Powersave Disabled"));
    phosh_quick_setting_set_active (PHOSH_QUICK_SETTING (self), FALSE);

    g_dbus_proxy_call (self->proxy,
                       "org.freedesktop.DBus.Properties.Set",
                       g_variant_new ("(ssv)", PP_IFACE, "ActiveProfile", g_variant_new_string ("balanced")),
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL,
                       NULL,
                       NULL);
  }
}

static void
phosh_powersaving_finalize (GObject *object)
{
  PhoshPowersavingQuickSetting *self = PHOSH_POWERSAVING_QUICK_SETTING (object);

  g_clear_object (&self->proxy);

  G_OBJECT_CLASS (phosh_powersaving_quick_setting_parent_class)->finalize (object);
}

static void
phosh_powersaving_quick_setting_class_init (PhoshPowersavingQuickSettingClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = phosh_powersaving_finalize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/plugins/powersaving-quick-setting/qs.ui");

  gtk_widget_class_bind_template_child (widget_class, PhoshPowersavingQuickSetting, info);

  gtk_widget_class_bind_template_callback (widget_class, on_clicked);
}

static void
phosh_powersaving_quick_setting_init (PhoshPowersavingQuickSetting *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                               G_DBUS_PROXY_FLAGS_NONE,
                                               NULL,
                                               PP_NAME,
                                               PP_PATH,
                                               PP_IFACE,
                                               NULL,
                                               NULL);

  phosh_status_icon_set_icon_name (self->info, "battery-full-charged-symbolic");
  phosh_status_icon_set_info (self->info, _("Powersave Disabled"));
  phosh_quick_setting_set_active (PHOSH_QUICK_SETTING (self), FALSE);
}
