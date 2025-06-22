#include "DisplayData.h"
#include "FastNoiseLite.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace RainEngine {

DisplayData::DisplayData(ID2D1DeviceContext* dc) : deviceContext_(dc) {
    if (!dc) {
        throw std::invalid_argument("Device context cannot be null");
    }
    
    // Initialize noise generator with modern smart pointer
    noiseGenerator_ = std::make_unique<FastNoiseLite>();
    noiseGenerator_->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    
    // Initialize legacy compatibility members
    pNoiseGen = noiseGenerator_.get();
    pScenePixels = nullptr;
    
    // Sync public members with private ones
    SyncPublicMembers();
}

Result DisplayData::SetRainColor(const COLORREF color) noexcept {
    try {
        const auto red = static_cast<float>(GetRValue(color)) / 255.0f;
        const auto green = static_cast<float>(GetGValue(color)) / 255.0f;
        const auto blue = static_cast<float>(GetBValue(color)) / 255.0f;

        // Create main drop brush with full opacity
        const auto hr = deviceContext_->CreateSolidColorBrush(
            D2D1::ColorF(red, green, blue, 1.0f), 
            dropColorBrush_.GetAddressOf()
        );
        
        if (FAILED(hr)) {
            return Result::Error(Result::ErrorCode::DirectXError, 
                               "Failed to create drop color brush");
        }

        // Create splatter brushes with varying opacity
        auto result = CreateSplatterBrushes(red, green, blue);
        if (result.IsSuccess()) {
            SyncPublicMembers();
        }
        return result;
        
    } catch (const std::exception& e) {
        return Result::Error(Result::ErrorCode::ResourceAllocationFailed, e.what());
    } catch (...) {
        return Result::Error(Result::ErrorCode::ResourceAllocationFailed, 
                           "Unknown error in SetRainColor");
    }
}

Result DisplayData::CreateSplatterBrushes(const float red, const float green, const float blue) noexcept {
    try {
        // Clear existing brushes
        splatterOpacityBrushes_.clear();
        splatterOpacityBrushes_.reserve(MAX_SPLUTTER_FRAME_COUNT_);

        for (int i = 0; i < MAX_SPLUTTER_FRAME_COUNT_; ++i) {
            const auto alpha = (1.0f - static_cast<float>(i) / static_cast<float>(MAX_SPLUTTER_FRAME_COUNT_)) * 0.75f;
            const auto splatterColor = D2D1::ColorF(red, green, blue, alpha);
            
            Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> splatterBrush;
            const auto hr = deviceContext_->CreateSolidColorBrush(splatterColor, splatterBrush.GetAddressOf());
            
            if (FAILED(hr)) {
                return Result::Error(Result::ErrorCode::DirectXError, 
                                   "Failed to create splatter brush at index " + std::to_string(i));
            }
            
            splatterOpacityBrushes_.emplace_back(std::move(splatterBrush));
        }
        
        return Result::Success();
        
    } catch (const std::exception& e) {
        return Result::Error(Result::ErrorCode::ResourceAllocationFailed, e.what());
    } catch (...) {
        return Result::Error(Result::ErrorCode::ResourceAllocationFailed, 
                           "Unknown error in CreateSplatterBrushes");
    }
}

Result DisplayData::SetSceneBounds(const RECT& sceneRect, const float scaleFactor) noexcept {
    try {
        // Check if bounds have changed and deallocate old scene pixels if needed
        if (!IsSameRect(sceneRect_, sceneRect)) {
            scenePixels_.reset(); // Smart pointer automatic cleanup
        }

        sceneRect_ = sceneRect;
        scaleFactor_ = scaleFactor;

        // Calculate normalized rectangle
        sceneRectNorm_ = {
            0, 0,
            sceneRect.right - sceneRect.left,
            sceneRect.bottom - sceneRect.top
        };

        width_ = sceneRect.right - sceneRect.left;
        height_ = sceneRect.bottom - sceneRect.top;

        // Validate dimensions
        if (width_ <= 0 || height_ <= 0) {
            return Result::Error(Result::ErrorCode::InvalidParameter,
                               "Scene dimensions must be positive");
        }

        // Allocate scene pixels if needed
        if (!scenePixels_) {
            const auto totalPixels = static_cast<size_t>(width_) * static_cast<size_t>(height_);
            
            // Check for potential overflow
            const auto maxSizeT = (std::numeric_limits<size_t>::max)();
            if (totalPixels > maxSizeT / sizeof(bool)) {
                return Result::Error(Result::ErrorCode::ResourceAllocationFailed,
                                   "Scene dimensions too large for allocation");
            }
            
            scenePixels_ = std::make_unique<bool[]>(totalPixels);
            
            // Initialize to false (air)
            std::fill_n(scenePixels_.get(), totalPixels, false);
            
            maxSnowHeight_ = height_ - 2;
        }

        // Sync public members after changes
        SyncPublicMembers();
        
        return Result::Success();
        
    } catch (const std::bad_alloc&) {
        return Result::Error(Result::ErrorCode::ResourceAllocationFailed,
                           "Failed to allocate scene pixel buffer");
    } catch (const std::exception& e) {
        return Result::Error(Result::ErrorCode::ResourceAllocationFailed, e.what());
    } catch (...) {
        return Result::Error(Result::ErrorCode::ResourceAllocationFailed,
                           "Unknown error in SetSceneBounds");
    }
}

void DisplayData::SyncPublicMembers() noexcept {
    // Synchronize public members with private ones for backward compatibility
    SceneRect = sceneRect_;
    SceneRectNorm = sceneRectNorm_;
    ScaleFactor = scaleFactor_;
    Width = width_;
    Height = height_;
    MaxSnowHeight = maxSnowHeight_;
    
    // Sync brush references
    DropColorBrush = dropColorBrush_;
    PrebuiltSplatterOpacityBrushes = splatterOpacityBrushes_;
    
    // Sync pointers
    pScenePixels = scenePixels_.get();
    pNoiseGen = noiseGenerator_.get();
}

} // namespace RainEngine