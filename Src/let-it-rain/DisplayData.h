#pragma once

#include <d2d1.h>
#include <vector>
#include <memory>
#ifdef __cpp_lib_span
    #include <span>
#endif
#include <dcomp.h>
#include <wrl/client.h>
#include "ErrorHandling.h"

class FastNoiseLite;

namespace RainEngine {

class DisplayData {
public:
    explicit DisplayData(ID2D1DeviceContext* dc);
    ~DisplayData() = default;

    // Modern RAII-based methods with error handling
    [[nodiscard]] Result SetRainColor(COLORREF color) noexcept;
    [[nodiscard]] Result SetSceneBounds(const RECT& sceneRect, float scaleFactor) noexcept;
    
    // Getters with const correctness
    [[nodiscard]] constexpr int GetWidth() const noexcept { return width_; }
    [[nodiscard]] constexpr int GetHeight() const noexcept { return height_; }
    [[nodiscard]] constexpr float GetScaleFactor() const noexcept { return scaleFactor_; }
    [[nodiscard]] constexpr const RECT& GetSceneRect() const noexcept { return sceneRect_; }
    [[nodiscard]] constexpr const RECT& GetSceneRectNorm() const noexcept { return sceneRectNorm_; }
    [[nodiscard]] constexpr int GetMaxSnowHeight() const noexcept { return maxSnowHeight_; }

    // Scene pixels interface with span fallback
    #ifdef __cpp_lib_span
    [[nodiscard]] std::span<bool> GetScenePixels() noexcept {
        return scenePixels_ ? std::span<bool>{scenePixels_.get(), static_cast<size_t>(width_ * height_)} : std::span<bool>{};
    }
    [[nodiscard]] std::span<const bool> GetScenePixels() const noexcept {
        return scenePixels_ ? std::span<const bool>{scenePixels_.get(), static_cast<size_t>(width_ * height_)} : std::span<const bool>{};
    }
    #else
    // Fallback for compilers without std::span
    [[nodiscard]] bool* GetScenePixelsRaw() noexcept { return scenePixels_.get(); }
    [[nodiscard]] const bool* GetScenePixelsRaw() const noexcept { return scenePixels_.get(); }
    [[nodiscard]] size_t GetScenePixelCount() const noexcept { return static_cast<size_t>(width_ * height_); }
    #endif

    // Accessors for brushes
    [[nodiscard]] ID2D1SolidColorBrush* GetDropColorBrush() const noexcept { return dropColorBrush_.Get(); }
    [[nodiscard]] const auto& GetSplatterOpacityBrushes() const noexcept { return splatterOpacityBrushes_; }
    
    // Noise generator access
    [[nodiscard]] FastNoiseLite* GetNoiseGenerator() const noexcept { return noiseGenerator_.get(); }

    // Setters
    void SetMaxSnowHeight(int height) noexcept { maxSnowHeight_ = height; }

    // Direct member access for legacy compatibility
    RECT SceneRect;
    RECT SceneRectNorm;
    float ScaleFactor;
    int Width;
    int Height;
    int MaxSnowHeight;
    
    // Legacy brush and pointer access
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> DropColorBrush;
    std::vector<Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>> PrebuiltSplatterOpacityBrushes;
    bool* pScenePixels;
    FastNoiseLite* pNoiseGen;

private:
    static constexpr int MAX_SPLUTTER_FRAME_COUNT_ = 50;
    
    // Private data members with modern naming convention
    int width_ = 100;
    int height_ = 100;
    float scaleFactor_ = 1.0f;
    int maxSnowHeight_ = 0;

    RECT sceneRect_{0, 0, 100, 100};
    RECT sceneRectNorm_{0, 0, 100, 100};

    ID2D1DeviceContext* deviceContext_; // Non-owning pointer
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> dropColorBrush_;
    std::vector<Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>> splatterOpacityBrushes_;

    // Modern smart pointer management
    std::unique_ptr<bool[]> scenePixels_;
    std::unique_ptr<FastNoiseLite> noiseGenerator_;

    // Helper methods
    [[nodiscard]] static constexpr bool IsSameRect(const RECT& lhs, const RECT& rhs) noexcept {
        return lhs.left == rhs.left && lhs.top == rhs.top &&
               lhs.right == rhs.right && lhs.bottom == rhs.bottom;
    }

    [[nodiscard]] Result CreateSplatterBrushes(float red, float green, float blue) noexcept;
    void SyncPublicMembers() noexcept;
};

} // namespace RainEngine

// Type alias for backward compatibility during transition
using DisplayData = RainEngine::DisplayData;