#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include "MantidGeometry/AuxException.h"

namespace ColErr
{

ExBase::ExBase(const int A,const std::string& Err) : 
    std::exception(),
    state(A),ErrLn(Err)
  /*!
    Constructor
    \param A :: State variable
    \param Err :: Class:method string
  */
{ }

ExBase::ExBase(const std::string& Err) :
  std::exception(),state(0),ErrLn(Err)
  /*!
    Constructor
    \param Err :: Class:method string
  */
{ }

ExBase::ExBase(const ExBase& A) :
  std::exception(A),state(A.state),ErrLn(A.ErrLn)
  /*!
    Copy Constructor
    \param A :: Error to copy
  */
{ }

ExBase&
ExBase::operator=(const ExBase& A)
  /*!
    Assignment operator
    \param A :: Error to copy
  */
{
  if(this!=&A)
    {
      std::exception::operator=(A);
      state=A.state;
      ErrLn=A.ErrLn;
    }
  return *this;
}

// Index Error class
  
/*!
  Constructor
  \param V :: Value of index
  \param B :: Maximum value
  \param Place :: Location of Error
*/
IndexError::IndexError(const int V,const int B, const std::string& Place) :
  ExBase(0,Place),Val(V),maxVal(B) 
{}

/*!
  Copy Constructor
  \param A IndexError to copy
*/
IndexError::IndexError(const IndexError& A) :
  ExBase(A),Val(A.Val),maxVal(A.maxVal)
{}

const char*
IndexError::what() const throw()
  /*!
    Writes out the range and limits
  */
{
  std::stringstream cx;
  cx<<"IndexError:"<<ExBase::what()<<" "<<Val<<" :: 0 <==> "<<maxVal;
  return cx.str().c_str();
}

//-------------------------
// InContainerError
//-------------------------

template<typename T>
InContainerError<T>::InContainerError(const T& V,const std::string& Place) :
  ExBase(0,Place),SearchObj(V)
  /*!
    Constructor
    \param V :: Value not found
    \param Place :: Function description
  */
{}

template<typename T>
InContainerError<T>::InContainerError(const InContainerError& A) :
  ExBase(A),SearchObj(A.SearchObj)
  /*!
    Copy Constructor
    \param A InContainerError to copy
  */
{}

template<typename T>
const char*
InContainerError<T>::what() const throw()
  /*!
    Writes out the range and limits
    \returns String description of error
  */
{
  std::stringstream cx;
  cx<<ExBase::what()<<" key== "<<SearchObj;
  return cx.str().c_str();
}

//-------------------------
// MisMatch
//-------------------------

template<typename T> 
MisMatch<T>::MisMatch(const T& A,const T& B,const std::string& Place) :
  ExBase(0,Place),Aval(A),Bval(B) 
  /*!
    Constructor store two mismatched items
    \param A :: Item to store
    \param B :: Item to store
    \param Place :: Reason/Code item for error
  */
{}

template<typename T> 
MisMatch<T>::MisMatch(const MisMatch<T>& A) :
  ExBase(A),Aval(A.Aval),Bval(A.Bval)
   /*!
    Copy Constructor
    \param A MisMatch to copy
  */
{}

template<typename T> 
const char*
MisMatch<T>::what() const throw()
  /*!
    Writes out the two mismatched items
    \returns String description of error
  */
{
  std::stringstream cx;
  cx<<ExBase::what()<<" Item A!=B "<<Aval<<
    " "<<Bval<<" ";
  return cx.str().c_str();
}

//-------------------------
// InvalidLine
//-------------------------

InvalidLine::InvalidLine(const std::string& Place,
			    const std::string& L,const int P) :
  ExBase(0,Place),pos(P),Line(L)
  /*!
    Constructor of an invalid line
    \param L :: Line causing the error
    \param P :: Positions of error
    \param Place :: Reason/Code item for error
  */
{}

InvalidLine::InvalidLine(const InvalidLine& A) :
  ExBase(A),pos(A.pos),Line(A.Line)
  /*!
    Copy constructor 
    \param A :: InvalidLine to copy
  */
{}

InvalidLine&
InvalidLine::operator=(const InvalidLine& A) 
  /*!
    Assignment operator
    \param A :: InvalidLine to copy
    \return *this
  */
{
  if (this!=&A)
    {
      ExBase::operator=(A);
      pos=A.pos;
      Line=A.Line;
    }
  return *this;
}

const char*
InvalidLine::what() const throw()
  /*!
    Writes out the line and positions of the error
    \returns String description of error
  */
{
  std::stringstream cx;
  cx<<ExBase::what()<<" Line: "<<Line<<" @ "<<pos;
  return cx.str().c_str();
}


}   // NAMESPACE ColErr

/// \cond TEMPLATE

template class ColErr::InContainerError<std::string>;
template class ColErr::InContainerError<int>;
template class ColErr::MisMatch<int>;

/// \endcond TEMPLATE
