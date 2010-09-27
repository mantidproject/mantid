#ifndef H_ERROR_MANTID_TT
#define H_ERROR_MANTID_TT

#include "stdafx.h"
// The class will be replaced by MANTID Error class

class errorMantid  : std::exception 
{
    std::string message;
public:
//    std::string Contents;
    errorMantid(const char *);
    errorMantid(const std::string &);
    std::string const& get_message(void){return message;}
//
    virtual const char* what() const throw(){
        return message.c_str();
    }

    ~errorMantid(void);
};
#endif