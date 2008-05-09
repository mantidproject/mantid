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

    ExBase(int const A,const std::string& Err);
    ExBase(const std::string& Err);
    ExBase(const ExBase& A);
    ExBase& operator=(const ExBase& A);
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

    IndexError(const int V,const int B, const std::string& Place);
    IndexError(const IndexError& A);
    ~IndexError() throw() {}

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

    ///The object that was being searchec for
    const T SearchObj;

  public:

    InContainerError(const T&,const std::string&);


    InContainerError(const InContainerError& A);
    ~InContainerError() throw() {}

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

    InvalidLine(const std::string&,const std::string&, int const=0);


    InvalidLine(const InvalidLine&);
    InvalidLine& operator=(const InvalidLine&);
    /// Destructor
    ~InvalidLine() throw() {}

    /// Overloaded reporting method
    const char* what() const throw();


  };

}  // NAMESPACE 

#endif
