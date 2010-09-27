#include "stdafx.h"
#include "errorMantid.h"


errorMantid::errorMantid(const char *text):
message(text)
{
}
errorMantid::errorMantid(const std::string &text):
message(text)
{
}
errorMantid::~errorMantid(void)
{
}
