#include "utils.h"

 std::vector<std::string> split (const std::string& source, const std::string& delimiter){
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = source.find (delimiter, pos_start)) != std::string::npos) {
        token = source.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (source.substr (pos_start));
    return res;
}

std::vector<std::string> splitMultiDelims(const std::string& source, std::vector<char>& captured_delims, const std::string& delimiters) {
        std::size_t prev = 0;
        std::size_t currentPos = 0;
        std::vector<std::string> results;
 
        while ((currentPos = source.find_first_of(delimiters, prev)) != std::string::npos) {
            captured_delims.push_back(source[currentPos]);
            if (currentPos > prev) {
                results.push_back(source.substr(prev, currentPos - prev));
            }
            prev = currentPos + 1;
        }
        if (prev < source.length()) {
            results.push_back(source.substr(prev));
        }
        return results;
}

int calculate(const int& a, const int& b, const char& op) {
    switch (op)
    {
        case '+': return a+b;
        case '-': return a-b;
        case '/': return a/b;
        case '*': return a*b;
    }
    return a;
}

bool is_number(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

int flags_to_int(bool* arr, int size) {
    int result = 0;
    size = (size<32)? size: 32;
    for(int i=0; i<size; i++) {
        result |= arr[i] << (size-i-1);
    }
    return result;
}