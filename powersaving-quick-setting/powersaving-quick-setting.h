/*
 * Copyright (C) 2026 Bardia Moshiri
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Bardia Moshiri <bardia@furilabs.com>
 */

#include <gtk/gtk.h>

#include "quick-setting.h"

#pragma once

G_BEGIN_DECLS

#define PHOSH_TYPE_POWERSAVING_QUICK_SETTING (phosh_powersaving_quick_setting_get_type ())
G_DECLARE_FINAL_TYPE (PhoshPowersavingQuickSetting,
                      phosh_powersaving_quick_setting,
                      PHOSH, POWERSAVING_QUICK_SETTING, PhoshQuickSetting)

G_END_DECLS
