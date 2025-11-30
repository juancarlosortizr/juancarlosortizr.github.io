#ifndef SQUARE_UTILS_H
#define SQUARE_UTILS_H

#include <stdexcept>
#include <string>

namespace square_utils {

inline char file_char(int x) {
    if (x < 0 || x > 7) throw std::runtime_error("Invalid file");
    return static_cast<char>('a' + x);
}

inline char rank_char(int y) {
    if (y < 0 || y > 7) throw std::runtime_error("Invalid rank");
    return static_cast<char>('1' + y);
}

inline std::string square_to_string(int x, int y) {
    std::string s(2, 'a');
    s[0] = file_char(x);
    s[1] = rank_char(y);
    return s;
}

} // namespace square_utils

#endif // SQUARE_UTILS_H
