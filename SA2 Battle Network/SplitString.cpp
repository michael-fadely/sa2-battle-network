#include "SplitString.h"
#include <sstream>

void SplitString(std::vector<std::string>& out, std::string in, char delim)
{
	using namespace std;

	stringstream ss(in);

	for (string s; getline(ss, s, delim);)
		out.push_back(s);

	return;
}

std::vector<std::string> SplitString(std::string in, char delim)
{
	using namespace std;

	vector<string> out;
	stringstream ss(in);

	for (string s; getline(ss, s, delim);)
		out.push_back(s);

	return out;
}
