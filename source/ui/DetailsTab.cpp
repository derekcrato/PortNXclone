#include <ui/DetailsTab.hpp>
#include <app/I18n.hpp>

#include <switch.h>

namespace pinx::ui {

namespace {

constexpr pu::ui::Color kBg       = { 12, 12, 18, 255 };
constexpr pu::ui::Color kOverlay   = { 0, 0, 0, 190 };
constexpr pu::ui::Color kText      = { 230, 237, 243, 255 };
constexpr pu::ui::Color kMuted     = { 139, 148, 158, 255 };
constexpr pu::ui::Color kAccent    = { 31, 111, 235, 255 };
constexpr pu::ui::Color kSuccess   = { 63, 185, 80, 255 };

const std::string kFontLarge  = pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Large);
const std::string kFontMedium = pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium);
const std::string kFontSmall  = pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small);

std::string FormatDate(std::uint64_t d) {
    if (d == 0) return "";
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04llu-%02llu-%02llu",
        static_cast<unsigned long long>(d / 10000),
        static_cast<unsigned long long>((d / 100) % 100),
        static_cast<unsigned long long>(d % 100));
    return buf;
}

std::string FormatSize(std::uint64_t bytes) {
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    double v = static_cast<double>(bytes);
    int u = 0;
    while (v >= 1024.0 && u < 4) { v /= 1024.0; ++u; }
    char buf[32];
    if (u == 0) std::snprintf(buf, sizeof(buf), "%llu B", static_cast<unsigned long long>(bytes));
    else        std::snprintf(buf, sizeof(buf), "%.1f %s", v, units[u]);
    return buf;
}

}

