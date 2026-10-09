#pragma once

#include <array>
#include <memory>

#include <pu/Plutonium>

#include <ui/BrowseTab.hpp>
#include <ui/DetailsTab.hpp>
#include <ui/QueueTab.hpp>
#include <ui/SettingsTab.hpp>

namespace pinx::ui {

class MainApplication;

enum class Tab : u8 { Browse = 0, Queue, Settings, Count };

class MainLayout : public pu::ui::Layout {
public:
    PU_SMART_CTOR(MainLayout)
    explicit MainLayout(MainApplication *app);
    void OnSettingsChanged();
    void ShowDetails(const pinx::catalog::CatalogItem &item);
    void HideDetails();

private:
    static constexpr s32 kW = 1920, kH = 1080;
    static constexpr s32 kTopBarH  = 100;
    static constexpr s32 kTabCount = static_cast<s32>(Tab::Count);
    static constexpr s32 kCardW = 360;
    static constexpr s32 kCardH = 360;
    static constexpr s32 kCardY = 440;

MainApplication *app_;
    Tab  current_tab_ = Tab::Browse;
    bool home_mode_   = true;
    bool details_mode_ = false;
    s32  home_sel_    = 0;

    pu::ui::elm::Rectangle::Ref home_bg_;
    pu::ui::elm::Image::Ref     home_logo_;
    std::array<pu::ui::elm::Rectangle::Ref, kTabCount> card_bg_;
    std::array<pu::ui::elm::Image::Ref,     kTabCount> card_icon_img_;
    std::array<pu::ui::elm::TextBlock::Ref, kTabCount> card_label_;
    pu::ui::elm::TextBlock::Ref home_hint_tb_;
    pu::ui::elm::Rectangle::Ref content_topbar_;
    pu::ui::elm::TextBlock::Ref content_title_;
    pu::ui::elm::TextBlock::Ref content_back_hint_;
    std::unique_ptr<BrowseTab>   browse_;
    std::unique_ptr<QueueTab>    queue_;
    std::unique_ptr<SettingsTab> settings_;
    std::unique_ptr<DetailsTab>  details_;

    void BuildHome();
    void BuildContentHeader();
    void BuildTabs();
    void ShowHome();
    void EnterTab(Tab t);
    void BackToHome();
    void UpdateCardColors();
    void RefreshHints();
    void RefreshStrings();
    static s32 CardX(s32 idx);
    void OnInput(u64 kd, u64 ku, u64 kh, pu::ui::TouchPoint tp);
};

} // namespace pinx::ui

