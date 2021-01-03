#ifndef _ERROR_H_
#define _ERROR_H_

#include <iostream>
#include <string>

class Error {
public:
    Error(const std::string& message) {
        std::cerr << message << std::endl;
        exit(1);
    }
};

#endif
