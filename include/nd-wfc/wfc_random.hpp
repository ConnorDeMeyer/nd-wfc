#pragma once

namespace WFC {

/**
* @brief Default constexpr random selector using a simple seed-based algorithm
* This provides a compile-time random selection that maintains state between calls
*/
template <typename VarT>
class DefaultRandomSelector {
private:
    mutable uint32_t m_seed;

public:
    constexpr explicit DefaultRandomSelector(uint32_t seed = 0x12345678) : m_seed(seed) {}

    constexpr uint32_t rng(uint32_t max) const { 
        m_seed = m_seed * 1103515245 + 12345;
        return m_seed % max; 
    }
};

/**
* @brief Advanced random selector using std::mt19937 and std::uniform_int_distribution
* This provides high-quality randomization for runtime use
*/
template <typename VarT>
class AdvancedRandomSelector {
private:
    std::mt19937& m_rng;

public:
    explicit AdvancedRandomSelector(std::mt19937& rng) : m_rng(rng) {}

    uint32_t rng(uint32_t max) const { 
        std::uniform_int_distribution<uint32_t> dist(0, max);
        return dist(m_rng); 
    }
};

}