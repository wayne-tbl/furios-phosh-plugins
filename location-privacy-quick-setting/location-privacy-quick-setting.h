/*
 * Copyright (C) 2025 Bardia Moshiri
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Bardia Moshiri <bardia@furilabs.com>
 */

#include <gtk/gtk.h>

#include "quick-setting.h"

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_LOCATION_PRIVACY_QUICK_SETTING (phosh_location_privacy_quick_setting_get_type ())
G_DECLARE_FINAL_TYPE (PhoshLocationPrivacyQuickSetting,
                      phosh_location_privacy_quick_setting,
                      PHOSH, LOCATION_PRIVACY_QUICK_SETTING, PhoshQuickSetting)

G_END_DECLS
