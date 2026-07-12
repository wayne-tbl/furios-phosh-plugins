/*
 * Copyright (C) 2026 Jeff Clemmons
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Jeff Clemmons <jeff@antisocialparadise.net>
 */

#include "weather-status-icon.h"

#include <geoclue.h>
#include <libgweather/gweather.h>

#define REFRESH_INTERVAL_SECONDS (30 * 60)
#define GEOCLUE_DESKTOP_ID "sm.puri.Phosh.WeatherStatusIcon"
#define CONTACT_INFO "jeff@antisocialparadise.net"

/**
 * PhoshWeatherStatusIcon:
 *
 * Shows current weather conditions as a symbolic status icon, using
 * GeoClue for location and libgweather for conditions -- the same stack
 * GNOME Weather itself uses, rather than talking to a weather API directly.
 */

struct _PhoshWeatherStatusIcon {
  PhoshStatusIcon parent;

  GtkWidget    *temp_label;
  GClueSimple  *geoclue;
  GWeatherInfo *weather;
  GSettings    *gweather_settings;
  guint         refresh_id;
};

G_DEFINE_TYPE (PhoshWeatherStatusIcon, phosh_weather_status_icon, PHOSH_TYPE_STATUS_ICON)


/* gweather_temperature_unit_to_real(GWEATHER_TEMP_UNIT_DEFAULT) does *not*
 * read the user's org.gnome.GWeather4 "temperature-unit" preference -- it
 * resolves purely from locale (is_locale_metric() in gweather-info.c), so
 * relying on it left the display stuck on whatever Phosh's process locale
 * implies regardless of what the user picked in Settings/GNOME Weather.
 * Read the actual preference ourselves instead. */
static GWeatherTemperatureUnit
get_preferred_temp_unit (PhoshWeatherStatusIcon *self)
{
  g_autofree char *value = NULL;

  if (self->gweather_settings)
    value = g_settings_get_string (self->gweather_settings, "temperature-unit");

  if (g_strcmp0 (value, "kelvin") == 0)
    return GWEATHER_TEMP_UNIT_KELVIN;
  if (g_strcmp0 (value, "centigrade") == 0)
    return GWEATHER_TEMP_UNIT_CENTIGRADE;
  if (g_strcmp0 (value, "fahrenheit") == 0)
    return GWEATHER_TEMP_UNIT_FAHRENHEIT;

  /* "default"/"invalid"/schema missing: same locale fallback GWeather uses. */
  return gweather_temperature_unit_to_real (GWEATHER_TEMP_UNIT_DEFAULT);
}


/* gweather_info_get_temp() returns a formatted string with a decimal
 * (e.g. "77.5 °F") and no way to ask for whole degrees, so read the raw
 * value ourselves and round it via "%.0f" instead. */
static char *
format_temp_int (PhoshWeatherStatusIcon *self, GWeatherInfo *info)
{
  GWeatherTemperatureUnit unit = get_preferred_temp_unit (self);
  double value;
  const char *suffix;

  if (!gweather_info_get_value_temp (info, unit, &value))
    return NULL;

  switch (unit) {
  case GWEATHER_TEMP_UNIT_FAHRENHEIT:
    suffix = "°F";
    break;
  case GWEATHER_TEMP_UNIT_CENTIGRADE:
    suffix = "°C";
    break;
  case GWEATHER_TEMP_UNIT_KELVIN:
    suffix = "K";
    break;
  default:
    suffix = "°";
    break;
  }

  return g_strdup_printf ("%.0f %s", value, suffix);
}


static void
update_display (PhoshWeatherStatusIcon *self, GWeatherInfo *info)
{
  const char *icon_name;
  g_autofree char *temp = NULL;

  if (!gweather_info_is_valid (info)) {
    g_debug ("Weather info not yet valid");
    return;
  }

  icon_name = gweather_info_get_symbolic_icon_name (info);
  temp = format_temp_int (self, info);
  if (!temp)
    temp = gweather_info_get_temp (info);

  g_debug ("Weather updated: %s, %s", icon_name, temp);

  phosh_status_icon_set_icon_name (PHOSH_STATUS_ICON (self), icon_name);
  /* "info" only ever renders in the quick-settings drawer (see
   * quick-setting.c); PhoshTopPanel shows "extra-widget" instead, so the
   * temperature has to be a real label, not just this property. */
  phosh_status_icon_set_info (PHOSH_STATUS_ICON (self), temp);
  gtk_label_set_text (GTK_LABEL (self->temp_label), temp);
}


static void
on_weather_updated (GWeatherInfo *info, gpointer user_data)
{
  update_display (PHOSH_WEATHER_STATUS_ICON (user_data), info);
}


/* Re-render immediately if the user flips their unit preference while
 * already-fetched data is on screen -- no need to refetch over the network. */
static void
on_temp_unit_changed (GSettings *settings, const char *key, gpointer user_data)
{
  PhoshWeatherStatusIcon *self = PHOSH_WEATHER_STATUS_ICON (user_data);
  (void) settings;
  (void) key;

  if (self->weather)
    update_display (self, self->weather);
}


static void
update_weather_for_location (PhoshWeatherStatusIcon *self, GWeatherLocation *location)
{
  g_clear_object (&self->weather);

  self->weather = gweather_info_new (location);
  gweather_info_set_application_id (self->weather, GEOCLUE_DESKTOP_ID);
  gweather_info_set_contact_info (self->weather, CONTACT_INFO);
  gweather_info_set_enabled_providers (self->weather,
                                       GWEATHER_PROVIDER_METAR | GWEATHER_PROVIDER_MET_NO);
  g_signal_connect_object (self->weather, "updated",
                            G_CALLBACK (on_weather_updated), self, 0);
  gweather_info_update (self->weather);
}


static void
on_geoclue_location_changed (GClueSimple *simple, GParamSpec *pspec, gpointer user_data)
{
  PhoshWeatherStatusIcon *self = PHOSH_WEATHER_STATUS_ICON (user_data);
  GClueLocation *location;
  (void) pspec;
  /* get_world()/find_nearest_city() are (transfer full); gweather_info_new()
   * takes its own reference on the location it's given ((transfer none)
   * parameter), so we still own and must drop these. */
  g_autoptr (GWeatherLocation) world = NULL;
  g_autoptr (GWeatherLocation) city = NULL;
  double lat, lon;

  location = gclue_simple_get_location (simple);
  if (!location)
    return;

  lat = gclue_location_get_latitude (location);
  lon = gclue_location_get_longitude (location);

  world = gweather_location_get_world ();
  city = gweather_location_find_nearest_city (world, lat, lon);
  if (!city) {
    g_warning ("No weather station found near %f, %f", lat, lon);
    return;
  }

  g_debug ("GeoClue fix: %f, %f -> %s", lat, lon, gweather_location_get_name (city));
  update_weather_for_location (self, city);
}


/* Falls back to the location the user already set up in GNOME Weather
 * (org.gnome.Weather's saved "locations" list) so the icon shows real
 * conditions immediately, rather than waiting on GeoClue's WiFi/GPS fix --
 * which can take a while or fail outright indoors. GeoClue still supersedes
 * this once/if it produces an actual fix. */
static GWeatherLocation *
get_saved_location (void)
{
  GSettingsSchemaSource *source = g_settings_schema_source_get_default ();
  g_autoptr (GSettingsSchema) schema = NULL;
  g_autoptr (GSettings) settings = NULL;
  g_autoptr (GVariant) locations = NULL;
  g_autoptr (GVariant) first = NULL;
  g_autoptr (GWeatherLocation) world = NULL;

  if (!source)
    return NULL;

  schema = g_settings_schema_source_lookup (source, "org.gnome.Weather", TRUE);
  if (!schema)
    return NULL;

  settings = g_settings_new ("org.gnome.Weather");
  locations = g_settings_get_value (settings, "locations");
  if (g_variant_n_children (locations) == 0)
    return NULL;

  /* "locations" is type "av": each element is a boxed variant wrapping the
   * actual serialized (uv) location tuple, so it must be unboxed first. */
  first = g_variant_get_child_value (locations, 0);
  {
    g_autoptr (GVariant) unboxed = g_variant_get_variant (first);
    world = gweather_location_get_world ();
    return gweather_location_deserialize (world, unboxed);
  }
}


static void
on_geoclue_ready (GObject *source, GAsyncResult *result, gpointer user_data)
{
  PhoshWeatherStatusIcon *self = PHOSH_WEATHER_STATUS_ICON (user_data);
  g_autoptr (GError) error = NULL;
  (void) source;

  self->geoclue = gclue_simple_new_finish (result, &error);
  if (!self->geoclue) {
    g_warning ("Failed to start GeoClue: %s", error->message);
    return;
  }

  g_signal_connect_object (self->geoclue, "notify::location",
                            G_CALLBACK (on_geoclue_location_changed), self, 0);
  on_geoclue_location_changed (self->geoclue, NULL, self);
}


static gboolean
on_refresh_timeout (gpointer user_data)
{
  PhoshWeatherStatusIcon *self = PHOSH_WEATHER_STATUS_ICON (user_data);

  /* Conditions change over time even without a new location fix. */
  if (self->weather)
    gweather_info_update (self->weather);

  return G_SOURCE_CONTINUE;
}


static void
phosh_weather_status_icon_dispose (GObject *object)
{
  PhoshWeatherStatusIcon *self = PHOSH_WEATHER_STATUS_ICON (object);

  g_clear_handle_id (&self->refresh_id, g_source_remove);
  g_clear_object (&self->weather);
  g_clear_object (&self->geoclue);
  g_clear_object (&self->gweather_settings);

  G_OBJECT_CLASS (phosh_weather_status_icon_parent_class)->dispose (object);
}


static void
phosh_weather_status_icon_class_init (PhoshWeatherStatusIconClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = phosh_weather_status_icon_dispose;

  gtk_widget_class_set_template_from_resource (widget_class,
                                                "/mobi/phosh/plugins/weather-status-icon/si.ui");
  gtk_widget_class_bind_template_child (widget_class, PhoshWeatherStatusIcon, temp_label);
}


static void
phosh_weather_status_icon_init (PhoshWeatherStatusIcon *self)
{
  g_autoptr (GWeatherLocation) saved = NULL;
  GSettingsSchemaSource *source = g_settings_schema_source_get_default ();
  g_autoptr (GSettingsSchema) schema = NULL;

  gtk_widget_init_template (GTK_WIDGET (self));

  phosh_status_icon_set_icon_name (PHOSH_STATUS_ICON (self), "weather-severe-alert-symbolic");

  if (source)
    schema = g_settings_schema_source_lookup (source, "org.gnome.GWeather4", TRUE);
  if (schema) {
    self->gweather_settings = g_settings_new ("org.gnome.GWeather4");
    g_signal_connect_object (self->gweather_settings, "changed::temperature-unit",
                              G_CALLBACK (on_temp_unit_changed), self, 0);
  }

  saved = get_saved_location ();
  if (saved) {
    g_debug ("Using saved GNOME Weather location: %s", gweather_location_get_name (saved));
    update_weather_for_location (self, saved);
  }

  gclue_simple_new (GEOCLUE_DESKTOP_ID, GCLUE_ACCURACY_LEVEL_CITY, NULL,
                     on_geoclue_ready, self);

  self->refresh_id = g_timeout_add_seconds (REFRESH_INTERVAL_SECONDS, on_refresh_timeout, self);
}
