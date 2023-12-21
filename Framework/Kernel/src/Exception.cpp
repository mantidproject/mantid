// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/Exception.h"
#include <sstream>
#include <utility>

namespace Mantid::Kernel::Exception {
//-------------------------
// FileError
//-------------------------
/** Constructor
        @param Desc :: Function description
        @param FName :: Filename
*/
FileError::FileError(const std::string &Desc, std::string FName)
    : std::runtime_error(Desc), fileName(std::move(FName)) {
  outMessage = std::string(std::runtime_error::what()) + " in \"" + fileName + "\"";
}

/// Copy constructor
FileError::FileError(const FileError &A) : std::runtime_error(A), fileName(A.fileName) {}

/** Writes out the range and limits
        @returns a char array of foramtted error information
*/
const char *FileError::what() const noexcept { return outMessage.c_str(); }

//-------------------------
// ParseError
//-------------------------
ParseError::ParseError(const std::string &desc, const std::string &fileName, const int &lineNumber)
    : FileError(desc, fileName), m_lineNumber(lineNumber) {
  std::stringstream ss;
  ss << FileError::what() << " on line " << m_lineNumber;
  m_outMessage = ss.str();
}

ParseError::ParseError(const ParseError &A) : FileError(A), m_lineNumber(A.m_lineNumber) {}

const char *ParseError::what() const noexcept { return m_outMessage.c_str(); }

//-------------------------
// NotImplementedError
//-------------------------
/** Constructor
        @param Desc :: Function description
*/
NotImplementedError::NotImplementedError(const std::string &Desc) : std::logic_error(Desc) {}

/** Writes out the range and limits
        @returns a char array of foramtted error information
*/
const char *NotImplementedError::what() const noexcept { return std::logic_error::what(); }

//-------------------------
// NotFoundError
//-------------------------
/** Constructor
        @param Desc :: Function description
        @param ObjectName :: The name of the search object
*/
NotFoundError::NotFoundError(const std::string &Desc, std::string ObjectName)
    : std::runtime_error(Desc), objectName(std::move(ObjectName)) {
  outMessage = std::string(std::runtime_error::what()) + " search object '" + objectName + "'";
}

/** Constructor
        @param Desc :: Function description
        @param ObjectNum :: The integer search object
*/
NotFoundError::NotFoundError(const std::string &Desc, const int &ObjectNum) : std::runtime_error(Desc) {
  std::stringstream ss;
  std::string obName;
  ss << ObjectNum;
  ss >> obName;
  outMessage = std::string(std::runtime_error::what()) + " search object '" + obName + "'";
}

NotFoundError::NotFoundError(const std::string &Desc, const int64_t &ObjectNum) : std::runtime_error(Desc) {
  std::stringstream ss;
  std::string obName;
  ss << ObjectNum;
  ss >> obName;
  outMessage = std::string(std::runtime_error::what()) + " search object '" + obName + "'";
}

NotFoundError::NotFoundError(const std::string &Desc, const std::size_t &ObjectNum) : std::runtime_error(Desc) {
  std::stringstream ss;
  std::string obName;
  ss << ObjectNum;
  ss >> obName;
  outMessage = std::string(std::runtime_error::what()) + " search object '" + obName + "'";
}

/// Copy constructor
NotFoundError::NotFoundError(const NotFoundError &A) : std::runtime_error(A), objectName(A.objectName) {}

/** Writes out the range and limits
        @returns a char array of foramtted error information
*/
const char *NotFoundError::what() const noexcept { return outMessage.c_str(); }

//-------------------------
// ExistsError
//-------------------------
/** Constructor
        @param Desc :: Function description
        @param ObjectName :: The name of the search object
*/
ExistsError::ExistsError(const std::string &Desc, std::string ObjectName)
    : std::runtime_error(Desc), objectName(std::move(ObjectName)) {
  outMessage = std::string(std::runtime_error::what()) + " search object '" + objectName + "'";
}

/// Copy constructor
ExistsError::ExistsError(const ExistsError &A) : std::runtime_error(A), objectName(A.objectName) {}

/** Writes out the range and limits
        @returns a char array of foramtted error information
*/
const char *ExistsError::what() const noexcept { return outMessage.c_str(); }

//-------------------------
// AbsObjMethod
//-------------------------
/** Constructor
        @param ObjectName :: The name of the search object
*/
AbsObjMethod::AbsObjMethod(std::string ObjectName) : std::runtime_error(""), objectName(std::move(ObjectName)) {
  outMessage = std::string("AbsObjMethod object: ") + objectName;
}

/// Copy constructor
AbsObjMethod::AbsObjMethod(const AbsObjMethod &A) : std::runtime_error(A), objectName(A.objectName) {}

/** Writes out the range and limits
        @returns a char array of foramtted error information
*/
const char *AbsObjMethod::what() const noexcept { return outMessage.c_str(); }

//-------------------------
// InstrumentDefinitionError
//-------------------------
/** Constructor
        @param Desc :: Function description
        @param ObjectName :: The name of the search object
*/
InstrumentDefinitionError::InstrumentDefinitionError(const std::string &Desc, std::string ObjectName)
    : std::runtime_error(Desc), objectName(std::move(ObjectName)) {
  outMessage = std::string(std::runtime_error::what()) + " while searching for " + objectName +
               ". See http://docs.mantidproject.org/concepts/InstrumentDefinitionFile for IDF syntax.";
}

/** Constructor
        @param Desc :: Function description
*/
InstrumentDefinitionError::InstrumentDefinitionError(const std::string &Desc) : std::runtime_error(Desc) {
  outMessage = std::string(std::runtime_error::what());
}

/// Copy constructor
InstrumentDefinitionError::InstrumentDefinitionError(const InstrumentDefinitionError &A)
    : std::runtime_error(A), objectName(A.objectName) {}

/** Writes out the range and limits
        @returns a char array of foramtted error information
*/
const char *InstrumentDefinitionError::what() const noexcept { return outMessage.c_str(); }

//-------------------------
// OpenGLError
//-------------------------
/** Constructor
        @param Desc :: Function description
        @param ObjectName :: The name of the search object
*/
OpenGLError::OpenGLError(const std::string &Desc, std::string ObjectName)
    : std::runtime_error(Desc), objectName(std::move(ObjectName)) {
  outMessage = std::string(std::runtime_error::what()) + " rendering " + objectName;
}

/** Constructor
        @param Desc :: Function description
*/
OpenGLError::OpenGLError(const std::string &Desc) : std::runtime_error(Desc) {
  outMessage = std::string(std::runtime_error::what());
}

/// Copy constructor
OpenGLError::OpenGLError(const OpenGLError &A) : std::runtime_error(A), objectName(A.objectName) {}

/** Writes out the range and limits
        @returns a char array of foramtted error information
*/
const char *OpenGLError::what() const noexcept { return outMessage.c_str(); }

//-------------------------
// MisMatch
//-------------------------

template <typename T>
MisMatch<T>::MisMatch(const T &A, const T &B, const std::string &Place)
    : std::runtime_error(Place), Aval(A), Bval(B)
/**
  Constructor store two mismatched items
  @param A :: Item to store
  @param B :: Item to store
  @param Place :: Reason/Code item for error
*/
{
  std::stringstream cx;
  cx << Place << " Item A!=B " << Aval << " " << Bval << " ";
  m_message = cx.str();
}

template <typename T>
MisMatch<T>::MisMatch(const MisMatch<T> &A)
    : std::runtime_error(A), Aval(A.Aval), Bval(A.Bval)
/**
 Copy Constructor
 @param A :: MisMatch to copy
*/
{}

template <typename T>
const char *MisMatch<T>::what() const noexcept
/**
  Writes out the two mismatched items
  @return String description of error
*/
{
  return m_message.c_str();
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
IndexError::IndexError(const size_t V, const size_t B, const std::string &Place)
    : std::runtime_error(Place), Val(V), maxVal(B) {
  std::stringstream cx;
  cx << "IndexError: " << Place << " " << Val << " :: 0 <==> " << maxVal;
  m_message = cx.str();
}

/**
  Copy Constructor
  @param A :: IndexError to copy
*/
IndexError::IndexError(const IndexError &A) : std::runtime_error(A), Val(A.Val), maxVal(A.maxVal) {}

/**
  Writes out the range and limits
  @return the error string
*/
const char *IndexError::what() const noexcept { return m_message.c_str(); }

//-------------------------
// NullPointerException class
//-------------------------

/** Constructor
 *  @param place ::      The class & function where the exception occurred
 *  @param objectName :: The name of the pointer
 */
NullPointerException::NullPointerException(const std::string &place, const std::string &objectName)
    : std::runtime_error(place),
      outMessage("Attempt to dereference zero pointer (" + objectName + ") in function " + place) {}

const char *NullPointerException::what() const noexcept { return outMessage.c_str(); }

//-------------------------
// InternetError Error class
//-------------------------

/**
  Constructor
  @param message :: The error message
  @param errorCode :: The HTTP error code if available
*/
InternetError::InternetError(const std::string &message, const int &errorCode) : std::runtime_error(message) {
  std::stringstream cx;
  cx << "InternetError: ";
  if (errorCode != 0) {
    cx << "[" << errorCode << "] ";
  }
  cx << message;
  outMessage = cx.str();
  m_errorCode = errorCode;
}

/**
  Writes out the range and limits
  @return the error string
*/
const char *InternetError::what() const noexcept { return outMessage.c_str(); }

/**
  Writes out the range and limits
  @return the error string
*/
const int &InternetError::errorCode() const { return m_errorCode; }

//-------------------------
// FitSizeError Error class
//-------------------------

/// Constructor.
/// @param oldSize :: Old number of free fitting parameters
FitSizeWarning::FitSizeWarning(size_t oldSize)
    : std::exception(),
      m_message("Number of fitting parameters is different from original value of " + std::to_string(oldSize)) {}

/// Constructor.
/// @param oldSize :: Old number of free fitting parameters
/// @param newSize :: New number of free fitting parameters
FitSizeWarning::FitSizeWarning(size_t oldSize, size_t newSize)
    : std::exception(), m_message("Number of fitting parameters changed from " + std::to_string(oldSize) + " to " +
                                  std::to_string(newSize)) {}

/// Get the warning message.
const char *FitSizeWarning::what() const noexcept { return m_message.c_str(); }

} // namespace Mantid::Kernel::Exception
