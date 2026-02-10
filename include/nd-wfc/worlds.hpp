#pragma once

#include <array>
#include <tuple>
#include <vector>

namespace WFC {

/**
 * @brief 2D Array World implementation
 */
template<typename T, size_t Width, size_t Height, bool Looping = false>
class Array2D {
public:
    using ValueType = T;
    using CoordType = std::tuple<int, int>;

    constexpr Array2D() = default;

    /**
     * @brief Get the total size of the world
     */
    constexpr size_t size() const {
        return Width * Height;
    }

    /**
     * @brief Convert coordinates to cell ID
     */
    constexpr int getId(CoordType coord) const {
        auto [x, y] = coord;
        return y * Width + x;
    }

    /**
     * @brief Convert cell ID to coordinates
     */
    constexpr CoordType getCoord(int id) const {
        int x = id % Width;
        int y = id / Width;
        return {x, y};
    }

    constexpr size_t GetCoordOffsetX(int x, int dx) const {
        if constexpr (Looping) {
            return (x + dx + Width) % Width;
        } else {
            return static_cast<size_t>(std::clamp(x + dx, 0, static_cast<int>(Width) - 1));
        }
    }

    constexpr size_t GetCoordOffsetY(int y, int dy) const {
        if constexpr (Looping) {
            return (y + dy + Width) % Width;
        } else {
            return static_cast<size_t>(std::clamp(y + dy, 0, static_cast<int>(Height) - 1));
        }
    }

    constexpr size_t getCoordOffset(int x, int y, int dx, int dy) const {
        return getId({GetCoordOffsetX(x, dx), GetCoordOffsetY(y, dy)});
    }

    /**
     * @brief Get width of the array
     */
    constexpr size_t width() const { return Width; }

    /**
     * @brief Get height of the array
     */
    constexpr size_t height() const { return Height; }

    /**
     * @brief Access element at coordinates
     */
    constexpr T& at(int x, int y) {
        return data_[y * Width + x];
    }

    /**
     * @brief Access element at coordinates (const)
     */
    constexpr const T& at(int x, int y) const {
        return data_[y * Width + x];
    }

    /**
     * @brief Access element by ID
     */
    constexpr T& operator[](int id) {
        return data_[id];
    }

    /**
     * @brief Access element by ID (const)
     */
    constexpr const T& operator[](int id) const {
        return data_[id];
    }

    /**
     * @brief Set value at specific index (required by WFC)
     */
    constexpr void setValue(size_t index, T value) {
        data_[index] = value;
    }

    /**
     * @brief Get value at specific index
     */
    constexpr T getValue(size_t index) const {
        return data_[index];
    }

private:
    std::array<T, Width * Height> data_{};
};

/**
 * @brief 3D Array World implementation
 */
template<typename T, size_t Width, size_t Height, size_t Depth>
class Array3D {
public:
    using ValueType = T;
    using CoordType = std::tuple<int, int, int>;

    constexpr Array3D() = default;

    /**
     * @brief Get the total size of the world
     */
    constexpr size_t size() const {
        return Width * Height * Depth;
    }

    /**
     * @brief Convert coordinates to cell ID
     */
    constexpr int getId(CoordType coord) const {
        auto [x, y, z] = coord;
        return z * (Width * Height) + y * Width + x;
    }

    /**
     * @brief Convert cell ID to coordinates
     */
    constexpr CoordType getCoord(int id) const {
        int x = id % Width;
        int y = (id / Width) % Height;
        int z = id / (Width * Height);
        return {x, y, z};
    }

    /**
     * @brief Access element at coordinates
     */
    constexpr T& at(int x, int y, int z) {
        return data_[z * (Width * Height) + y * Width + x];
    }

    /**
     * @brief Access element at coordinates (const)
     */
    constexpr const T& at(int x, int y, int z) const {
        return data_[z * (Width * Height) + y * Width + x];
    }

    /**
     * @brief Access element by ID
     */
    constexpr T& operator[](int id) {
        return data_[id];
    }

    /**
     * @brief Access element by ID (const)
     */
    constexpr const T& operator[](int id) const {
        return data_[id];
    }

    /**
     * @brief Set value at specific index (required by WFC)
     */
    constexpr void setValue(size_t index, T value) {
        data_[index] = value;
    }

    /**
     * @brief Get value at specific index
     */
    constexpr T getValue(size_t index) const {
        return data_[index];
    }

private:
    std::array<T, Width * Height * Depth> data_{};
};

} // namespace WFC
