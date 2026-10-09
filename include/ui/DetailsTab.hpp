#pragma once

#include <functional>
#include <string>

#include <pu/Plutonium>

#include <catalog\CatalogItem.hpp>

namespace pinx::ui {

class DetailsTab {
    public:
        DetailsTab(const pinx::catalog::CatalogItem &item,
                   std::function<void()> on_back,
                   std::function<void(const pinx::catalog::CatalogItem &)> on_install);

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
        pu::ui::elm::Image::Ref     poster_;
        pu::ui::elm::Rectangle::Ref overlay_;
        pu::ui::elm::TextBlock::Ref title_;
        pu::ui::elm::TextBlock::Ref meta_;
        pu::ui::elm::TextBlock::Ref desc_;
        pu::ui::elm::TextBlock::Ref notes_;
        pu::ui::elm::TextBlock::Ref back_hint_;
        pu::ui::elm::TextBlock::Ref install_hint_;

        void BuildLayout();
        void RefreshStrings();
};

} // namespace pinx::ui