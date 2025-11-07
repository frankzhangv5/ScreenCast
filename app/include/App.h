#pragma once

namespace UI
{
    constexpr int WINDOW_WIDTH = 360;
    constexpr int WINDOW_HEIGHT = 640;

    constexpr int SPLASH_WIDTH = 400;
    constexpr int SPLASH_HEIGHT = 300; // Golden ratio: 500 / 1.618 ≈ 309

    constexpr int TITLE_BAR_HEIGHT = 36;
    constexpr int STATUS_BAR_HEIGHT = 36;
} // namespace UI

namespace App
{
    constexpr const char* AUTHOR = APP_AUTHOR;
    constexpr const char* COPYRIGHT = "Copyright (c) 2025 " APP_AUTHOR;
    constexpr const char* BUILD_ID = APP_BUILD_ID;

    constexpr const char* HELP_URL = HOME_PAGE_URL "/blob/main/docs/quickstart.md";
    constexpr const char* UPDATE_URL = HOME_PAGE_URL "/raw/main/update/version.json";
    constexpr const char* DRIVER_DOWNLOAD_URL = DRIVER_REPO_URL;
    constexpr const char* DONATION_URL = HOME_PAGE_URL "/blob/main/docs/donation.md";
} // namespace App