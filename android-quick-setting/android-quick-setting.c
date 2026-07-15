/*
 * Copyright (C) 2026 Bardia Moshiri
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Bardia Moshiri <bardia@furilabs.com>
 */

#include "android-quick-setting.h"

#include "quick-setting.h"
#include "status-icon.h"

#include <glib/gi18n.h>
#include <gio/gio.h>

#define ANDROMEDA_CONTAINER_DBUS_NAME          "io.furios.Andromeda.Container"
#define ANDROMEDA_CONTAINER_DBUS_PATH          "/ContainerManager"
#define ANDROMEDA_CONTAINER_DBUS_INTERFACE     "io.furios.Andromeda.ContainerManager"

/**
 * PhoshAndroidQuickSetting:
 *
 * Start or stop Android
 */
struct _PhoshAndroidQuickSetting {
  PhoshQuickSetting        parent;
  PhoshStatusIcon         *info;
  GDBusConnection         *connection;
  guint                    subscription_id;
};

G_DEFINE_TYPE (PhoshAndroidQuickSetting, phosh_android_quick_setting, PHOSH_TYPE_QUICK_SETTING);

static gboolean
run_command_async (const char *command)
{
  g_autoptr(GError) error = NULL;
  g_auto(GStrv) argv = NULL;

  if (!g_shell_parse_argv (command, NULL, &argv, &error)) {
    g_warning ("Failed to parse command: %s", error->message);
    return FALSE;
  }

  if (!g_spawn_async (NULL,    /* working directory */
                      argv,     /* argv */
                      NULL,     /* envp */
                      G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                      NULL,     /* child setup function */
                      NULL,     /* user data */
                      NULL,     /* child pid */
                      &error)) {  /* error */
    g_warning ("Failed to execute command: %s", error->message);
    return FALSE;
  }

  return TRUE;
}

static void
update_ui (PhoshAndroidQuickSetting *self, gboolean active)
{
  phosh_quick_setting_set_active (PHOSH_QUICK_SETTING (self), active);
}

static void
on_session_state_changed (GDBusConnection *connection,
                          const gchar     *sender_name,
                          const gchar     *object_path,
                          const gchar     *interface_name,
                          const gchar     *signal_name,
                          GVariant        *parameters,
                          gpointer         user_data)
{
  PhoshAndroidQuickSetting *self = PHOSH_ANDROID_QUICK_SETTING (user_data);
  const gchar *state = NULL;
  (void) connection;
  (void) sender_name;
  (void) object_path;
  (void) interface_name;
  (void) signal_name;

  g_variant_get (parameters, "(&s)", &state);
  g_debug ("Andromeda session state changed: %s", state);

  if (g_strcmp0 (state, "started") == 0)
    update_ui (self, TRUE);
  else if (g_strcmp0 (state, "stopped") == 0)
    update_ui (self, FALSE);
}

static void
on_get_session_finished (GObject      *source_object,
                         GAsyncResult *res,
                         gpointer      user_data)
{
  PhoshAndroidQuickSetting *self = PHOSH_ANDROID_QUICK_SETTING (user_data);
  g_autoptr(GError) error = NULL;
  g_autoptr(GVariant) result = NULL;
  g_autoptr(GVariant) session_dict = NULL;
  gboolean is_running = FALSE;

  result = g_dbus_connection_call_finish (G_DBUS_CONNECTION (source_object), res, &error);

  if (error) {
    g_debug ("Failed to get session state: %s", error->message);
    update_ui (self, FALSE);
    return;
  }

  g_variant_get (result, "(@a{ss})", &session_dict);

  if (g_variant_n_children (session_dict) == 0) {
    update_ui (self, FALSE);
    return;
  }

  GVariantIter iter;
  g_variant_iter_init (&iter, session_dict);
  gchar *key, *value;

  while (g_variant_iter_next (&iter, "{ss}", &key, &value)) {
    if (g_strcmp0 (key, "state") == 0 && g_strcmp0 (value, "RUNNING") == 0)
      is_running = TRUE;

    g_free (key);
    g_free (value);
    if (is_running)
      break;
  }

  update_ui (self, is_running);
}

static void
check_initial_state (PhoshAndroidQuickSetting *self)
{
  g_dbus_connection_call (self->connection,
                          ANDROMEDA_CONTAINER_DBUS_NAME,
                          ANDROMEDA_CONTAINER_DBUS_PATH,
                          ANDROMEDA_CONTAINER_DBUS_INTERFACE,
                          "GetSession",
                          NULL,
                          NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          on_get_session_finished,
                          self);
}

static void
on_clicked (PhoshAndroidQuickSetting *self)
{
  gboolean active = phosh_quick_setting_get_active (PHOSH_QUICK_SETTING (self));

  if (!active) {
    g_debug ("Starting Andromeda session");
    if (run_command_async ("andromeda session start"))
      update_ui (self, TRUE);
  } else {
    g_debug ("Stopping Andromeda session");
    if (run_command_async ("andromeda session stop"))
      update_ui (self, FALSE);
  }
}

static void
phosh_android_finalize (GObject *object)
{
  PhoshAndroidQuickSetting *self = PHOSH_ANDROID_QUICK_SETTING (object);

  if (self->subscription_id > 0 && self->connection) {
    g_dbus_connection_signal_unsubscribe (self->connection, self->subscription_id);
    self->subscription_id = 0;
  }

  g_clear_object (&self->connection);

  G_OBJECT_CLASS (phosh_android_quick_setting_parent_class)->finalize (object);
}

static void
phosh_android_quick_setting_class_init (PhoshAndroidQuickSettingClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = phosh_android_finalize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/mobi/phosh/plugins/android-quick-setting/qs.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshAndroidQuickSetting, info);
  gtk_widget_class_bind_template_callback (widget_class, on_clicked);
}

static void
phosh_android_quick_setting_init (PhoshAndroidQuickSetting *self)
{
  g_autoptr(GError) error = NULL;

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_icon_theme_add_resource_path (gtk_icon_theme_get_default (),
                                    "/mobi/phosh/plugins/android-quick-setting/icons");

  phosh_status_icon_set_info (self->info, _("Android"));
  phosh_status_icon_set_icon_name (self->info, "android-symbolic");

  update_ui (self, FALSE);

  self->connection = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, &error);
  if (error) {
    g_warning ("Failed to connect to system bus: %s", error->message);
    return;
  }

  self->subscription_id = g_dbus_connection_signal_subscribe (self->connection,
                                                              ANDROMEDA_CONTAINER_DBUS_NAME,
                                                              ANDROMEDA_CONTAINER_DBUS_INTERFACE,
                                                              "SessionStateChanged",
                                                              ANDROMEDA_CONTAINER_DBUS_PATH,
                                                              NULL,
                                                              G_DBUS_SIGNAL_FLAGS_NONE,
                                                              on_session_state_changed,
                                                              self,
                                                              NULL);

  check_initial_state (self);
}
