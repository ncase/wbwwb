#include "VulkanFont.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace wbwwb::vulkan {

namespace {

std::vector<std::uint32_t> LatinUiCodepoints() {
    std::vector<std::uint32_t> codepoints;
    codepoints.reserve(352);
    for (std::uint32_t codepoint = 32; codepoint <= 126; ++codepoint) {
        codepoints.push_back(codepoint);
    }
    for (std::uint32_t codepoint = 160; codepoint <= 383; ++codepoint) {
        codepoints.push_back(codepoint);
    }
    return codepoints;
}

std::uint32_t DecodeUtf8(std::string_view text, std::size_t& offset) {
    const unsigned char lead = static_cast<unsigned char>(text[offset++]);
    if (lead < 0x80) {
        return lead;
    }
    if ((lead >> 5) == 0x6 && offset < text.size()) {
        const unsigned char b1 = static_cast<unsigned char>(text[offset++]);
        if ((b1 & 0xc0) == 0x80) {
            return ((lead & 0x1f) << 6) | (b1 & 0x3f);
        }
    } else if ((lead >> 4) == 0xe && offset + 1 < text.size()) {
        const unsigned char b1 = static_cast<unsigned char>(text[offset++]);
        const unsigned char b2 = static_cast<unsigned char>(text[offset++]);
        if ((b1 & 0xc0) == 0x80 && (b2 & 0xc0) == 0x80) {
            return ((lead & 0x0f) << 12) | ((b1 & 0x3f) << 6) | (b2 & 0x3f);
        }
    } else if ((lead >> 3) == 0x1e && offset + 2 < text.size()) {
        offset += 3;
    }
    return '?';
}

std::wstring FaceNameForPath(const std::filesystem::path& fontPath) {
    const std::wstring filename = fontPath.stem().wstring();
    if (filename.find(L"arial") != std::wstring::npos || filename.find(L"Arial") != std::wstring::npos) {
        return L"Arial";
    }
    if (filename.find(L"calibri") != std::wstring::npos || filename.find(L"Calibri") != std::wstring::npos) {
        return L"Calibri";
    }
    if (filename.find(L"DejaVu") != std::wstring::npos) {
        return L"DejaVu Sans";
    }
    if (filename.find(L"Liberation") != std::wstring::npos) {
        return L"Liberation Sans";
    }
    return L"Arial";
}

} // namespace

void VulkanFont::Create(const VulkanDevice& device,
                        VkCommandPool commandPool,
                        VkQueue graphicsQueue,
                        const std::filesystem::path& fontPath,
                        int pixelHeight) {
    if (pixelHeight <= 0) {
        throw std::runtime_error("Cannot create Vulkan font atlas with non-positive pixel height");
    }

    const bool addedPrivateFont = std::filesystem::exists(fontPath) &&
        AddFontResourceExW(fontPath.c_str(), FR_PRIVATE, nullptr) > 0;

    HDC measureDc = CreateCompatibleDC(nullptr);
    if (measureDc == nullptr) {
        throw std::runtime_error("CreateCompatibleDC failed for Vulkan font atlas");
    }

    const std::wstring faceName = FaceNameForPath(fontPath);
    HFONT font = CreateFontW(-pixelHeight,
                             0,
                             0,
                             0,
                             FW_NORMAL,
                             FALSE,
                             FALSE,
                             FALSE,
                             DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS,
                             ANTIALIASED_QUALITY,
                             DEFAULT_PITCH | FF_DONTCARE,
                             faceName.c_str());
    if (font == nullptr) {
        DeleteDC(measureDc);
        if (addedPrivateFont) {
            RemoveFontResourceExW(fontPath.c_str(), FR_PRIVATE, nullptr);
        }
        throw std::runtime_error("CreateFontW failed for Vulkan font atlas");
    }

    HGDIOBJ previousFont = SelectObject(measureDc, font);
    TEXTMETRICW metrics{};
    if (!GetTextMetricsW(measureDc, &metrics)) {
        SelectObject(measureDc, previousFont);
        DeleteObject(font);
        DeleteDC(measureDc);
        if (addedPrivateFont) {
            RemoveFontResourceExW(fontPath.c_str(), FR_PRIVATE, nullptr);
        }
        throw std::runtime_error("GetTextMetricsW failed for Vulkan font atlas");
    }

    pixelHeight_ = metrics.tmHeight;
    lineHeight_ = static_cast<float>(metrics.tmHeight);

    constexpr int padding = 2;
    constexpr int atlasWidth = 2048;
    int cursorX = padding;
    int cursorY = padding;
    int rowHeight = metrics.tmHeight + padding * 2;

    const std::vector<std::uint32_t> codepoints = LatinUiCodepoints();
    for (std::uint32_t codepoint : codepoints) {
        wchar_t ch = static_cast<wchar_t>(codepoint);
        SIZE size{};
        if (!GetTextExtentPoint32W(measureDc, &ch, 1, &size)) {
            size.cx = metrics.tmAveCharWidth;
            size.cy = metrics.tmHeight;
        }
        const int glyphWidth = std::max(1, static_cast<int>(size.cx));
        const int cellWidth = glyphWidth + padding * 2;
        if (cursorX + cellWidth + padding > atlasWidth) {
            cursorX = padding;
            cursorY += rowHeight;
        }

        Glyph glyph;
        glyph.x = static_cast<float>(cursorX);
        glyph.y = static_cast<float>(cursorY);
        glyph.width = static_cast<float>(glyphWidth);
        glyph.height = static_cast<float>(metrics.tmHeight);
        glyph.advance = static_cast<float>(glyphWidth);
        if (codepoint < glyphs_.size()) {
            glyphs_[static_cast<std::size_t>(codepoint)] = glyph;
        }
        if (codepoint == '?') {
            fallback_ = glyph;
        }
        cursorX += cellWidth;
    }

    const int atlasHeight = 1 << static_cast<int>(std::ceil(std::log2(static_cast<double>(cursorY + rowHeight + padding))));
    BITMAPINFO bitmapInfo{};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = atlasWidth;
    bitmapInfo.bmiHeader.biHeight = -atlasHeight;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    void* dibPixels = nullptr;
    HBITMAP bitmap = CreateDIBSection(measureDc, &bitmapInfo, DIB_RGB_COLORS, &dibPixels, nullptr, 0);
    if (bitmap == nullptr || dibPixels == nullptr) {
        SelectObject(measureDc, previousFont);
        DeleteObject(font);
        DeleteDC(measureDc);
        if (addedPrivateFont) {
            RemoveFontResourceExW(fontPath.c_str(), FR_PRIVATE, nullptr);
        }
        throw std::runtime_error("CreateDIBSection failed for Vulkan font atlas");
    }

    HGDIOBJ previousBitmap = SelectObject(measureDc, bitmap);
    RECT clearRect{0, 0, atlasWidth, atlasHeight};
    HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(measureDc, &clearRect, blackBrush);
    DeleteObject(blackBrush);
    SetBkMode(measureDc, TRANSPARENT);
    SetTextColor(measureDc, RGB(255, 255, 255));
    SetTextAlign(measureDc, TA_LEFT | TA_TOP | TA_NOUPDATECP);

    for (std::uint32_t codepoint : codepoints) {
        if (codepoint >= glyphs_.size()) {
            continue;
        }
        const Glyph& glyph = glyphs_[static_cast<std::size_t>(codepoint)];
        wchar_t ch = static_cast<wchar_t>(codepoint);
        TextOutW(measureDc, static_cast<int>(glyph.x), static_cast<int>(glyph.y), &ch, 1);
    }

    const auto* bgra = static_cast<const std::uint8_t*>(dibPixels);
    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(atlasWidth) * static_cast<std::size_t>(atlasHeight) * 4u);
    for (std::size_t i = 0; i < static_cast<std::size_t>(atlasWidth) * static_cast<std::size_t>(atlasHeight); ++i) {
        const std::uint8_t b = bgra[i * 4 + 0];
        const std::uint8_t g = bgra[i * 4 + 1];
        const std::uint8_t r = bgra[i * 4 + 2];
        const std::uint8_t a = std::max({r, g, b});
        rgba[i * 4 + 0] = 255;
        rgba[i * 4 + 1] = 255;
        rgba[i * 4 + 2] = 255;
        rgba[i * 4 + 3] = a;
    }

    atlas_.CreateRGBA8(device,
                       commandPool,
                       graphicsQueue,
                       static_cast<std::uint32_t>(atlasWidth),
                       static_cast<std::uint32_t>(atlasHeight),
                       rgba.data(),
                       VK_FILTER_LINEAR);

    SelectObject(measureDc, previousBitmap);
    SelectObject(measureDc, previousFont);
    DeleteObject(bitmap);
    DeleteObject(font);
    DeleteDC(measureDc);
    if (addedPrivateFont) {
        RemoveFontResourceExW(fontPath.c_str(), FR_PRIVATE, nullptr);
    }
}

void VulkanFont::Destroy(VkDevice device) {
    atlas_.Destroy(device);
    glyphs_ = {};
    fallback_ = {};
    pixelHeight_ = 0;
    lineHeight_ = 0.0f;
}

const VulkanFont::Glyph& VulkanFont::GlyphFor(std::uint32_t codepoint) const noexcept {
    if (codepoint < glyphs_.size() && glyphs_[static_cast<std::size_t>(codepoint)].advance > 0.0f) {
        return glyphs_[static_cast<std::size_t>(codepoint)];
    }
    return fallback_;
}

void VulkanFont::DrawText(VulkanRenderer2D& renderer,
                          VkDescriptorSet descriptorSet,
                          std::string_view text,
                          float x,
                          float y,
                          float size,
                          VulkanRenderer2D::Color color) const {
    if (!Ready() || descriptorSet == VK_NULL_HANDLE || size <= 0.0f || color.a <= 0.0f) {
        return;
    }

    const float scale = size / static_cast<float>(std::max(1, pixelHeight_));
    float cursorX = x;
    float cursorY = y;
    for (std::size_t offset = 0; offset < text.size();) {
        const std::uint32_t codepoint = DecodeUtf8(text, offset);
        if (codepoint == '\n') {
            cursorX = x;
            cursorY += lineHeight_ * scale;
            continue;
        }

        const Glyph& glyph = GlyphFor(codepoint);
        if (codepoint != ' ') {
            renderer.DrawTexturedSourceRect(cursorX,
                                            cursorY,
                                            glyph.width * scale,
                                            glyph.height * scale,
                                            descriptorSet,
                                            AtlasWidth(),
                                            AtlasHeight(),
                                            {glyph.x, glyph.y, glyph.width, glyph.height},
                                            color);
        }
        cursorX += glyph.advance * scale;
    }
}

VulkanFont::TextExtent VulkanFont::MeasureText(std::string_view text, float size) const {
    if (size <= 0.0f || pixelHeight_ <= 0) {
        return {};
    }

    const float scale = size / static_cast<float>(pixelHeight_);
    float lineWidth = 0.0f;
    float maxWidth = 0.0f;
    float lineCount = 1.0f;
    for (std::size_t offset = 0; offset < text.size();) {
        const std::uint32_t codepoint = DecodeUtf8(text, offset);
        if (codepoint == '\n') {
            maxWidth = std::max(maxWidth, lineWidth);
            lineWidth = 0.0f;
            lineCount += 1.0f;
            continue;
        }
        lineWidth += GlyphFor(codepoint).advance * scale;
    }
    maxWidth = std::max(maxWidth, lineWidth);
    return {maxWidth, lineHeight_ * scale * lineCount};
}

} // namespace wbwwb::vulkan
