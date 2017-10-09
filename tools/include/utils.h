#ifndef UTILS_H_
#define UTILS_H_

#include <string>
#include <vector>
#include <TH1.h>
#include <TH2.h>
#include <TProfile.h>


/**
 * Utils
 */
inline bool isws(char c, char const * const wstr=" \t\n")
{
    return (strchr(wstr, c) != NULL);
}

inline std::string trim_right(const std::string &s)
{
    std::string b=" \t\n";
    std::string str = s;
    return str.erase(str.find_last_not_of(b) +1);
}

inline std::string trim_left(const std::string &s)
{
    std::string b=" \t\n";
    std::string str = s;
    return str.erase( 0, str.find_first_not_of(b) );
}

inline std::string trim_str(const std::string &s)
{
    std::string str = s;
    return trim_left(trim_right(str) );
}

TH1 *create_h1(const char *, const char *, int, double, double);
TH2 *create_h2(const char *, const char *, int, double, double, int, double, double);
TProfile *create_profile(const char *, const char *, int, double, double, double, double);



#endif /*UTILS_H_*/
