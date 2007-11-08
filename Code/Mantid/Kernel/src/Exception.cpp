#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include "Exception.h"


namespace Mantid
{
namespace Kernel
{
namespace Exception
{
//-------------------------
// FileError
//-------------------------
/** Constructor
	@param Desc :: Function description
	@param Fname :: Filename 
*/
FileError::FileError(const std::string& Desc,const std::string& FName) :
std::runtime_error(Desc),fileName(FName)
{}
 
/// Copy constructor
FileError::FileError(const FileError& A) :
  std::runtime_error(A),fileName(A.fileName)
{}

/** Writes out the range and limits
	@returns a char array of foramtted error information
*/
const char* FileError::what() const throw()
{
  std::stringstream cx;
  cx<<std::runtime_error::what()<<" in "<<fileName;
  return cx.str().c_str();
}

//-------------------------
// NotImplementedError
//-------------------------
/** Constructor
	@param Desc :: Function description
*/
NotImplementedError::NotImplementedError(const std::string& Desc) :
std::logic_error(Desc)
{}
 
/// Copy constructor
NotImplementedError::NotImplementedError(const NotImplementedError& A) :
  std::logic_error(A)
{}

/** Writes out the range and limits
	@returns a char array of foramtted error information
*/
const char* NotImplementedError::what() const throw()
{
  std::stringstream cx;
  cx<<std::logic_error::what();
  return cx.str().c_str();
}

//-------------------------
// NotFoundError
//-------------------------
/** Constructor
	@param Desc :: Function description
	@param ObjectName :: the name of the search object
*/
NotFoundError::NotFoundError(const std::string& Desc,const std::string& ObjectName) :
std::runtime_error(Desc),objectName(ObjectName)
{}
 
/// Copy constructor
NotFoundError::NotFoundError(const NotFoundError& A) :
  std::runtime_error(A),objectName(A.objectName)
{}

/** Writes out the range and limits
	@returns a char array of foramtted error information
*/
const char* NotFoundError::what() const throw()
{
  std::stringstream cx;
  cx<<std::runtime_error::what()<<" search object "<<objectName;;
  return cx.str().c_str();
}

//-------------------------
// ExistsError
//-------------------------
/** Constructor
	@param Desc :: Function description
	@param ObjectName :: the name of the search object
*/
ExistsError::ExistsError(const std::string& Desc,const std::string& ObjectName) :
std::runtime_error(Desc),objectName(ObjectName)
{}
 
/// Copy constructor
ExistsError::ExistsError(const ExistsError& A) :
  std::runtime_error(A),objectName(A.objectName)
{}

/** Writes out the range and limits
	@returns a char array of foramtted error information
*/
const char* ExistsError::what() const throw()
{
  std::stringstream cx;
  cx<<std::runtime_error::what()<<" search object "<<objectName;;
  return cx.str().c_str();
}

} // namespace Exception
} // namespace Kernel
} // namespace Mantid
