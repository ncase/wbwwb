#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace wb {

struct LanguageInfo {
    std::string id;
    std::string displayName;
    std::string englishName;
    std::vector<std::string> authors;
    bool fan = false;
};

class Localization {
public:
    bool load(
        const std::filesystem::path& officialDirectory,
        const std::vector<std::filesystem::path>& fanDirectories,
        const std::string& preferredLanguageId
    );
    bool setLanguage(const std::string& languageId);
    void stepLanguage(int direction);

    const std::string& tr(const std::string& key) const;
    const std::string& tr(const char* key) const { return tr(std::string(key)); }

    const std::vector<LanguageInfo>& languages() const { return languages_; }
    const std::string& currentLanguageId() const { return currentLanguageId_; }
    const std::string& currentLanguageDisplayName() const;
    const std::string& currentLanguageDisplayLabel() const;
    bool hasLanguage(const std::string& languageId) const;

private:
    enum class Source {
        Official,
        Fan
    };

    struct LanguagePack {
        LanguageInfo info;
        std::string displayLabel;
        std::unordered_map<std::string, std::string> strings;
    };

    void loadDirectory(const std::filesystem::path& directory, Source source);
    bool loadFile(const std::filesystem::path& path, Source source);
    const LanguagePack* pack(const std::string& languageId) const;
    void logMissingOnce(const std::string& languageId, const std::string& key) const;
    void rebuildLanguageList();

    std::vector<LanguageInfo> languages_;
    std::unordered_map<std::string, LanguagePack> packs_;
    std::unordered_set<std::string> officialLanguageIds_;
    std::unordered_set<std::string> knownKeys_;
    std::string fallbackLanguageId_ = "en";
    std::string currentLanguageId_ = "en";
    mutable std::unordered_set<std::string> missingKeysLogged_;
    mutable std::unordered_map<std::string, std::string> missingKeyFallbacks_;
};

}  // namespace wb
