// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_EXCEPTION_H_
#define MANTID_KERNEL_EXCEPTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include <exception>
#include <stdexcept>
#include <string>

namespace Mantid {
namespace Kernel {
/**
  The exception classes used by Mantid.
  All exceptions inherit from std:exception.

  The exception tree is
  <ul>
    <li><b>std::exception</b> - Superclass of all standard exception, never
  thrown itself
    <ul>
      <li><b>std::bad_alloc</b> - Thrown when new runs out of memory</li>
      <li><b>std::bad_exception</b> - Thrown if an exception is thrown which is
  not listed in a function's exception specification.</li>
      <li><b>std::bad_cast</b> - Thrown if you attempt an invalid dynamic_cast
  expression</li>
      <li><b>std::bad_typeid</b> - Thrown if you use a NULL pointer in a typeid
  expression</li>
      <li><b>std::logic_error</b> - Superclass for all logic errors, never
  thrown itself. Logic errors represent problems in the internal logic of a
  program; in theory, these are preventable, and even detectable before the
  program runs (e.g., violations of class invariants).
      <ul>
        <li><b>std::length_error</b> - Thrown when an object is constructed that
  would exceed its maximum
        permitted size (e.g., a string instance).</li>
        <li><b>std::domain_error</b> - Thrown to report domain errors (domain in
  the mathematical sense).</li>
        <li><b>std::out_of_range</b> - Thrown if an argument has a value which
  is not within the expected range
        (e.g. boundary checks in string).</li>
        <li><b>std::invalid_argument</b> - Thrown to report invalid arguments to
  functions.</li>
        <li><b>NotImplementedError</b> - Thrown if accessing areas of code that
  are not implmented yet.</li>
      </ul>
      </li>
      <li><b>std::runtime_error</b> - Superclass for all runtime errors, never
  thrown itself. Runtime errors represent problems outside the scope of a
  program; they cannot be easily predicted and can generally only be caught as
  the program executes.
      <ul>
        <li><b>std::range_error</b> - Thrown to indicate range errors in
  internal computations.</li>
        <li><b>std::overflow_error</b> - Thrown to indicate arithmetic
  overflow.</li>
        <li><b>std::underflow_error</b> - Thrown to indicate arithmetic
  underflow.</li>
        <li>
          <b>FileError</b> - Thrown to indicate errors with file operations.
          <ul>
            <li><b>ParseError</b> - Thrown to indicate errors when parsing a
  file.</li>
          </ul>
        </li>
        <li><b>NotFoundError</b> - Thrown to indicate that an item was not found
  in a collection.</li>
        <li><b>ExistsError</b> - Thrown to indicate that an item was is already
  found in a collection.</li>
        <li><b>InstrumentDefinitionError</b> - Thrown to indicate a problem with
  the instrument definition.</li>
        <li><b>MisMatch</b> - Error when two numbers should be identical (or
  close).</li>
        <li><b>IndexError</b> - Error when an incorrect index value is
  given.</li>
        <li><b>NullPointerException</b> - Thrown when a zero pointer is
  dereferenced.</li>
        <li><b>InternetError</b> - Thrown when an error occurs accessing an
  internet resource.</li>
      </ul>
      </li>
    </ul>
    </li>
  </ul>

    @author Nick Draper, Tessella Support Services plc
    @date 8/11/2007
*/
namespace Exception {

/// Records the filename and the description of failure.
class MANTID_KERNEL_DLL FileError : public std::runtime_error {
private:
  /// The name of the file relating to the error
  const std::string fileName;
  /// The message returned by what()
  std::string outMessage;

public:
  FileError(const std::string &Desc, const std::string &FName);
  FileError(const FileError &A);
  /// Assignment operator
  FileError &operator=(const FileError &A);
  const char *what() const noexcept override;
};

/// Records the filename, the description of failure and the line on which it
/// happened
class MANTID_KERNEL_DLL ParseError final : public FileError {
private:
  /// Number of the line where the error occured
  const int m_lineNumber;
  /// The message returned by what()
  std::string m_outMessage;

public:
  /**
   * Constructor
   * @param desc :: Error description
   * @param fileName :: Filename where happened
   * @param lineNumber :: Number of the line where error happened
   */
  ParseError(const std::string &desc, const std::string &fileName,
             const int &lineNumber);
  /// Copy constructor
  ParseError(const ParseError &A);
  /// Assignment operator
  ParseError &operator=(const ParseError &A);
  const char *what() const noexcept override;
};

/// Marks code as not implemented yet.
class MANTID_KERNEL_DLL NotImplementedError final : public std::logic_error {
public:
  NotImplementedError(const std::string &);
  const char *what() const noexcept override;
};

/// Exception for when an item is not found in a collection.
class MANTID_KERNEL_DLL NotFoundError final : public std::runtime_error {
private:
  /// The name of the search object
  const std::string objectName;
  /// The message returned by what()
  std::string outMessage;

public:
  NotFoundError(const std::string &, const std::string &);
  NotFoundError(const std::string &, const int &);
  NotFoundError(const std::string &, const int64_t &);
  NotFoundError(const std::string &, const std::size_t &);
  NotFoundError(const NotFoundError &A);
  /// Assignment operator
  NotFoundError &operator=(const NotFoundError &A);
  const char *what() const noexcept override;
};

/// Exception for when an item is already in a collection.
class MANTID_KERNEL_DLL ExistsError final : public std::runtime_error {
private:
  /// The name of the search object
  const std::string objectName;
  /// The message returned by what()
  std::string outMessage;

public:
  ExistsError(const std::string &, const std::string &);
  ExistsError(const ExistsError &A);
  /// Assignment operator
  ExistsError &operator=(const ExistsError &A);

  const char *what() const noexcept override;
};

/**
  \class AbsObjMethod
  \brief Exception for a call to an abstract class function

  For a method virtual abstract class exists that
  needs to have instance because of interation over
  a base class pointer. e.g
  vector<AbsBase*> Vec;
  for(vc=Vec.begin();vc!=Vec.end();vc++)
     (*vc)->method();

   but: X=new AbsBase() is forbidden by the compiler
   but it always possible to obtain a pure AbsBase pointer
   by casting and this exception is to be placed in these sorts
   of methods in the Abstract class.

   It is a runtime error constructed a runtime reinterpret cast.

   Anyone has a better way of doing this ? Please let me know!!!
   -- Note: I believe this was tabled for Cx00 but rejected (why?)
*/

class MANTID_KERNEL_DLL AbsObjMethod : public std::runtime_error {
private:
  /// The name of the search object
  const std::string objectName;
  /// The message returned by what()
  std::string outMessage;

public:
  AbsObjMethod(const std::string &);
  AbsObjMethod(const AbsObjMethod &A);
  /// Assignment operator
  AbsObjMethod &operator=(const AbsObjMethod &A);

  const char *what() const noexcept override;
};

/// Exception for errors associated with the instrument definition.
/// This might e.g. occur while reading the instrument definition file.
class MANTID_KERNEL_DLL InstrumentDefinitionError : public std::runtime_error {
private:
  /// The name of the search object
  const std::string objectName;
  /// The message returned by what()
  std::string outMessage;

public:
  InstrumentDefinitionError(const std::string &, const std::string &);
  InstrumentDefinitionError(const std::string &);
  InstrumentDefinitionError(const InstrumentDefinitionError &A);
  /// Assignment operator
  InstrumentDefinitionError &operator=(const InstrumentDefinitionError &A);

  const char *what() const noexcept override;
};

/**
 * OpenGL Exception
 */
class MANTID_KERNEL_DLL OpenGLError : public std::runtime_error {
private:
  /// The name of the search object
  const std::string objectName;
  /// The message returned by what()
  std::string outMessage;

public:
  OpenGLError(const std::string &, const std::string &);
  OpenGLError(const std::string &);
  OpenGLError(const OpenGLError &A);
  /// Assignment operator
  OpenGLError &operator=(const OpenGLError &A);

  const char *what() const noexcept override;
};
/**
\class MisMatch
\brief Error when two numbers should be identical (or close)

Records the object being looked for
and the range required.
*/
template <typename T> class MisMatch : public std::runtime_error {
private:
  const T Aval;          ///< Number A
  const T Bval;          ///< container size
  std::string m_message; ///< The message reported by what()

public:
  MisMatch(const T &, const T &, const std::string &);
  MisMatch(const MisMatch<T> &A);
  MisMatch<T> &operator=(const MisMatch<T> &rhs);

  /// Overloaded reporting method
  const char *what() const noexcept override;
};

/**
\class IndexError
\brief Exception for index errors

Called when an index falls out of range
*/
class MANTID_KERNEL_DLL IndexError : public std::runtime_error {
private:
  const size_t Val;      ///< Actual value called
  const size_t maxVal;   ///< Maximum value
  std::string m_message; ///< The message reported by what()

public:
  // Unsigned versions
  IndexError(const size_t V, const size_t B, const std::string &Place);
  IndexError(const IndexError &A);
  IndexError &operator=(const IndexError &A);

  /// Overloaded reporting method
  const char *what() const noexcept override;
};

/** Exception thrown when an attempt is made to dereference a null pointer
 *
 *  @author Russell Taylor, Tessella Support Services plc
 *  @date 01/07/2008
 */
class MANTID_KERNEL_DLL NullPointerException : public std::runtime_error {
private:
  /// The message returned by what()
  const std::string outMessage;

public:
  NullPointerException(const std::string &place, const std::string &objectName);
  /// Overloaded reporting method
  const char *what() const noexcept override;
};

/** Exception thrown when error occurs accessing an internet resource
 *
 *  @author Nick Draper, Tessella
 *  @date 13/11/2013
 */
class MANTID_KERNEL_DLL InternetError final : public std::runtime_error {
private:
  /// The message returned by what()
  std::string outMessage;
  int m_errorCode; ///< The message reported by what()

public:
  InternetError(const std::string &message, const int &errorCode = 0);
  /// Overloaded reporting method
  const char *what() const noexcept override;
  const int &errorCode() const;
};

/// Exception thrown when a fitting function changes number of parameters
/// during fit.
class MANTID_KERNEL_DLL FitSizeWarning final : public std::exception {
  std::string m_message;

public:
  explicit FitSizeWarning(size_t oldSize);
  FitSizeWarning(size_t oldSize, size_t newSize);
  const char *what() const noexcept override;
};

} // namespace Exception
} // namespace Kernel
} // namespace Mantid

#endif // MANTID_KERNEL_EXCEPTION_H_
