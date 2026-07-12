/*
 * Copyright (C) 2026 Jeff Clemmons
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Jeff Clemmons <jeff@antisocialparadise.net>
 */

#pragma once

#include <status-icon.h>

G_BEGIN_DECLS

#define PHOSH_TYPE_WEATHER_STATUS_ICON phosh_weather_status_icon_get_type ()

G_DECLARE_FINAL_TYPE (PhoshWeatherStatusIcon,
                      phosh_weather_status_icon,
                      PHOSH, WEATHER_STATUS_ICON, PhoshStatusIcon)

G_END_DECLS
