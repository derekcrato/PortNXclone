#pragma once

#include <functional>
#include <string>
#include <vector>

#include <pu/Plutonium>

#include <catalog/CatalogItem.hpp>
#include <switch.h>

namespace pinx::ui {

class DetailsTab {
    public:
        DetailsTab(const pinx::catalog::CatalogItem &item,
                   std::function<void()> on_back,
                   std::function<void(const pinx::catalog::CatalogItem &)> on_install);
        ~DetailsTab();

        void AddElementsTo(pu::ui::Layout *layout);
        void Show();
        void Hide();
        void Poll();
        void TriggerInstall();
        void SetItem(const pinx::catalog::CatalogItem &item);

    private:
        pinx::catalog::CatalogItem item_;
        std::function<void()> on_back_;
        std::function<void(const pinx::catalog::CatalogItem &)> on_install_;

        bool visible_ = false;

        pu::ui::elm::Rectangle::Ref bg_;
        pu::ui::elm::Rectangle::Ref poster_placeholder_;
        pu::ui::elm::Image::Ref poster_img_;
        pu::ui::elm::Rectangle::Ref overlay_;
        pu::ui::elm::TextBlock::Ref title_;
        pu::ui::elm::TextBlock::Ref meta_;
        pu::ui::elm::TextBlock::Ref desc_;
        pu::ui::elm::TextBlock::Ref notes_;
        pu::ui::elm::TextBlock::Ref back_hint_;
        pu::ui::elm::TextBlock::Ref install_hint_;

        // Poster loading
        Thread poster_thread_{};
        bool poster_thread_running_ = false;
        bool poster_loaded_ = false;
        bool poster_ready_ = false;
        std::vector<uint8_t> poster_bytes_;
        pu::sdl2::TextureHandle::Ref poster_tex_;

        void BuildLayout();
        void RefreshStrings();
        void StartPosterLoad();
};

} // namespace pinx::ui