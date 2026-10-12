#pragma once

#include <cstdint>
#include <string>

namespace pinx::catalog {

enum class EntryKind {
    Directory,  // a navigable sub-listing
    File,       // a downloadable port (.nsz/.nsp/.xci/.xcz/...)
};

struct CatalogItem {
    EntryKind     kind = EntryKind::File;
    std::string   name;            // display name (titledb name when available)
    std::string   url;             // absolute URL to fetch / download (no #fragment)
    std::string   filename;        // download filename derived from the URL basename
    std::uint64_t size = 0;        // bytes for display (may be approximate)
    bool          size_authoritative = false;  // true only when from the file entry
    std::string   format;          // lowercase extension: nsz/nsp/xci/xcz/zip
    std::string   version;         // optional
    std::string   sha256;          // optional lowercase hex integrity hash
    std::string   icon_url;        // optional cover art URL (rendered later)
    std::uint64_t title_id = 0;   // 0 = unknown; set when filename is a 16-hex TID

    // Rich metadata sourced from the "titledb" section of the index JSON.
    std::string   description;     // short description / blurb
    std::string   notes;           // installation notes / caveats
    std::string   region;          // e.g. US, BR, EU
    std::string   publisher;       // publisher name
    std::uint64_t release_date = 0; // YYYYMMDD integer, 0 = unknown
    int           rating = 0;      // 0-10, 0 = unset
    bool          is_installable = false; // true if item can be installed
    std::string   genre;          // e.g. Ação, RPG, Puzzle
    std::string   portedFrom;     // origin platform: PC, Android, Arcade, etc.
};

} // namespace pinx::catalog