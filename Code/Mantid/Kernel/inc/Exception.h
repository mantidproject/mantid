#ifndef MANTID_KERNEL_EXCEPTION_H_
#define MANTID_KERNEL_EXCEPTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <stdexcept>
#include "System.h"

namespace Mantid
{
namespace Kernel
{
/** @file
	The File contains all of the exception classes used by Mantid that are extended from the std:exception classes.
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
			</ul>
			</li>
		</ul>
		</li>
	</ul>
    
    @author Nick Draper, Tessella Support Services plc
    @date 8/11/2007
    
    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratories

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>    
	Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
namespace Exception
{

/// Records the filename and the description of failure.
class DLLExport FileError : public std::runtime_error
{
 private:
  const std::string fileName;
  std::string outMessage;

 public:
  FileError(const std::string&,const std::string&);
  FileError(const FileError& A);
  FileError& operator=(const FileError& A);
  ~FileError() throw() {}

  const char* what() const throw();
};

/// Marks code as not implemented yet.
class DLLExport NotImplementedError : public std::logic_error
{ 
 public:
  NotImplementedError(const std::string&);
  NotImplementedError(const NotImplementedError& A);
  NotImplementedError& operator=(const NotImplementedError& A);
  ~NotImplementedError() throw() {}

  const char* what() const throw();
};

/// Exception for when an item is not found in a collection.
class DLLExport NotFoundError : public std::runtime_error
{
 private:
  const std::string objectName;
  std::string outMessage;

 public:
  NotFoundError(const std::string&,const std::string&);
  NotFoundError(const NotFoundError& A);
  NotFoundError& operator=(const NotFoundError& A);
  ~NotFoundError() throw() {}

  const char* what() const throw();
};

/// Exception for when an item is already in a collection.
class DLLExport ExistsError : public std::runtime_error
{
 private:
  const std::string objectName;
  std::string outMessage;

 public:
  ExistsError(const std::string&,const std::string&);
  ExistsError(const ExistsError& A);
  ExistsError& operator=(const ExistsError& A);
  ~ExistsError() throw() {}

  const char* what() const throw();
};

} //namespace Exception
} // namespace Kernel
} // namespace Mantid

#endif // MANTID_KERNEL_EXCEPTION_H_
