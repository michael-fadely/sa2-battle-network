#ifndef SPLITSTRING_H
#define SPLITSTRING_H

#include <vector>
#include <string>

void SplitString(std::vector<std::string>& out, std::string in, char delim);
std::vector<std::string> SplitString(std::string in, char delim);

#endif // SPLITSTRING_H