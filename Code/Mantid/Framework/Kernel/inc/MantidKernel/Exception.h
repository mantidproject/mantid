#ifndef MANTID_KERNEL_EXCEPTION_H_
#define MANTID_KERNEL_EXCEPTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <stdexcept>
#include "MantidKernel/System.h"

namespace Mantid
{
namespace Kernel
{
/**
	The exception classes used by Mantid.
	All exceptions inherit from std:exception.

	The exception tree is
	<ul>
		<li><b>std::exception</b> - Superclass of all standard exception, never thrown itself
		<ul>
			<li><b>std::bad_alloc</b> - Thrown when new runs out of memory</li>
			<li><b>std::bad_exception</b> - Thrown if an exception is thrown which is not listed in a function's exception specification.</li>
			<li><b>std::bad_cast</b> - Thrown if you attempt an invalid dynamic_cast expression</li>
			<li><b>std::bad_typeid</b> - Thrown if you use a NULL pointer in a typeid expression</li>
			<li><b>std::logic_error</b> - Superclass for all logic errors, never thrown itself. Logic errors represent problems in the internal logic of a program; in theory, these are preventable, and even detectable before the program runs (e.g., violations of class invariants).
			<ul>
				<li><b>std::length_error</b> - Thrown when an object is constructed that would exceed its maximum
				permitted size (e.g., a string instance).</li>
				<li><b>std::domain_error</b> - Thrown to report domain errors (domain in the mathematical sense).</li>
				<li><b>std::out_of_range</b> - Thrown if an argument has a value which is not within the expected range
				(e.g. boundary checks in string).</li>
				<li><b>std::invalid_argument</b> - Thrown to report invalid arguments to functions.</li>
				<li><b>NotImplementedError</b> - Thrown if accessing areas of code that are not implmented yet.</li>
			</ul>
			</li>
			<li><b>std::runtime_error</b> - Superclass for all runtime errors, never thrown itself. Runtime errors represent problems outside the scope of a program; they cannot be easily predicted and can generally only be caught as the program executes.
			<ul>
				<li><b>std::range_error</b> - Thrown to indicate range errors in internal computations.</li>
				<li><b>std::overflow_error</b> - Thrown to indicate arithmetic overflow.</li>
				<li><b>std::underflow_error</b> - Thrown to indicate arithmetic underflow.</li>
				<li><b>FileError</b> - Thrown to indicate errors with file operations.</li>
				<li><b>NotFoundError</b> - Thrown to indicate that an item was not found in a collection.</li>
				<li><b>ExistsError</b> - Thrown to indicate that an item was is already found in a collection.</li>
        <li><b>InstrumentDefinitionError</b> - Thrown to indicate a problem with the instrument definition.</li>
        <li><b>MisMatch</b> - Error when two numbers should be identical (or close).</li>
        <li><b>IndexError</b> - Error when an incorrect index value is given.</li>
        <li><b>NullPointerException</b> - Thrown when a zero pointer is dereferenced.</li>
			</ul>
			</li>
		</ul>
		</li>
	</ul>

    @author Nick Draper, Tessella Support Services plc
    @date 8/11/2007

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
	  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
namespace Exception
{

/// Records the filename and the description of failure.
class
#ifdef IN_MANTID_KERNEL
DLLExport
#else
DLLImport
#endif /* IN_MANTID_KERNEL */
FileError : public std::runtime_error
{
 private:
  /// The name of the file relating to the error
  const std::string fileName;
  /// The message returned by what()
  std::string outMessage;

 public:
  FileError(const std::string& Description,const std::string& FileName);
  FileError(const FileError& A);
  /// Assignment operator
  FileError& operator=(const FileError& A);
  /// Destructor
  ~FileError() throw() {}

  const char* what() const throw();
};

/// Marks code as not implemented yet.
class
#ifdef IN_MANTID_KERNEL
DLLExport
#else
DLLImport
#endif /* IN_MANTID_KERNEL */
NotImplementedError : public std::logic_error
{
 public:
  NotImplementedError(const std::string&);
  NotImplementedError(const NotImplementedError& A);
  /// Assignment operator
  NotImplementedError& operator=(const NotImplementedError& A);
  /// Destructor
  ~NotImplementedError() throw() {}

  const char* what() const throw();
};

/// Exception for when an item is not found in a collection.
class
#ifdef IN_MANTID_KERNEL
DLLExport
#else
DLLImport
#endif /* IN_MANTID_KERNEL */
NotFoundError : public std::runtime_error
{
 private:
  /// The name of the search object
  const std::string objectName;
  /// The message returned by what()
  std::string outMessage;

 public:
  NotFoundError(const std::string&,const std::string&);
  NotFoundError(const std::string&,const int&);
  NotFoundError(const std::string&,const int64_t&);
  NotFoundError(const std::string&,const std::size_t&);
  NotFoundError(const NotFoundError& A);
  /// Assignment operator
  NotFoundError& operator=(const NotFoundError& A);
  /// Destructor
  ~NotFoundError() throw() {}

  const char* what() const throw();
};

/// Exception for when an item is already in a collection.
class
#ifdef IN_MANTID_KERNEL
DLLExport
#else
DLLImport
#endif /* IN_MANTID_KERNEL */
ExistsError : public std::runtime_error
{
 private:
  /// The name of the search object
  const std::string objectName;
  /// The message returned by what()
  std::string outMessage;

 public:
  ExistsError(const std::string&,const std::string&);
  ExistsError(const ExistsError& A);
  /// Assignment operator
  ExistsError& operator=(const ExistsError& A);
  /// Destructor
  ~ExistsError() throw() {}

  const char* what() const throw();
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

class
#ifdef IN_MANTID_KERNEL
DLLExport
#else
DLLImport
#endif /* IN_MANTID_KERNEL */
AbsObjMethod : public std::runtime_error
{
 private:
  /// The name of the search object
  const std::string objectName;
  /// The message returned by what()
  std::string outMessage;

 public:
  AbsObjMethod(const std::string&);
  AbsObjMethod(const AbsObjMethod& A);
  /// Assignment operator
  AbsObjMethod& operator=(const AbsObjMethod& A);
  /// Destructor
  ~AbsObjMethod() throw() {}

  const char* what() const throw();
};

/// Exception for errors associated with the instrument definition.
/// This might e.g. occur while reading the instrument definition file.
class DLLExport InstrumentDefinitionError : public std::runtime_error
{
 private:
  /// The name of the search object
  const std::string objectName;
  /// The message returned by what()
  std::string outMessage;

 public:
  InstrumentDefinitionError(const std::string&,const std::string&);
  InstrumentDefinitionError(const std::string&);
  InstrumentDefinitionError(const InstrumentDefinitionError& A);
  /// Assignment operator
  InstrumentDefinitionError& operator=(const InstrumentDefinitionError& A);
  /// Destructor
  ~InstrumentDefinitionError() throw() {}

  const char* what() const throw();
};

/**
 * OpenGL Exception
 */
class DLLExport OpenGLError: public std::runtime_error
{
 private:
  /// The name of the search object
  const std::string objectName;
  /// The message returned by what()
  std::string outMessage;

 public:
  OpenGLError(const std::string&,const std::string&);
  OpenGLError(const std::string&);
  OpenGLError(const OpenGLError& A);
  /// Assignment operator
  OpenGLError& operator=(const OpenGLError& A);
  /// Destructor
  ~OpenGLError() throw() {}

  const char* what() const throw();
};
  /**
  \class MisMatch
  \brief Error when two numbers should be identical (or close)
  \date October 2005
  \version 1.0

  Records the object being looked for
  and the range required.
  */
  template<typename T>
  class MisMatch : public std::runtime_error
  {
  private:

    const T Aval;        ///< Number A
    const T Bval;        ///< container size

  public:

    MisMatch(const T&,const T&,const std::string&);


    MisMatch(const MisMatch<T>& A);
    ~MisMatch() throw() {}

    /// Overloaded reporting method
    const char* what() const throw();

  };


  /**
  \class IndexError
  \brief Exception for index errors
  \author Stuart Ansell
  \date Sept 2005
  \version 1.0

  Called when an index falls out of range

  */
  class DLLExport IndexError : public std::runtime_error
  {
  private:

    const size_t Val;     ///< Actual value called
    const size_t maxVal;  ///< Maximum value

  public:
    // Unsigned versions
    IndexError(const size_t V, const size_t B, const std::string& Place);
    IndexError(const IndexError& A);
    ~IndexError() throw() {}

    /// Overloaded reporting method
    const char* what() const throw();

  };

  /** Exception thrown when an attempt is made to dereference a null pointer
   *
   *  @author Russell Taylor, Tessella Support Services plc
   *  @date 01/07/2008
   */
  class
  #ifdef IN_MANTID_KERNEL
    DLLExport
  #else
    DLLImport
  #endif /* IN_MANTID_KERNEL */
  NullPointerException : public std::runtime_error
  {
  private:
    /// The message returned by what()
    const std::string outMessage;
  public:
    NullPointerException(const std::string& place, const std::string& objectName);
    NullPointerException(const NullPointerException&);
    ~NullPointerException() throw() {}

    /// Overloaded reporting method
    const char* what() const throw();
  };

} //namespace Exception
} // namespace Kernel
} // namespace Mantid

#endif // MANTID_KERNEL_EXCEPTION_H_
