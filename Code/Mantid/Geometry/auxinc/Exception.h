#ifndef Exception_h
#define Exception_h

/*!
  \namespace ColErr
  \brief All the exceptions that make sense to trap
  \author S. Ansell
  \version 1.0
  \date February 2006
  
*/

namespace ColErr
{

/*!
  \class ExBase
  \brief Exception Base class
  \author Stuart Ansell
  \date Sept 2005

  Base class of all exceptions. The main
  virtual function is the getErrorStr 
  which returns the reporting information.
*/
class ExBase : public std::exception
{
 private:

  int state;           ///< Type of error
  std::string ErrLn;   ///< String causing error

 public:

  ExBase(const int,const std::string&);
  ExBase(const std::string&);
  ExBase(const ExBase&);
  ExBase& operator=(const ExBase&);
  virtual ~ExBase() throw() {}

  /// Main reporting method
  virtual const char* what() const throw()
    { return ErrLn.c_str(); }
  /// Return the error number 
  int getErrorNum() const { return state; }    
};


/*!
  \class IndexError
  \brief Exception for index errors
  \author Stuart Ansell
  \date Sept 2005
  \version 1.0

  Called when an index falls out of range

*/
class IndexError : public ExBase
{
 private:

  const int Val;     ///< Actual value called 
  const int maxVal;  ///< Maximum value

 public:

  IndexError(const int,const int,const std::string&);
  IndexError(const IndexError& A);
  IndexError& operator=(const IndexError& A);
  ~IndexError() throw() {}

  /// Overloaded reporting method
  const char* what() const throw();

};

/*!
  \class FileError
  \brief Exception for file problems
  \author Stuart Ansell
  \date Sept 2005
  \version 1.0

  Records the filename and the point of failure.

*/
class FileError : public ExBase
{
 private:

  const std::string fileName;

 public:

  FileError(const int,const std::string&,const std::string&);
  FileError(const FileError& A);
  FileError& operator=(const FileError& A);
  ~FileError() throw() {}

  /// Overloaded reporting method
  const char* what() const throw();

};

/*!
  \class InContainerError
  \brief Exception for an object not in a containedr
  \author Stuart Ansell
  \date Sept 2005
  \version 1.0

  Records the object being looked for

*/
template<typename T>
class InContainerError : public ExBase
  {
 private:

  const T SearchObj;

 public:

  InContainerError(const T&,const std::string&);


  InContainerError(const InContainerError& A);
  InContainerError& operator=(const InContainerError& A);
  ~InContainerError() throw() {}

  /// Overloaded reporting method
  const char* what() const throw();



};

/*!
  \class RangeError
  \brief Error Range in an array/list etc
  \author Stuart Ansell
  \date October 2005
  \version 1.0

  Records the object being looked for
  and the range required.
*/
template<typename T>
class RangeError : public ExBase
{
 private:

  const T Index;         ///< Current value
  const T minV;          ///< Min Value
  const T maxV;          ///< Max Value

 public:

  RangeError(const T&,const T&,const T&,const std::string&);
  RangeError(const RangeError& A);
  RangeError& operator=(const RangeError& A);
  ~RangeError() throw() {} 

  /// Overloaded reporting method
  const char* what() const throw();

};

/*!
  \class ArrayError
  \brief Error Range in an array/list etc
  \author Stuart Ansell
  \date October 2005
  \version 1.0

  Records the object being looked for
  and the range required.
*/
template<int ndim>
class ArrayError : public ExBase
{
 private:

  int arraySize[ndim];
  int indexSize[ndim];

 public:

  ArrayError(const int*,const int*,const std::string&);
  ArrayError(const ArrayError<ndim>& A);
  ArrayError<ndim>& operator=(const ArrayError<ndim>& A);
  ~ArrayError() throw() {} 

  /// Overloaded reporting method
  const char* what() const throw();

};


/*!
  \class MisMatch
  \brief Error when two numbers should be identical (or close)
  \author Stuart Ansell
  \date October 2005
  \version 1.0

  Records the object being looked for
  and the range required.
*/
template<typename T>
class MisMatch : public ExBase
{
 private:

  const T Aval;        ///< Number A 
  const T Bval;        ///< container size
 
 public:

  MisMatch(const T&,const T&,const std::string&);


  MisMatch(const MisMatch<T>& A);
  MisMatch<T>& operator=(const MisMatch<T>& A);
  ~MisMatch() throw() {}

  /// Overloaded reporting method
  const char* what() const throw();

};

/*!
  \class InvalidLine
  \brief For a parser error on a line
  \author Stuart Ansell
  \date October 2005
  \version 1.0

  Stores the position on the line that the error occured
  as well as the line
*/

class InvalidLine : public ExBase
{
 private:
  
  int pos;             ///< Position of error
  std::string Line;    ///< Error Line

 public:

  InvalidLine(const std::string&,const std::string&,const int =0);


  InvalidLine(const InvalidLine&);
  InvalidLine& operator=(const InvalidLine&);
  /// Destructor
  ~InvalidLine() throw() {}

  /// Overloaded reporting method
  const char* what() const throw();


};

/*!
  \class CastError
  \brief Dynamic Cast problems
  \author Stuart Ansell
  \date October 2006
  \version 1.0

  When a pointer cant by case
*/

template<typename Ptr>
class CastError : public ExBase
{
 private:
  
  Ptr const* Base;

 public:

  CastError(const Ptr*,const std::string&);


  CastError(const CastError<Ptr>&);
  CastError<Ptr>& operator=(const CastError<Ptr>&);
  /// Destructor
  ~CastError() throw() {}

  /// Overloaded reporting method
  const char* what() const throw();

};

/*!
  \class CommandError
  \brief Exception a command breaking in Command
  \author Stuart Ansell
  \date December 2006
  \version 1.0

  Called when a command cannot be executed

*/
class CommandError : public ExBase
{
 private:

  std::string Cmd;

 public:

  CommandError(const std::string&,const std::string&);
  CommandError(const CommandError& A);
  CommandError& operator=(const CommandError& A);
  ~CommandError() throw() {} 

  /// Overloaded reporting method
  const char* what() const throw();

};

}  // NAMESPACE 

#endif
