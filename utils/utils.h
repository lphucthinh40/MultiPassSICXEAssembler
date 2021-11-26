#include <vector>
#include <string>

std::vector<std::string> split (const std::string& source, const std::string& delimiter);
bool is_number(const std::string& s);
int flags_to_int(bool* arr, int size);
std::vector<std::string> splitMultiDelims(const std::string& source, std::vector<char>& captured_delims, const std::string& delimiters);
int calculate(const int& a, const int& b, const char& op);
