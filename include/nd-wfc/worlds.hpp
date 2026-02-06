#pragma once

#include <array>
#include <tuple>
#include <vector>

namespace WFC {

/**
 * @brief 2D Array World implementation
 */
template<typename T, size_t Width, size_t Height>
class Array2D {
public:
    using ValueType = T;
    using CoordType = std::tuple<int, int>;

    Array2D() = default;

    /**
     * @brief Get the total size of the world
     */
    size_t size() const {
        return Width * Height;
    }

    /**
     * @brief Convert coordinates to cell ID
     */
    int getId(CoordType coord) const {
        auto [x, y] = coord;
        return y * Width + x;
    }

    /**
     * @brief Convert cell ID to coordinates
     */
    CoordType getCoord(int id) const {
        int x = id % Width;
        int y = id / Width;
        return {x, y};
    }

    /**
     * @brief Get width of the array
     */
    size_t width() const { return Width; }

    /**
     * @brief Get height of the array
     */
    size_t height() const { return Height; }

    /**
     * @brief Access element at coordinates
     */
    T& at(int x, int y) {
        return data_[y * Width + x];
    }

    /**
     * @brief Access element at coordinates (const)
     */
    const T& at(int x, int y) const {
        return data_[y * Width + x];
    }

    /**
     * @brief Access element by ID
     */
    T& operator[](int id) {
        return data_[id];
    }

    /**
     * @brief Access element by ID (const)
     */
    const T& operator[](int id) const {
        return data_[id];
    }

    /**
     * @brief Set value at specific index (required by WFC)
     */
    void setValue(size_t index, T value) {
        data_[index] = value;
    }

    /**
     * @brief Get value at specific index
     */
    T getValue(size_t index) const {
        return data_[index];
    }

private:
    std::array<T, Width * Height> data_;
};

/**
 * @brief 3D Array World implementation
 */
template<typename T, size_t Width, size_t Height, size_t Depth>
class Array3D {
public:
    using ValueType = T;
    using CoordType = std::tuple<int, int, int>;

    Array3D() = default;

    /**
     * @brief Get the total size of the world
     */
    size_t size() const {
        return Width * Height * Depth;
    }

    /**
     * @brief Convert coordinates to cell ID
     */
    int getId(CoordType coord) const {
        auto [x, y, z] = coord;
        return z * (Width * Height) + y * Width + x;
    }

    /**
     * @brief Convert cell ID to coordinates
     */
    CoordType getCoord(int id) const {
        int x = id % Width;
        int y = (id / Width) % Height;
        int z = id / (Width * Height);
        return {x, y, z};
    }

    /**
     * @brief Access element at coordinates
     */
    T& at(int x, int y, int z) {
        return data_[z * (Width * Height) + y * Width + x];
    }

    /**
     * @brief Access element at coordinates (const)
     */
    const T& at(int x, int y, int z) const {
        return data_[z * (Width * Height) + y * Width + x];
    }

    /**
     * @brief Access element by ID
     */
    T& operator[](int id) {
        return data_[id];
    }

    /**
     * @brief Access element by ID (const)
     */
    const T& operator[](int id) const {
        return data_[id];
    }

    /**
     * @brief Set value at specific index (required by WFC)
     */
    void setValue(size_t index, T value) {
        data_[index] = value;
    }

    /**
     * @brief Get value at specific index
     */
    T getValue(size_t index) const {
        return data_[index];
    }

private:
    std::array<T, Width * Height * Depth> data_;
};

} // namespace WFC

