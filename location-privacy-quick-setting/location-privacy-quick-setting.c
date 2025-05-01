/*
 * Copyright (C) 2025 Bardia Moshiri
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Bardia Moshiri <bardia@furilabs.com>
 */

#include "location-privacy-quick-setting.h"

#include "quick-setting.h"
#include "status-icon.h"

#include <glib/gi18n.h>
#include <gio/gio.h>

/**
 * PhoshLocationPrivacyQuickSetting:
 *
 * Allow or disallow access to location
 */
struct _PhoshLocationPrivacyQuickSetting {
  PhoshQuickSetting        parent;

  GSettings               *settings;
  PhoshStatusIcon         *info;
};

G_DEFINE_TYPE (PhoshLocationPrivacyQuickSetting, phosh_location_privacy_quick_setting, PHOSH_TYPE_QUICK_SETTING);

static void
update_location_privacy_status (PhoshLocationPrivacyQuickSetting *self)
{
  gboolean setting_value = g_settings_get_boolean (self->settings, "enabled");

  if (!setting_value) {
    phosh_status_icon_set_icon_name (self->info, "location-services-disabled-symbolic");
    phosh_status_icon_set_info (self->info, _("Disallow Location"));
    phosh_quick_setting_set_active (PHOSH_QUICK_SETTING (self), FALSE);
  } else {
    phosh_status_icon_set_icon_name (self->info, "location-services-active-symbolic");
    phosh_status_icon_set_info (self->info, _("Allow Location"));
    phosh_quick_setting_set_active (PHOSH_QUICK_SETTING (self), TRUE);
  }
}

static void
on_clicked (PhoshLocationPrivacyQuickSetting *self)
{
  gboolean active = phosh_quick_setting_get_active (PHOSH_QUICK_SETTING (self));

  if (!active)
    g_settings_set_boolean (self->settings, "enabled", TRUE);
  else
    g_settings_set_boolean (self->settings, "enabled", FALSE);
}

static void
on_settings_changed (PhoshLocationPrivacyQuickSetting *self,
                     gchar                            *key,
                     GSettings                        *settings)
{
  (void) settings;

  if (g_strcmp0 (key, "enabled") == 0)
    update_location_privacy_status (self);
}

static void
phosh_location_privacy_finalize (GObject *object)
{
  PhoshLocationPrivacyQuickSetting *self = PHOSH_LOCATION_PRIVACY_QUICK_SETTING (object);

  g_clear_object (&self->settings);

  G_OBJECT_CLASS (phosh_location_privacy_quick_setting_parent_class)->finalize (object);
}

static void
phosh_location_privacy_quick_setting_class_init (PhoshLocationPrivacyQuickSettingClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = phosh_location_privacy_finalize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/plugins/location-privacy-quick-setting/qs.ui");

  gtk_widget_class_bind_template_child (widget_class, PhoshLocationPrivacyQuickSetting, info);

  gtk_widget_class_bind_template_callback (widget_class, on_clicked);
}

static void
phosh_location_privacy_quick_setting_init (PhoshLocationPrivacyQuickSetting *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->settings = g_settings_new ("org.gnome.system.location");

  update_location_privacy_status (self);

  g_signal_connect_swapped (self->settings, "changed::enabled",
                            (GCallback) on_settings_changed, self);
}
