#ifndef MANTID_KERNEL_EXCEPTION_H_
#define MANTID_KERNEL_EXCEPTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <stdexcept>
#include <System.h>

namespace Mantid
{
namespace Kernel
{
/** @class Exception Exception.h Kernel/Exception.h

    The Exception classes provide an exception strucure to be used throughout Mantid.
	All exceptions inherit from std:exception.
    
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
class DLLExport Exception: public std::exception
{
public:
	/// Creates an exception.
	Exception(const std::string& msg);
	/// Creates an exception.
	Exception(const std::string& msg, const std::string& arg);
	/// Creates an exception and stores a clone of the nested exception.
	Exception(const std::string& msg, const Exception& nested);

	/// Copy constructor.
	Exception(const Exception& exc);

	/// Destroys the exception and deletes the nested exception.	
	~Exception() throw();
	/// Assignment operator.
	Exception& operator = (const Exception& exc);
	/// Returns a static string describing the exception.	
	virtual const char* name() const throw();
	/// Returns the name of the exception class.	
	virtual const char* className() const throw();
	/// Returns a static string describing the exception.
	///
	/// Same as name(), but for compatibility with std::exception.
	virtual const char* what() const throw();
	/// Returns a pointer to the nested exception, or
	/// null if no nested exception exists.
	const Exception* nested() const;

	/// Returns the message text.			
	const std::string& message() const;

	/// Returns a string consisting of the
	/// message name and the message text.		
	std::string displayText() const;

	/// Clones the exception
	virtual Exception* clone() const;

	/// (Re)Throws the exception.
	virtual void rethrow() const;

protected:
	/// Standard constructor.
	Exception();

		
private:
	std::string _msg;
	Exception*  _pNested;
};


//
// Macros for quickly declaring and implementing exception classes.
// Unfortunately, we cannot use a template here because character
// pointers (which we need for specifying the exception name)
// are not allowed as template arguments.
//
#define MANTID_DECLARE_EXCEPTION(API, CLS, BASE) \
	class API CLS: public BASE											\
	{																	\
	public:																\
		CLS();															\
		CLS(const std::string& msg);									\
		CLS(const std::string& msg, const std::string& arg);			\
		CLS(const std::string& msg, const Mantid::Kernel::Exception& exc);		\
		CLS(const CLS& exc);											\
		~CLS() throw();													\
		CLS& operator = (const CLS& exc);								\
		const char* name() const throw();								\
		const char* className() const throw();							\
		Mantid::Kernel::Exception* clone() const;									\
		void rethrow() const;											\
	};


#define MANTID_IMPLEMENT_EXCEPTION(CLS, BASE, NAME) \
	CLS::CLS()																			\
	{																					\
	}																					\
	CLS::CLS(const std::string& msg): BASE(msg)											\
	{																					\
	}																					\
	CLS::CLS(const std::string& msg, const std::string& arg): BASE(msg, arg)			\
	{																					\
	}																					\
	CLS::CLS(const std::string& msg, const Mantid::Kernel::Exception& exc): BASE(msg, exc)		\
	{																					\
	}																					\
	CLS::CLS(const CLS& exc): BASE(exc)													\
	{																					\
	}																					\
	CLS::~CLS() throw()																	\
	{																					\
	}																					\
	CLS& CLS::operator = (const CLS& exc)												\
	{																					\
		BASE::operator = (exc);															\
		return *this;																	\
	}																					\
	const char* CLS::name() const throw()												\
	{																					\
		return NAME;																	\
	}																					\
	const char* CLS::className() const throw()											\
	{																					\
		return typeid(*this).name();													\
	}																					\
	Mantid::Kernel::Exception* CLS::clone() const													\
	{																					\
		return new CLS(*this);															\
	}																					\
	void CLS::rethrow() const															\
	{																					\
		throw *this;																	\
	}


//
// Standard exception classes
//
MANTID_DECLARE_EXCEPTION(DLLExport, LogicException, Exception)
MANTID_DECLARE_EXCEPTION(DLLExport, AssertionViolationException, LogicException)
MANTID_DECLARE_EXCEPTION(DLLExport, NullPointerException, LogicException)
MANTID_DECLARE_EXCEPTION(DLLExport, InvalidArgumentException, LogicException)
MANTID_DECLARE_EXCEPTION(DLLExport, NotImplementedException, LogicException)
MANTID_DECLARE_EXCEPTION(DLLExport, RangeException, LogicException)

MANTID_DECLARE_EXCEPTION(DLLExport, RuntimeException, Exception)
MANTID_DECLARE_EXCEPTION(DLLExport, NotFoundException, RuntimeException)
MANTID_DECLARE_EXCEPTION(DLLExport, ExistsException, RuntimeException)
MANTID_DECLARE_EXCEPTION(DLLExport, TimeoutException, RuntimeException)
MANTID_DECLARE_EXCEPTION(DLLExport, SystemException, RuntimeException)
MANTID_DECLARE_EXCEPTION(DLLExport, LibraryLoadException, RuntimeException)
MANTID_DECLARE_EXCEPTION(DLLExport, LibraryAlreadyLoadedException, RuntimeException)
MANTID_DECLARE_EXCEPTION(DLLExport, NoPermissionException, RuntimeException)
MANTID_DECLARE_EXCEPTION(DLLExport, OutOfMemoryException, RuntimeException)
MANTID_DECLARE_EXCEPTION(DLLExport, DataException, RuntimeException)

MANTID_DECLARE_EXCEPTION(DLLExport, DataFormatException, DataException)
MANTID_DECLARE_EXCEPTION(DLLExport, SyntaxException, DataException)
MANTID_DECLARE_EXCEPTION(DLLExport, PathSyntaxException, SyntaxException)
MANTID_DECLARE_EXCEPTION(DLLExport, IOException, RuntimeException)
MANTID_DECLARE_EXCEPTION(DLLExport, FileException, IOException)
MANTID_DECLARE_EXCEPTION(DLLExport, FileExistsException, FileException)
MANTID_DECLARE_EXCEPTION(DLLExport, FileNotFoundException, FileException)
MANTID_DECLARE_EXCEPTION(DLLExport, PathNotFoundException, FileException)
MANTID_DECLARE_EXCEPTION(DLLExport, FileReadOnlyException, FileException)
MANTID_DECLARE_EXCEPTION(DLLExport, FileAccessDeniedException, FileException)
MANTID_DECLARE_EXCEPTION(DLLExport, CreateFileException, FileException)
MANTID_DECLARE_EXCEPTION(DLLExport, OpenFileException, FileException)
MANTID_DECLARE_EXCEPTION(DLLExport, WriteFileException, FileException)
MANTID_DECLARE_EXCEPTION(DLLExport, ReadFileException, FileException)

MANTID_DECLARE_EXCEPTION(DLLExport, ApplicationException, Exception)
MANTID_DECLARE_EXCEPTION(DLLExport, BadCastException, RuntimeException)


} // namespace Kernel
} // namespace Mantid

#endif // MANTID_KERNEL_EXCEPTION_H_
