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
	@param Desc :: Function description
	@param FName :: Filename
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
  return std::logic_error::what();
}

//-------------------------
// NotFoundError
//-------------------------
/** Constructor
	@param Desc :: Function description
	@param ObjectName :: The name of the search object
*/
NotFoundError::NotFoundError(const std::string& Desc,const std::string& ObjectName) :
std::runtime_error(Desc),objectName(ObjectName)
{
	outMessage = std::string(std::runtime_error::what()) + " search object " + objectName;
}

/** Constructor
	@param Desc :: Function description
	@param ObjectNum :: The integer search object
*/
NotFoundError::NotFoundError(const std::string& Desc,const int& ObjectNum) :
std::runtime_error(Desc)
{
  std::stringstream ss;
  std::string obName;
  ss << ObjectNum;
  ss >> obName;
	outMessage = std::string(std::runtime_error::what()) + " search object " + obName;
}

NotFoundError::NotFoundError(const std::string& Desc,const int64_t& ObjectNum) :
std::runtime_error(Desc)
{
  std::stringstream ss;
  std::string obName;
  ss << ObjectNum;
  ss >> obName;
  outMessage = std::string(std::runtime_error::what()) + " search object " + obName;
}

NotFoundError::NotFoundError(const std::string& Desc,const std::size_t& ObjectNum) :
std::runtime_error(Desc)
{
  std::stringstream ss;
  std::string obName;
  ss << ObjectNum;
  ss >> obName;
  outMessage = std::string(std::runtime_error::what()) + " search object " + obName;
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
	@param Desc :: Function description
	@param ObjectName :: The name of the search object
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
	@param ObjectName :: The name of the search object
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
	@param Desc :: Function description
	@param ObjectName :: The name of the search object
*/
InstrumentDefinitionError::InstrumentDefinitionError(const std::string& Desc,const std::string& ObjectName) :
std::runtime_error(Desc),objectName(ObjectName)
{
	outMessage = std::string(std::runtime_error::what()) + " search object " 
    + objectName + ". See http://www.mantidproject.org/IDF for IDF syntax.";
}

/** Constructor
	@param Desc :: Function description
*/
InstrumentDefinitionError::InstrumentDefinitionError(const std::string& Desc) :
std::runtime_error(Desc)
{
	outMessage = std::string(std::runtime_error::what());
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


//-------------------------
// OpenGLError
//-------------------------
/** Constructor
	@param Desc :: Function description
	@param ObjectName :: The name of the search object
*/
OpenGLError::OpenGLError(const std::string& Desc,const std::string& ObjectName) :
std::runtime_error(Desc),objectName(ObjectName)
{
	outMessage = std::string(std::runtime_error::what()) + " rendering " + objectName;
}

/** Constructor
	@param Desc :: Function description
*/
OpenGLError::OpenGLError(const std::string& Desc) :
std::runtime_error(Desc)
{
	outMessage = std::string(std::runtime_error::what());
}

/// Copy constructor
OpenGLError::OpenGLError(const OpenGLError& A) :
  std::runtime_error(A),objectName(A.objectName)
{}

/** Writes out the range and limits
	@returns a char array of foramtted error information
*/
const char* OpenGLError::what() const throw()
{
	return outMessage.c_str();
}


//-------------------------
// MisMatch
//-------------------------

template<typename T>
MisMatch<T>::MisMatch(const T& A,const T& B,const std::string& Place) :
  std::runtime_error(Place),Aval(A),Bval(B)
  /**
    Constructor store two mismatched items
    @param A :: Item to store
    @param B :: Item to store
    @param Place :: Reason/Code item for error
  */
{}

template<typename T>
MisMatch<T>::MisMatch(const MisMatch<T>& A) :
  std::runtime_error(A.what()),Aval(A.Aval),Bval(A.Bval)
   /**
    Copy Constructor
    @param A :: MisMatch to copy
  */
{}

template<typename T>
MisMatch<T> & MisMatch<T>::operator=(const MisMatch<T> &)
{
  /**
    Copy assignment
    @param A :: MisMatch to copy
  */
  throw NotImplementedError("MisMatch<T>::operator=");
}


template<typename T>
const char*
MisMatch<T>::what() const throw()
  /**
    Writes out the two mismatched items
    @return String description of error
  */
{
  std::stringstream cx;
  cx<<std::runtime_error::what()<<" Item A!=B "<<Aval<<
    " "<<Bval<<" ";
  return cx.str().c_str();
}

/// \cond TEMPLATE
template class DLLExport MisMatch<int>;
template class DLLExport MisMatch<size_t>;
/// \endcond TEMPLATE

//-------------------------
// Index Error class
//-------------------------

/**
  Constructor
  @param V :: Value of index
  @param B :: Maximum value
  @param Place :: Location of Error
*/
IndexError::IndexError(const size_t V,const size_t B, const std::string& Place) :
  std::runtime_error(Place),Val(V),maxVal(B)
{}

/**
  Copy Constructor
  @param A :: IndexError to copy
*/
IndexError::IndexError(const IndexError& A) :
  std::runtime_error(A),Val(A.Val),maxVal(A.maxVal)
{}

    /**
    Writes out the range and limits
    @return the error string
  */
const char* IndexError::what() const throw()
{
  std::stringstream cx;
  cx<<"IndexError:"<<std::runtime_error::what()<<" "<<Val<<" :: 0 <==> "<<maxVal;
  return cx.str().c_str();
}

//-------------------------
// NullPointerException class
//-------------------------

/** Constructor
 *  @param place ::      The class & function where the exception occurred
 *  @param objectName :: The name of the pointer
 */
NullPointerException::NullPointerException(const std::string& place, const std::string& objectName) :
  std::runtime_error(place),
  outMessage("Attempt to dereference zero pointer ("+objectName+") in function "+place)
{}

/** Copy Constructor
 *  @param rhs :: The NullPointerException to copy
 */
NullPointerException::NullPointerException(const NullPointerException& rhs) :
  std::runtime_error(rhs), outMessage(rhs.outMessage)
{}

const char* NullPointerException::what() const throw()
{
  return outMessage.c_str();
}

} // namespace Exception
} // namespace Kernel
} // namespace Mantid
