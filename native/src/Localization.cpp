#include "Localization.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <system_error>

namespace wb {
namespace {

constexpr const char* kEnglishLanguageId = "en";
constexpr const char* kTemplateFileName = "template.json";
constexpr int kSupportedSchemaVersion = 1;
constexpr std::uintmax_t kMaxFanFileBytes = 1024u * 1024u;
constexpr std::size_t kMaxLanguageIdLength = 48;
constexpr std::size_t kMaxDisplayNameLength = 96;
constexpr std::size_t kMaxKeyLength = 128;
constexpr std::size_t kMaxStringLength = 4096;

bool isTemplateFile(const std::filesystem::path& path) {
    return path.filename() == kTemplateFileName;
}

bool hasNonWhitespace(const std::string& value) {
    return std::any_of(value.begin(), value.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    });
}

bool isSafeLanguageId(const std::string& id) {
    if (id.empty() || id.size() > kMaxLanguageIdLength || id.front() == '-' || id.back() == '-') {
        return false;
    }
    if (!std::isalpha(static_cast<unsigned char>(id.front()))) {
        return false;
    }
    for (char ch : id) {
        const unsigned char value = static_cast<unsigned char>(ch);
        if (!(std::isalnum(value) || ch == '-')) {
            return false;
        }
    }
    return id.find("..") == std::string::npos;
}

bool isSafeStringKey(const std::string& key) {
    if (key.empty() || key.size() > kMaxKeyLength) {
        return false;
    }
    for (char ch : key) {
        const unsigned char value = static_cast<unsigned char>(ch);
        if (!(std::isalnum(value) || ch == '.' || ch == '_' || ch == '-')) {
            return false;
        }
    }
    return true;
}

bool fileTooLarge(const std::filesystem::path& path) {
    std::error_code ec;
    const std::uintmax_t size = std::filesystem::file_size(path, ec);
    return !ec && size > kMaxFanFileBytes;
}

std::vector<std::filesystem::path> jsonFilesIn(const std::filesystem::path& directory) {
    std::vector<std::filesystem::path> files;
    std::error_code ec;
    if (!std::filesystem::exists(directory, ec) || !std::filesystem::is_directory(directory, ec)) {
        return files;
    }

    for (const auto& entry : std::filesystem::directory_iterator(directory, ec)) {
        if (ec) {
            break;
        }
        if (entry.is_regular_file(ec) && entry.path().extension() == ".json" && !isTemplateFile(entry.path())) {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

}  // namespace

bool Localization::load(
    const std::filesystem::path& officialDirectory,
    const std::vector<std::filesystem::path>& fanDirectories,
    const std::string& preferredLanguageId
) {
    languages_.clear();
    packs_.clear();
    officialLanguageIds_.clear();
    knownKeys_.clear();
    missingKeysLogged_.clear();
    missingKeyFallbacks_.clear();
    fallbackLanguageId_ = kEnglishLanguageId;
    currentLanguageId_ = kEnglishLanguageId;

    loadDirectory(officialDirectory, Source::Official);

    if (const LanguagePack* english = pack(kEnglishLanguageId)) {
        for (const auto& [key, _] : english->strings) {
            knownKeys_.insert(key);
        }
    } else if (!packs_.empty()) {
        fallbackLanguageId_ = packs_.begin()->first;
        for (const auto& [key, _] : packs_.begin()->second.strings) {
            knownKeys_.insert(key);
        }
        std::cerr << "Warning: English localization file is missing; using '" << fallbackLanguageId_ << "' as fallback.\n";
    }

    for (const std::filesystem::path& directory : fanDirectories) {
        loadDirectory(directory, Source::Fan);
    }

    rebuildLanguageList();

    if (packs_.empty()) {
        std::cerr << "Warning: no localization files were loaded from " << officialDirectory << ".\n";
        return false;
    }

    if (!hasLanguage(fallbackLanguageId_)) {
        fallbackLanguageId_ = languages_.front().id;
    }

    if (!setLanguage(preferredLanguageId)) {
        setLanguage(fallbackLanguageId_);
    }
    return true;
}

void Localization::loadDirectory(const std::filesystem::path& directory, Source source) {
    const std::vector<std::filesystem::path> files = jsonFilesIn(directory);
    for (const auto& file : files) {
        loadFile(file, source);
    }
}

bool Localization::loadFile(const std::filesystem::path& path, Source source) {
    const bool fan = source == Source::Fan;
    const char* label = fan ? "fan translation" : "localization file";

    if (fan && fileTooLarge(path)) {
        std::cerr << "Warning: fan translation " << path << " ignored because it is larger than 1 MB.\n";
        return false;
    }

    std::ifstream input(path, std::ios::binary);
    if (!input) {
        std::cerr << "Warning: could not open " << label << " " << path << ".\n";
        return false;
    }

    try {
        nlohmann::json json;
        input >> json;

        if (fan) {
            if (!json.contains("schema_version") || !json.at("schema_version").is_number_integer()) {
                std::cerr << "Warning: fan translation " << path << " ignored because schema_version is missing or invalid.\n";
                return false;
            }
            const int schemaVersion = json.at("schema_version").get<int>();
            if (schemaVersion != kSupportedSchemaVersion) {
                std::cerr << "Warning: fan translation " << path << " ignored because schema_version " << schemaVersion << " is unsupported.\n";
                return false;
            }
        } else if (json.contains("schema_version") &&
                   (!json.at("schema_version").is_number_integer() || json.at("schema_version").get<int>() != kSupportedSchemaVersion)) {
            std::cerr << "Warning: localization file " << path << " ignored because schema_version is unsupported.\n";
            return false;
        }

        if (!json.contains("language_id") || !json.at("language_id").is_string() ||
            !json.contains("display_name") || !json.at("display_name").is_string() ||
            !json.contains("strings") || !json.at("strings").is_object()) {
            std::cerr << "Warning: " << label << " " << path << " ignored because required metadata is missing or invalid.\n";
            return false;
        }

        LanguagePack loaded;
        loaded.info.id = json.at("language_id").get<std::string>();
        loaded.info.displayName = json.at("display_name").get<std::string>();
        loaded.info.fan = fan;
        loaded.displayLabel = loaded.info.displayName + (fan ? " (Fan)" : "");

        if (!isSafeLanguageId(loaded.info.id)) {
            std::cerr << "Warning: " << label << " " << path << " ignored because language_id is unsafe.\n";
            return false;
        }
        if (loaded.info.displayName.size() > kMaxDisplayNameLength || !hasNonWhitespace(loaded.info.displayName)) {
            std::cerr << "Warning: " << label << " " << path << " ignored because display_name is empty or too long.\n";
            return false;
        }

        if (json.contains("english_name") && json.at("english_name").is_string()) {
            loaded.info.englishName = json.at("english_name").get<std::string>();
        }
        if (json.contains("authors") && json.at("authors").is_array()) {
            for (const auto& author : json.at("authors")) {
                if (author.is_string() && loaded.info.authors.size() < 16) {
                    loaded.info.authors.push_back(author.get<std::string>());
                }
            }
        }

        auto duplicate = packs_.find(loaded.info.id);
        if (duplicate != packs_.end()) {
            if (fan && officialLanguageIds_.find(loaded.info.id) != officialLanguageIds_.end()) {
                std::cerr << "Warning: fan translation " << path << " ignored because language_id '" << loaded.info.id
                          << "' duplicates an official language. Use a distinct ID such as '" << loaded.info.id << "-fan'.\n";
            } else {
                std::cerr << "Warning: " << label << " " << path << " ignored because language_id '" << loaded.info.id
                          << "' was already loaded.\n";
            }
            return false;
        }

        int ignoredEntries = 0;
        for (const auto& [key, value] : json.at("strings").items()) {
            if (!isSafeStringKey(key)) {
                ++ignoredEntries;
                continue;
            }
            if (fan && !knownKeys_.empty() && knownKeys_.find(key) == knownKeys_.end()) {
                ++ignoredEntries;
                continue;
            }
            if (!value.is_string()) {
                ++ignoredEntries;
                continue;
            }
            std::string text = value.get<std::string>();
            if (text.size() > kMaxStringLength) {
                ++ignoredEntries;
                continue;
            }
            loaded.strings[key] = std::move(text);
        }

        if (loaded.strings.empty()) {
            std::cerr << "Warning: " << label << " " << path << " ignored because it has no usable string entries.\n";
            return false;
        }
        if (fan && ignoredEntries > 0) {
            std::cerr << "Warning: fan translation " << path << " ignored " << ignoredEntries
                      << " invalid, unsafe, or unknown string entries.\n";
        }

        if (!fan) {
            officialLanguageIds_.insert(loaded.info.id);
        }
        packs_[loaded.info.id] = std::move(loaded);
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "Warning: failed to parse " << label << " " << path << ": " << ex.what() << "\n";
        return false;
    }
}

bool Localization::setLanguage(const std::string& languageId) {
    if (!hasLanguage(languageId)) {
        return false;
    }
    currentLanguageId_ = languageId;
    return true;
}

void Localization::stepLanguage(int direction) {
    if (languages_.empty()) {
        return;
    }

    auto it = std::find_if(languages_.begin(), languages_.end(), [this](const LanguageInfo& language) {
        return language.id == currentLanguageId_;
    });
    int index = it == languages_.end() ? 0 : static_cast<int>(std::distance(languages_.begin(), it));
    const int count = static_cast<int>(languages_.size());
    index = (index + direction + count) % count;
    setLanguage(languages_[static_cast<std::size_t>(index)].id);
}

const std::string& Localization::tr(const std::string& key) const {
    if (const LanguagePack* current = pack(currentLanguageId_)) {
        auto it = current->strings.find(key);
        if (it != current->strings.end()) {
            return it->second;
        }
        logMissingOnce(currentLanguageId_, key);
    }

    if (const LanguagePack* fallback = pack(fallbackLanguageId_)) {
        auto it = fallback->strings.find(key);
        if (it != fallback->strings.end()) {
            return it->second;
        }
    }

    logMissingOnce(fallbackLanguageId_, key);
    return missingKeyFallbacks_.emplace(key, "[missing:" + key + "]").first->second;
}

const std::string& Localization::currentLanguageDisplayName() const {
    if (const LanguagePack* current = pack(currentLanguageId_)) {
        return current->info.displayName;
    }
    static const std::string unknown = "Unknown";
    return unknown;
}

const std::string& Localization::currentLanguageDisplayLabel() const {
    if (const LanguagePack* current = pack(currentLanguageId_)) {
        return current->displayLabel;
    }
    static const std::string unknown = "Unknown";
    return unknown;
}

bool Localization::hasLanguage(const std::string& languageId) const {
    return packs_.find(languageId) != packs_.end();
}

const Localization::LanguagePack* Localization::pack(const std::string& languageId) const {
    auto it = packs_.find(languageId);
    return it == packs_.end() ? nullptr : &it->second;
}

void Localization::logMissingOnce(const std::string& languageId, const std::string& key) const {
    const std::string token = languageId + ":" + key;
    if (missingKeysLogged_.insert(token).second) {
        std::cerr << "Warning: missing localization key '" << key << "' for language '" << languageId << "'.\n";
    }
}

void Localization::rebuildLanguageList() {
    languages_.clear();
    for (const auto& [_, languagePack] : packs_) {
        languages_.push_back(languagePack.info);
    }
    std::sort(languages_.begin(), languages_.end(), [](const LanguageInfo& a, const LanguageInfo& b) {
        if (a.id == kEnglishLanguageId) return true;
        if (b.id == kEnglishLanguageId) return false;
        if (a.fan != b.fan) return !a.fan;
        if (a.displayName != b.displayName) return a.displayName < b.displayName;
        return a.id < b.id;
    });
}

}  // namespace wb
