#pragma once
#include <exception>
#include <string>

class StringException : public std::exception {
public:
   StringException(std::string const& s) : s(s) {}
   ~StringException() throw () {}
   char const* why() const throw() {
       return s.c_str();
   }
private:
    std::string s;
};

inline void throwIf(bool const cond, StringException const& e) {
    if(cond) {
        throw e;
    }
}
