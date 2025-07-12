#pragma once

#include <QString>

namespace AppConfig
{
    // Default width and height of application window
    constexpr int WINDOW_WIDTH = 320;
    constexpr int WINDOW_HEIGHT = 640;

    constexpr int SPLASH_WIDTH = 400;
    constexpr int SPLASH_HEIGHT = 300; // Golden ratio: 500 / 1.618 ≈ 309

    constexpr int TITLE_BAR_WIDTH = WINDOW_WIDTH;
    constexpr int TITLE_BAR_HEIGHT = 36;

    constexpr int STATUS_BAR_WIDTH = WINDOW_WIDTH;
    constexpr int STATUS_BAR_HEIGHT = 36;

    constexpr int PAGE_WIDTH = WINDOW_WIDTH;
    constexpr int PAGE_HEIGHT = WINDOW_HEIGHT - TITLE_BAR_HEIGHT - STATUS_BAR_HEIGHT;

    const QString PRIMARY_COLOR = "#008D4E";        // Primary color
    const QString SECONDARY_COLOR = "#F5F5F5";      // Secondary color
    const QString TEXT_COLOR = "white";             // Main text color
    const QString BACKGROUND_COLOR = PRIMARY_COLOR; // Background color
    const QString ERROR_COLOR = "#FF0000";          // Error color
    const QString SUCCESS_COLOR = "#4CAF50";        // Success color
    const QString WARNING_COLOR = "#FF9800";        // Warning color
    const QString INFO_COLOR = "#2196F3";           // Info color
    const QString BORDER_COLOR = "#E0E0E0";         // Border color
    const QString DISABLED_COLOR = "#BDBDBD";       // Disabled state color
    const QString HOVER_COLOR = "#E8F5E9";          // Hover color
    const QString ACTIVE_COLOR = "#C8E6C9";         // Active (clicked) color
    const QString FOCUS_COLOR = "#C8E6C9";          // Focus color for input fields

    const QString HOME_PAGE_URL = "https://frankzhangv5/ScreenCast/ScreenCast";
    const QString HELP_URL = HOME_PAGE_URL + "/blob/master/docs/quickstart.md";
    const QString UPDATE_URL = HOME_PAGE_URL + "/raw/master/pack/update.json";
    const QString DRIVER_DOWNLOAD_URL = "https://frankzhangv5/ScreenCast/DeviceDriver";
    const QString DONATION_URL = HOME_PAGE_URL + "/blob/master/docs/donation.md";
} // namespace AppConfig
