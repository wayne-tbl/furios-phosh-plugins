/*
 * Copyright (C) 2026 Bardia Moshiri
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Bardia Moshiri <bardia@furilabs.com>
 */

#include "microphone-privacy-quick-setting.h"

#include "quick-setting.h"
#include "status-icon.h"

#include <glib/gi18n.h>
#include <gio/gio.h>

/**
 * PhoshMicrophonePrivacyQuickSetting:
 *
 * Allow or disallow access to microphone
 */
struct _PhoshMicrophonePrivacyQuickSetting {
  PhoshQuickSetting        parent;

  GSettings               *settings;
  PhoshStatusIcon         *info;
};

G_DEFINE_TYPE (PhoshMicrophonePrivacyQuickSetting, phosh_microphone_privacy_quick_setting, PHOSH_TYPE_QUICK_SETTING);

static void
update_microphone_privacy_status (PhoshMicrophonePrivacyQuickSetting *self)
{
  gboolean setting_value = g_settings_get_boolean (self->settings, "disable-microphone");

  if (setting_value) {
    phosh_status_icon_set_icon_name (self->info, "microphone-hardware-disabled-symbolic");
    phosh_status_icon_set_info (self->info, _("Disallow Microphone"));
    phosh_quick_setting_set_active (PHOSH_QUICK_SETTING (self), FALSE);
  } else {
    phosh_status_icon_set_icon_name (self->info, "audio-input-microphone-symbolic");
    phosh_status_icon_set_info (self->info, _("Allow Microphone"));
    phosh_quick_setting_set_active (PHOSH_QUICK_SETTING (self), TRUE);
  }
}

static void
on_clicked (PhoshMicrophonePrivacyQuickSetting *self)
{
  gboolean active = phosh_quick_setting_get_active (PHOSH_QUICK_SETTING (self));

  if (active)
    g_settings_set_boolean (self->settings, "disable-microphone", TRUE);
  else
    g_settings_set_boolean (self->settings, "disable-microphone", FALSE);
}

static void
on_settings_changed (PhoshMicrophonePrivacyQuickSetting *self,
                     gchar                              *key,
                     GSettings                          *settings)
{
  (void) settings;

  if (g_strcmp0 (key, "disable-microphone") == 0)
    update_microphone_privacy_status (self);
}

static void
phosh_microphone_privacy_finalize (GObject *object)
{
  PhoshMicrophonePrivacyQuickSetting *self = PHOSH_MICROPHONE_PRIVACY_QUICK_SETTING (object);

  g_clear_object (&self->settings);

  G_OBJECT_CLASS (phosh_microphone_privacy_quick_setting_parent_class)->finalize (object);
}

static void
phosh_microphone_privacy_quick_setting_class_init (PhoshMicrophonePrivacyQuickSettingClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = phosh_microphone_privacy_finalize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/plugins/microphone-privacy-quick-setting/qs.ui");

  gtk_widget_class_bind_template_child (widget_class, PhoshMicrophonePrivacyQuickSetting, info);

  gtk_widget_class_bind_template_callback (widget_class, on_clicked);
}

static void
phosh_microphone_privacy_quick_setting_init (PhoshMicrophonePrivacyQuickSetting *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->settings = g_settings_new ("org.gnome.desktop.privacy");

  update_microphone_privacy_status (self);

  g_signal_connect_swapped (self->settings, "changed::disable-microphone",
                            (GCallback) on_settings_changed, self);
}
