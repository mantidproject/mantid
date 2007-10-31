#ifndef MANTID_STRFUNC_SUPPORT_H_
#define MANTID_STRFUNC_SUPPORT_H_

#include "System.h"
#include <iostream>
#include <sstream>
#include <vector>

namespace Mantid
{
/*!
  \namespace StrFunc
  \brief Holds support functions for strings
  \author S. Ansell
  \date February 2006
  \version 1.0
*/
namespace StrFunc
{
/// determine if a character group exists in a string
DLLExport int confirmStr(const std::string&,const std::string&);
/// Get a word from a string
DLLExport int extractWord(std::string&,const std::string&,const int=4);

/// strip all spaces
DLLExport std::string removeSpace(const std::string&);
/// strip pre/post spaces
DLLExport std::string fullBlock(const std::string&);
/// strip trialling comments
DLLExport void stripComment(std::string&);
/// Determines if a string is only spaces
DLLExport int isEmpty(const std::string&);
/// Get a line and strip comments 
DLLExport std::string getLine(std::istream&,const int= 256);
/// get a part of a long line
DLLExport int getPartLine(std::istream&,std::string&,std::string&,const int= 256);

template<typename T> int convPartNum(const std::string&,T&);

/// Convert a string into a number
template<typename T> int convert(const std::string&,T&);
/// Convert a char* into a number
template<typename T> int convert(const char*,T&);

/// Convert and cut a string
template<typename T> int sectPartNum(std::string&,T&);

/// Convert and cut a string
template<typename T> int section(std::string&,T&);
/// Convert and cut a char* 
template<typename T> int section(char*,T&);

/// Convert and cut a string for MCNPX
template<typename T> int sectionMCNPX(std::string&,T&);

/// Convert a VAX number to x86 little eindien
DLLExport float getVAXnum(const float);

}
}

#endif //MANTID_STRFUNC_SUPPORT_H_

