#include "utils.hpp"
#include <algorithm>
#include <cstring>

namespace hm11 {
namespace utils {

std::optional<std::size_t> find(
    const std::uint8_t* buffer, 
    std::size_t size, 
    std::string_view value, 
    std::size_t from
) {
    if (value.empty() || from >= size || value.size() > size - from) {
        return std::nullopt;
    }
    
    for (std::size_t i = from; i <= size - value.size(); ++i) {
        if (std::memcmp(buffer + i, value.data(), value.size()) == 0) {
            return i;
        }
    }
    
    return std::nullopt;
}

bool starts_with(
    const std::uint8_t* buffer, 
    std::size_t size, 
    std::string_view value, 
    std::size_t from
) {
    if (from >= size || value.size() > size - from) {
        return false;
    }
    
    return std::memcmp(buffer + from, value.data(), value.size()) == 0;
}

std::optional<std::size_t> find_zero(
    const std::uint8_t* buffer, 
    std::size_t size, 
    std::size_t from
) {
    if (from >= size) {
        return std::nullopt;
    }
    
    const auto* it = std::find(buffer + from, buffer + size, 0);
    
    if (it == buffer + size) {
        return std::nullopt;
    }
    
    return static_cast<std::size_t>(it - buffer);
}

} // namespace utils
} // namespace hm11