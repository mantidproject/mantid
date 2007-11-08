#include "Exception.h"
#include <typeinfo>


namespace Mantid
{
namespace Kernel
{

/** No Arg Constructor
	Intialises the nested exception to 0
*/
Exception::Exception(): _pNested(0)
{
}

/** Constructor
	@param msg Sets the message that the Exception will carry
*/
Exception::Exception(const std::string& msg): _msg(msg), _pNested(0)
{
}

/** Constructor
	@param msg Sets the message that the Exception will carry
	@param arg message arguments are appeneded to the end of the message with a preceding ': '
*/
Exception::Exception(const std::string& msg, const std::string& arg): _msg(msg), _pNested(0)
{
	if (!arg.empty())
	{
		_msg.append(": ");
		_msg.append(arg);
	}
}

/** Constructor
	@param msg Sets the message that the Exception will carry
	@param nested A nested exception which is cloned and held within this exception
*/
Exception::Exception(const std::string& msg, const Exception& nested): _msg(msg), _pNested(nested.clone())
{
}

/** Constructor
	@param nested A nested exception, the message for this exception is taken from the inner exception
*/
Exception::Exception(const Exception& exc): std::exception(exc)
{
	_msg = exc._msg;
	_pNested = exc._pNested ? exc._pNested->clone() : 0;
}

/// Destructor
Exception::~Exception() throw()
{
	delete _pNested;
}

/** Copy contructor
	@param exc The excption to copy
	@returns The copied exception
*/
Exception& Exception::operator = (const Exception& exc)
{
	if (&exc != this)
	{
		delete _pNested;
		_msg = exc._msg;
		_pNested = exc._pNested ? exc._pNested->clone() : 0;
	}
	return *this;
}

/** Gets the name of the exception
	@returns the name of the exception
*/
const char* Exception::name() const throw()
{
	return "Exception";
}

/** Gets the classname of the exception
	@returns the classname of the exception
*/
const char* Exception::className() const throw()
{
	return typeid(*this).name();
}

/** Gets the name of the exception (compatible with std::exception)
	@returns the name of the exception
*/
const char* Exception::what() const throw()
{
	return name();
}

/** Gets the name and message of the exception formatted for display or logging
	@returns the name and message of the exception
*/
std::string Exception::displayText() const
{
	std::string txt = name();
	if (!_msg.empty())
	{
		txt.append(": ");
		txt.append(_msg);
	}
	return txt;
}

/** Creates an exact copy of the exception.
	The copy can later be thrown again by
	invoking rethrow() on it.
	@returns A pointer to the cloned exception
*/
Exception* Exception::clone() const
{
	return new Exception(*this);
}

/** (Re)Throws the exception.
	This is useful for temporarily storing a
	copy of an exception (see clone()), then
	throwing it again.	
*/
void Exception::rethrow() const
{
	throw *this;
}

/** Gets the nested exception
	@returns A pointer to the nested exception
*/
const Exception* Exception::nested() const
{
	return _pNested;
}

/** gets the message of the exception
	@returns A reference to the message
*/
const std::string& Exception::message() const
{
	return _msg;
}


MANTID_IMPLEMENT_EXCEPTION(LogicException, Exception, "Logic exception")
MANTID_IMPLEMENT_EXCEPTION(AssertionViolationException, LogicException, "Assertion violation")
MANTID_IMPLEMENT_EXCEPTION(NullPointerException, LogicException, "Null pointer")
MANTID_IMPLEMENT_EXCEPTION(InvalidArgumentException, LogicException, "Invalid argument")
MANTID_IMPLEMENT_EXCEPTION(NotImplementedException, LogicException, "Not implemented")
MANTID_IMPLEMENT_EXCEPTION(RangeException, LogicException, "Out of range")

MANTID_IMPLEMENT_EXCEPTION(RuntimeException, Exception, "Runtime exception")
MANTID_IMPLEMENT_EXCEPTION(NotFoundException, RuntimeException, "Not found")
MANTID_IMPLEMENT_EXCEPTION(ExistsException, RuntimeException, "Exists")
MANTID_IMPLEMENT_EXCEPTION(TimeoutException, RuntimeException, "Timeout")
MANTID_IMPLEMENT_EXCEPTION(SystemException, RuntimeException, "System exception")
MANTID_IMPLEMENT_EXCEPTION(LibraryLoadException, RuntimeException, "Cannot load library")
MANTID_IMPLEMENT_EXCEPTION(LibraryAlreadyLoadedException, RuntimeException, "Library already loaded")
MANTID_IMPLEMENT_EXCEPTION(NoPermissionException, RuntimeException, "No permission")
MANTID_IMPLEMENT_EXCEPTION(OutOfMemoryException, RuntimeException, "Out of memory")
MANTID_IMPLEMENT_EXCEPTION(DataException, RuntimeException, "Data error")

MANTID_IMPLEMENT_EXCEPTION(DataFormatException, DataException, "Bad data format")
MANTID_IMPLEMENT_EXCEPTION(SyntaxException, DataException, "Syntax error")
MANTID_IMPLEMENT_EXCEPTION(PathSyntaxException, SyntaxException, "Bad path syntax")
MANTID_IMPLEMENT_EXCEPTION(IOException, RuntimeException, "I/O error")
MANTID_IMPLEMENT_EXCEPTION(FileException, IOException, "File access error")
MANTID_IMPLEMENT_EXCEPTION(FileExistsException, FileException, "File exists")
MANTID_IMPLEMENT_EXCEPTION(FileNotFoundException, FileException, "File not found")
MANTID_IMPLEMENT_EXCEPTION(PathNotFoundException, FileException, "Path not found")
MANTID_IMPLEMENT_EXCEPTION(FileReadOnlyException, FileException, "File is read-only")
MANTID_IMPLEMENT_EXCEPTION(FileAccessDeniedException, FileException, "Access to file denied")
MANTID_IMPLEMENT_EXCEPTION(CreateFileException, FileException, "Cannot create file")
MANTID_IMPLEMENT_EXCEPTION(OpenFileException, FileException, "Cannot open file")
MANTID_IMPLEMENT_EXCEPTION(WriteFileException, FileException, "Cannot write file")
MANTID_IMPLEMENT_EXCEPTION(ReadFileException, FileException, "Cannot read file")


MANTID_IMPLEMENT_EXCEPTION(ApplicationException, Exception, "Application exception")
MANTID_IMPLEMENT_EXCEPTION(BadCastException, RuntimeException, "Bad cast exception")

} // namespace Kernel
} // namespace Mantid
