#pragma once

#include <WinkEngine/GFX/Texture2D.hpp>

namespace Wink::GFX
{
    class Texture2DArray
    {
    public:
        bool hotReloadEnabled = true;

    public:
        Texture2DArray();
        ~Texture2DArray();

        DISABLE_COPY(Texture2DArray);
        MOVE_CTOR(Texture2DArray) noexcept;
        MOVE_ASSIGN(Texture2DArray) noexcept;

        void upload(const u8* pixels,
            u32 width, u32 height, u32 layers,
            u32 channels = 4,
            TextureDataType dataType = TextureDataType::UnsignedByte,
            const Texture2DParams& params = {}) noexcept;

        void upload(const float* pixels,
            u32 width, u32 height, u32 layers,
            u32 channels = 4,
            TextureDataType dataType = TextureDataType::Float,
            const Texture2DParams& params = {}) noexcept;

        void allocate(u32 width, u32 height, u32 layers,
            u32 internalFormat,
            const Texture2DParams& params = {}) noexcept;

        void bind(u32 unit = 0) const noexcept;

        void make_resident() noexcept;
        void make_non_resident() noexcept;

        [[nodiscard]] u32 get_id() const noexcept { return mID; }
        [[nodiscard]] u32 get_width() const noexcept { return mWidth; }
        [[nodiscard]] u32 get_height() const noexcept { return mHeight; }
        [[nodiscard]] u32 get_layers() const noexcept { return mLayers; }
        [[nodiscard]] u64 get_bindless_handle() const noexcept { return mBindlessHandle; }
        [[nodiscard]] bool is_resident() const noexcept { return mResident; }
        [[nodiscard]] bool is_valid() const noexcept { return mID != 0; }

        explicit operator bool() const noexcept { return is_valid(); }

    private:
        void apply_params(const Texture2DParams& params) const noexcept;

    private:
        u32 mID = 0;
        u32 mWidth = 0;
        u32 mHeight = 0;
        u32 mLayers = 0;
        u64 mBindlessHandle = 0;
        bool mResident = false;
    };
}
