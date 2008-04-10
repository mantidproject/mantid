#include <iostream>
#include <sstream>
#include <string>
#include "MantidKernel/Exception.h"


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
	@param Desc Function description
	@param FName Filename 
*/
FileError::FileError(const std::string& Desc,const std::string& FName) :
std::runtime_error(Desc),fileName(FName)
{
	outMessage = std::string(std::runtime_error::what()) + " in " + fileName;
}
 
/// Copy constructor
FileError::FileError(const FileError& A) :
  std::runtime_error(A),fileName(A.fileName)
{}

/** Writes out the range and limits
	@returns a char array of foramtted error information
*/
const char* FileError::what() const throw()
{
	return outMessage.c_str();
}

//-------------------------
// NotImplementedError
//-------------------------
/** Constructor
	@param Desc Function description
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
  return std::logic_error::what();
}

//-------------------------
// NotFoundError
//-------------------------
/** Constructor
	@param Desc Function description
	@param ObjectName The name of the search object
*/
NotFoundError::NotFoundError(const std::string& Desc,const std::string& ObjectName) :
std::runtime_error(Desc),objectName(ObjectName)
{ 
	outMessage = std::string(std::runtime_error::what()) + " search object " + objectName;
}
 
/// Copy constructor
NotFoundError::NotFoundError(const NotFoundError& A) :
  std::runtime_error(A),objectName(A.objectName)
{}

/** Writes out the range and limits
	@returns a char array of foramtted error information
*/
const char* NotFoundError::what() const throw()
{
	return outMessage.c_str();
}

//-------------------------
// ExistsError
//-------------------------
/** Constructor
	@param Desc Function description
	@param ObjectName The name of the search object
*/
ExistsError::ExistsError(const std::string& Desc,const std::string& ObjectName) :
std::runtime_error(Desc),objectName(ObjectName)
{
	outMessage = std::string(std::runtime_error::what()) + " search object " + objectName;
}
 
/// Copy constructor
ExistsError::ExistsError(const ExistsError& A) :
  std::runtime_error(A),objectName(A.objectName)
{}

/** Writes out the range and limits
	@returns a char array of foramtted error information
*/
const char* ExistsError::what() const throw()
{
	return outMessage.c_str();
}

//-------------------------
// AbsObjMethod
//-------------------------
/** Constructor
	@param Desc Function description
	@param ObjectName The name of the search object
*/
AbsObjMethod::AbsObjMethod(const std::string& ObjectName) :
  std::runtime_error(""),objectName(ObjectName)
{
  outMessage = std::string("AbsObjMethod object: ") + objectName;
}
 
/// Copy constructor
AbsObjMethod::AbsObjMethod(const AbsObjMethod& A) :
  std::runtime_error(A),objectName(A.objectName)
{}

/** Writes out the range and limits
	@returns a char array of foramtted error information
*/
const char* AbsObjMethod::what() const throw()
{
  return outMessage.c_str();
}

//-------------------------
// InstrumentDefinitionError
//-------------------------
/** Constructor
	@param Desc Function description
	@param ObjectName The name of the search object
*/
InstrumentDefinitionError::InstrumentDefinitionError(const std::string& Desc,const std::string& ObjectName) :
std::runtime_error(Desc),objectName(ObjectName)
{
	outMessage = std::string(std::runtime_error::what()) + " search object " + objectName;
}
 
/// Copy constructor
InstrumentDefinitionError::InstrumentDefinitionError(const InstrumentDefinitionError& A) :
  std::runtime_error(A),objectName(A.objectName)
{}

/** Writes out the range and limits
	@returns a char array of foramtted error information
*/
const char* InstrumentDefinitionError::what() const throw()
{
	return outMessage.c_str();
}


} // namespace Exception
} // namespace Kernel
} // namespace Mantid
