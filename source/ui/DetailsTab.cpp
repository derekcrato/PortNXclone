#include <ui/DetailsTab.hpp>
#include <app/I18n.hpp>
#include <catalog/CatalogItem.hpp>

#include <switch.h>
#include <cstdio>
#include <string>
#include <cstdint>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <unordered_map>
#include <sys/stat.h>

#include <net/HttpClient.hpp>
#include <pu/ui/render/render_SDL2.hpp>

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

constexpr const char *kPosterCacheDir = "sdmc:/switch/PortNX/posters";

std::uint64_t fnv1a64(const std::string &s) {
    std::uint64_t h = 14695981039346656037ULL;
    for (unsigned char c : s) { h ^= c; h *= 1099511628211ULL; }
    return h;
}

std::string posterCachePath(const std::string &url) {
    char buf[128];
    std::snprintf(buf, sizeof(buf), "%s/%016llx.jpg", kPosterCacheDir,
                  static_cast<unsigned long long>(fnv1a64(url)));
    return buf;
}

std::vector<uint8_t> loadPosterFromDisk(const std::string &url) {
    FILE *f = fopen(posterCachePath(url).c_str(), "rb");
    if (!f) return {};
    fseek(f, 0, SEEK_END);
    const long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return {}; }
    std::vector<uint8_t> buf(static_cast<std::size_t>(sz));
    fread(buf.data(), 1, buf.size(), f);
    fclose(f);
    return buf;
}

void savePosterToDisk(const std::string &url, const std::vector<uint8_t> &data) {
    mkdir(kPosterCacheDir, 0777);
    FILE *f = fopen(posterCachePath(url).c_str(), "wb");
    if (!f) return;
    fwrite(data.data(), 1, data.size(), f);
    fclose(f);
}

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

struct PosterThreadArgs {
    std::string url;
    std::function<void(std::vector<uint8_t>)> callback;
};

void posterThreadFunc(void *arg) {
    auto *a = static_cast<PosterThreadArgs *>(arg);
    pinx::net::HttpOptions opts;
    opts.timeout_ms = 10000;
    opts.verify_tls = false;

    const pinx::net::HttpResponse res = pinx::net::Get(a->url, opts);
    if (res.success && !res.body.empty()) {
        std::vector<uint8_t> bytes(res.body.begin(), res.body.end());
        savePosterToDisk(a->url, bytes);
        if (a->callback) a->callback(std::move(bytes));
    }
    delete a;
}

} // anonymous namespace

DetailsTab::DetailsTab(const pinx::catalog::CatalogItem &item,
                       std::function<void()> on_back,
                       std::function<void(const pinx::catalog::CatalogItem &)> on_install)
    : item_(item), on_back_(std::move(on_back)), on_install_(std::move(on_install)),
      poster_loaded_(false), poster_thread_running_(false) {
    BuildLayout();
    if (!item_.icon_url.empty()) {
        StartPosterLoad();
    }
}

DetailsTab::~DetailsTab() {
    if (poster_thread_running_) {
        threadWaitForExit(&poster_thread_);
        threadClose(&poster_thread_);
    }
}

void DetailsTab::StartPosterLoad() {
    if (poster_thread_running_) return;
    
    auto *args = new PosterThreadArgs{
        item_.icon_url,
        [this](std::vector<uint8_t> bytes) {
            poster_bytes_ = std::move(bytes);
            poster_ready_ = true;
        }
    };

    const Result rc = threadCreate(&poster_thread_, posterThreadFunc, args,
                                   4 * 1024 * 1024, 0x2C, -2);
    if (R_SUCCEEDED(rc)) {
        threadStart(&poster_thread_);
        poster_thread_running_ = true;
    } else {
        delete args;
    }
}

void DetailsTab::Poll() {
    if (poster_ready_ && !poster_loaded_) {
        if (!poster_bytes_.empty()) {
            auto raw = pu::ui::render::LoadImageFromBuffer(poster_bytes_.data(), poster_bytes_.size());
            if (raw != nullptr) {
                poster_tex_ = pu::sdl2::TextureHandle::New(raw);
                if (poster_tex_) {
                    if (poster_img_) {
                        poster_img_->SetImage(poster_tex_);
                        poster_img_->SetVisible(true);
                    }
                    if (poster_placeholder_) {
                        poster_placeholder_->SetVisible(false);
                    }
                    poster_loaded_ = true;
                }
            }
        }
        poster_ready_ = false;
    }
}

void DetailsTab::AddElementsTo(pu::ui::Layout *layout) {
    if (!layout) return;
    layout->Add(bg_);
    layout->Add(overlay_);
    if (poster_placeholder_) layout->Add(poster_placeholder_);
    if (poster_img_) layout->Add(poster_img_);
    layout->Add(title_);
    layout->Add(meta_);
    layout->Add(desc_);
    layout->Add(notes_);
    layout->Add(back_hint_);
    layout->Add(install_hint_);
}

void DetailsTab::Show() {
    visible_ = true;
    bg_->SetVisible(true);
    overlay_->SetVisible(true);
    if (poster_placeholder_) poster_placeholder_->SetVisible(!poster_loaded_);
    if (poster_img_) poster_img_->SetVisible(poster_loaded_);
    title_->SetVisible(true);
    meta_->SetVisible(true);
    desc_->SetVisible(true);
    notes_->SetVisible(true);
    back_hint_->SetVisible(true);
    install_hint_->SetVisible(true);
    RefreshStrings();
}

void DetailsTab::Hide() {
    visible_ = false;
    bg_->SetVisible(false);
    overlay_->SetVisible(false);
    if (poster_placeholder_) poster_placeholder_->SetVisible(false);
    if (poster_img_) poster_img_->SetVisible(false);
    title_->SetVisible(false);
    meta_->SetVisible(false);
    desc_->SetVisible(false);
    notes_->SetVisible(false);
    back_hint_->SetVisible(false);
    install_hint_->SetVisible(false);
}

void DetailsTab::TriggerInstall() {
    if (on_install_) on_install_(item_);
}

void DetailsTab::SetItem(const pinx::catalog::CatalogItem &item) {
    item_ = item;
    poster_loaded_ = false;
    poster_ready_ = false;
    poster_bytes_.clear();
    poster_tex_.reset();
    if (poster_thread_running_) {
        threadWaitForExit(&poster_thread_);
        threadClose(&poster_thread_);
        poster_thread_running_ = false;
    }
    if (!item_.icon_url.empty()) {
        StartPosterLoad();
    }
    RefreshStrings();
}

void DetailsTab::BuildLayout() {
    bg_ = pu::ui::elm::Rectangle::New(0, 0, 1280, 720, kBg);
    
    overlay_ = pu::ui::elm::Rectangle::New(0, 360, 1280, 360, kOverlay);
    
    // Placeholder poster (gray rectangle) - shown while loading
    poster_placeholder_ = pu::ui::elm::Rectangle::New(860, 80, 420, 240, pu::ui::Color{30, 30, 40, 255});
    
    // Real poster image - shown after loading
    pu::sdl2::TextureHandle::Ref empty_th = nullptr;
    poster_img_ = pu::ui::elm::Image::New(860, 80, empty_th);
    poster_img_->SetWidth(420);
    poster_img_->SetHeight(240);
    poster_img_->SetVisible(false);
    
    title_ = pu::ui::elm::TextBlock::New(40, 380, item_.name);
    title_->SetFont(kFontLarge);
    title_->SetColor(kText);
    title_->SetClampWidth(800);
    
    std::string meta_text;
    if (!item_.genre.empty()) meta_text += item_.genre + "  |  ";
    if (!item_.portedFrom.empty()) meta_text += item_.portedFrom + "  |  ";
    if (!item_.region.empty()) meta_text += item_.region + "  |  ";
    meta_text += FormatSize(item_.size);
    if (!item_.version.empty()) meta_text += "  |  v" + item_.version;
    if (item_.rating > 0) meta_text += "  |  " + std::to_string(item_.rating) + "/10";
    if (!item_.publisher.empty()) meta_text += "  |  " + item_.publisher;
    
    meta_ = pu::ui::elm::TextBlock::New(40, 440, meta_text);
    meta_->SetFont(kFontMedium);
    meta_->SetColor(kMuted);
    meta_->SetClampWidth(800);
    
    desc_ = pu::ui::elm::TextBlock::New(40, 480, item_.description.empty() ? "Sem descrição disponível." : item_.description);
    desc_->SetFont(kFontMedium);
    desc_->SetColor(kText);
    desc_->SetClampWidth(800);
    
    std::string notes_text;
    if (!item_.notes.empty()) {
        notes_text = "Notas:\n" + item_.notes;
    }
    notes_ = pu::ui::elm::TextBlock::New(40, 580, notes_text);
    notes_->SetFont(kFontSmall);
    notes_->SetColor(kMuted);
    notes_->SetClampWidth(800);
    
    back_hint_ = pu::ui::elm::TextBlock::New(30, 680, "B Voltar ao menu");
    back_hint_->SetFont(kFontMedium);
    back_hint_->SetColor(kMuted);
    
    install_hint_ = pu::ui::elm::TextBlock::New(1000, 680, "A Instalar / Baixar");
    install_hint_->SetFont(kFontMedium);
    install_hint_->SetColor(kAccent);
}

void DetailsTab::RefreshStrings() {
    if (!title_) return;
    
    title_->SetText(item_.name);
    
    std::string meta_text;
    if (!item_.genre.empty()) meta_text += item_.genre + "  |  ";
    if (!item_.portedFrom.empty()) meta_text += item_.portedFrom + "  |  ";
    if (!item_.region.empty()) meta_text += item_.region + "  |  ";
    meta_text += FormatSize(item_.size);
    if (!item_.version.empty()) meta_text += "  |  v" + item_.version;
    if (item_.rating > 0) meta_text += "  |  " + std::to_string(item_.rating) + "/10";
    if (!item_.publisher.empty()) meta_text += "  |  " + item_.publisher;
    meta_->SetText(meta_text);
    
    desc_->SetText(item_.description.empty() ? "Sem descrição disponível." : item_.description);
    
    std::string notes_text;
    if (!item_.notes.empty()) {
        notes_text = "Notas:\n" + item_.notes;
    }
    notes_->SetText(notes_text);
}

} // namespace pinx::ui