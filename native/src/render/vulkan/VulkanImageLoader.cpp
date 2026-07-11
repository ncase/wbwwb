#include "VulkanImageLoader.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <limits>
#include <stdexcept>
#include <string>

namespace wbwwb::vulkan {

namespace {

std::string NarrowPath(const std::filesystem::path& path) {
    return path.u8string();
}

std::string HResultMessage(HRESULT result, const char* action, const std::filesystem::path& path) {
    return std::string(action) + " for " + NarrowPath(path) + " failed with HRESULT 0x" +
           std::to_string(static_cast<unsigned long>(result));
}

void CheckHr(HRESULT result, const char* action, const std::filesystem::path& path) {
    if (FAILED(result)) {
        throw std::runtime_error(HResultMessage(result, action, path));
    }
}

class ScopedCom final {
public:
    ScopedCom() {
        const HRESULT result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (SUCCEEDED(result)) {
            initialized_ = true;
            return;
        }
        if (result == RPC_E_CHANGED_MODE) {
            return;
        }
        throw std::runtime_error("CoInitializeEx failed with HRESULT 0x" +
                                 std::to_string(static_cast<unsigned long>(result)));
    }

    ~ScopedCom() {
        if (initialized_) {
            CoUninitialize();
        }
    }

    ScopedCom(const ScopedCom&) = delete;
    ScopedCom& operator=(const ScopedCom&) = delete;

private:
    bool initialized_ = false;
};

} // namespace

LoadedImage LoadRgbaImage(const std::filesystem::path& path,
                          std::uint32_t requestedWidth,
                          std::uint32_t requestedHeight,
                          bool nearest) {
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("Image file does not exist: " + NarrowPath(path));
    }

    ScopedCom com;
    Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
    CheckHr(CoCreateInstance(CLSID_WICImagingFactory,
                             nullptr,
                             CLSCTX_INPROC_SERVER,
                             IID_PPV_ARGS(&factory)),
            "creating WIC imaging factory",
            path);

    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    CheckHr(factory->CreateDecoderFromFilename(path.c_str(),
                                               nullptr,
                                               GENERIC_READ,
                                               WICDecodeMetadataCacheOnLoad,
                                               &decoder),
            "opening image decoder",
            path);

    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
    CheckHr(decoder->GetFrame(0, &frame), "reading image frame", path);

    UINT width = 0;
    UINT height = 0;
    CheckHr(frame->GetSize(&width, &height), "reading image size", path);
    if (width == 0 || height == 0) {
        throw std::runtime_error("Image has zero dimensions: " + NarrowPath(path));
    }

    Microsoft::WRL::ComPtr<IWICBitmapScaler> scaler;
    IWICBitmapSource* source = frame.Get();
    if (requestedWidth > 0 && requestedHeight > 0 &&
        (requestedWidth != width || requestedHeight != height)) {
        CheckHr(factory->CreateBitmapScaler(&scaler), "creating WIC bitmap scaler", path);
        CheckHr(scaler->Initialize(frame.Get(),
                                   requestedWidth,
                                   requestedHeight,
                                   nearest ? WICBitmapInterpolationModeNearestNeighbor
                                           : WICBitmapInterpolationModeFant),
                "resizing image",
                path);
        source = scaler.Get();
        width = requestedWidth;
        height = requestedHeight;
    }

    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    CheckHr(factory->CreateFormatConverter(&converter), "creating WIC format converter", path);
    CheckHr(converter->Initialize(source,
                                  GUID_WICPixelFormat32bppRGBA,
                                  WICBitmapDitherTypeNone,
                                  nullptr,
                                  0.0,
                                  WICBitmapPaletteTypeCustom),
            "converting image to RGBA8",
            path);

    const std::uint64_t stride64 = static_cast<std::uint64_t>(width) * 4ull;
    const std::uint64_t size64 = stride64 * static_cast<std::uint64_t>(height);
    if (stride64 > std::numeric_limits<UINT>::max() ||
        size64 > std::numeric_limits<UINT>::max() ||
        size64 > std::numeric_limits<std::size_t>::max()) {
        throw std::runtime_error("Image is too large to load: " + NarrowPath(path));
    }

    LoadedImage image;
    image.width = static_cast<std::uint32_t>(width);
    image.height = static_cast<std::uint32_t>(height);
    image.rgba.resize(static_cast<std::size_t>(size64));
    CheckHr(converter->CopyPixels(nullptr,
                                  static_cast<UINT>(stride64),
                                  static_cast<UINT>(size64),
                                  image.rgba.data()),
            "copying image pixels",
            path);
    return image;
}

} // namespace wbwwb::vulkan
