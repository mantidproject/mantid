#ifndef MANTID_KERNEL_EXCEPTION_H_
#define MANTID_KERNEL_EXCEPTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <stdexcept>
#include <string>
#include "MantidKernel/DllConfig.h"

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

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
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
  FileError(const std::string &Description, const std::string &FileName);
  FileError(const FileError &A);
  /// Assignment operator
  FileError &operator=(const FileError &A);
  /// Destructor
  ~FileError() throw() override {}

  const char *what() const throw() override;
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
  /// Destructor
  ~ParseError() throw() override {}

  const char *what() const throw() override;
};

/// Marks code as not implemented yet.
class MANTID_KERNEL_DLL NotImplementedError final : public std::logic_error {
public:
  NotImplementedError(const std::string & /*Desc*/);
  NotImplementedError(const NotImplementedError &A);
  /// Assignment operator
  NotImplementedError &operator=(const NotImplementedError &A);
  /// Destructor
  ~NotImplementedError() throw() override {}

  const char *what() const throw() override;
};

/// Exception for when an item is not found in a collection.
class MANTID_KERNEL_DLL NotFoundError final : public std::runtime_error {
private:
  /// The name of the search object
  const std::string objectName;
  /// The message returned by what()
  std::string outMessage;

public:
  NotFoundError(const std::string & /*Desc*/,
                const std::string & /*ObjectName*/);
  NotFoundError(const std::string & /*Desc*/, const int & /*ObjectNum*/);
  NotFoundError(const std::string & /*Desc*/, const int64_t & /*ObjectNum*/);
  NotFoundError(const std::string & /*Desc*/,
                const std::size_t & /*ObjectNum*/);
  NotFoundError(const NotFoundError &A);
  /// Assignment operator
  NotFoundError &operator=(const NotFoundError &A);
  /// Destructor
  ~NotFoundError() throw() override {}

  const char *what() const throw() override;
};

/// Exception for when an item is already in a collection.
class MANTID_KERNEL_DLL ExistsError final : public std::runtime_error {
private:
  /// The name of the search object
  const std::string objectName;
  /// The message returned by what()
  std::string outMessage;

public:
  ExistsError(const std::string & /*Desc*/, const std::string & /*ObjectName*/);
  ExistsError(const ExistsError &A);
  /// Assignment operator
  ExistsError &operator=(const ExistsError &A);
  /// Destructor
  ~ExistsError() throw() override {}

  const char *what() const throw() override;
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
  AbsObjMethod(const std::string & /*ObjectName*/);
  AbsObjMethod(const AbsObjMethod &A);
  /// Assignment operator
  AbsObjMethod &operator=(const AbsObjMethod &A);
  /// Destructor
  ~AbsObjMethod() throw() override {}

  const char *what() const throw() override;
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
  InstrumentDefinitionError(const std::string & /*Desc*/,
                            const std::string & /*ObjectName*/);
  InstrumentDefinitionError(const std::string & /*Desc*/);
  InstrumentDefinitionError(const InstrumentDefinitionError &A);
  /// Assignment operator
  InstrumentDefinitionError &operator=(const InstrumentDefinitionError &A);
  /// Destructor
  ~InstrumentDefinitionError() throw() override {}

  const char *what() const throw() override;
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
  OpenGLError(const std::string & /*Desc*/, const std::string & /*ObjectName*/);
  OpenGLError(const std::string & /*Desc*/);
  OpenGLError(const OpenGLError &A);
  /// Assignment operator
  OpenGLError &operator=(const OpenGLError &A);
  /// Destructor
  ~OpenGLError() throw() override {}

  const char *what() const throw() override;
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
  MisMatch(const T & /*A*/, const T & /*B*/, const std::string & /*Place*/);
  MisMatch(const MisMatch<T> &A);
  MisMatch<T> &operator=(const MisMatch<T> &rhs);
  ~MisMatch() throw() override {}

  /// Overloaded reporting method
  const char *what() const throw() override;
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
  ~IndexError() throw() override {}

  /// Overloaded reporting method
  const char *what() const throw() override;
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
  NullPointerException(const NullPointerException & /*rhs*/);
  ~NullPointerException() throw() override {}

  NullPointerException &operator=(const NullPointerException &);

  /// Overloaded reporting method
  const char *what() const throw() override;
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
  InternetError(const InternetError & /*A*/);
  ~InternetError() throw() override {}

  InternetError &operator=(const InternetError &);

  /// Overloaded reporting method
  const char *what() const throw() override;
  const int &errorCode() const;
};

} // namespace Exception
} // namespace Kernel
} // namespace Mantid

#endif // MANTID_KERNEL_EXCEPTION_H_
