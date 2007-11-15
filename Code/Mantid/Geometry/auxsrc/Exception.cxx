#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include "Exception.h"

namespace Mantid
{

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
  */
{ }

ExBase&
ExBase::operator=(const ExBase& A)
  /*!
    Assignment operator
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

/// Index Error class

IndexError::IndexError(const int V,const int B,
		       const std::string& Place) :
  ExBase(0,Place),Val(V),maxVal(B)
{}

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
// FileError
//-------------------------

FileError::FileError(const int V,const std::string& FName,const std::string& Place) :
  ExBase(V,Place),fileName(FName)
  /*!
    Constructor
    \param V :: Error number
    \param Fname :: Filename 
    \param Place :: Function description
  */
{}

FileError::FileError(const FileError& A) :
  ExBase(A),fileName(A.fileName)
  /// Copy constructor
{}

const char*
FileError::what() const throw()
  /*!
    Writes out the range and limits
  */
{
  std::stringstream cx;
  cx<<ExBase::what()<<" in "<<fileName;
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
  /// Copy constructor
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
// RangeError
//-------------------------
template<typename T>
RangeError<T>::RangeError(const T& V,const T& aV,const T& bV,
		       const std::string& Place) :
  ExBase(0,Place),Index(V),minV(aV),maxV(bV)
  /*!
    Set a RangeError
    \param I :: Value that caused the problem
    \param size :: size of the indexed object
    \param Place :: String describing the place
  */
{}

template<typename T>
RangeError<T>::RangeError(const RangeError& A) :
  ExBase(A),Index(A.Index),minV(A.minV),maxV(A.maxV)
  /// Copy constructor
{}

template<typename T>
const char*
RangeError<T>::what() const throw()
  /*!
    Writes out the range and aim point
    \returns String description of error
  */
{
  std::stringstream cx;
  cx<<ExBase::what()<<" Value == "<<Index<<
    " Min == "<<minV<<
    " Max == "<<maxV;
  return cx.str().c_str();
}

//-------------------------
// ArrayError
//-------------------------

template<int ndim>
ArrayError<ndim>::ArrayError(const int* A,const int* I,const std::string& Place) :
  ExBase(0,Place)
  /*!
    Set a ArrayError
    \param A :: Array size
    \param I :: Index given
    \param Place :: String describing the place
  */
{
  for(int i=0;i<ndim;i++)
    {
      arraySize[i]=A[i];
      indexSize[i]=I[i];
    }  
}

template<int ndim>
ArrayError<ndim>::ArrayError(const ArrayError<ndim>& A) :
  ExBase(A)
  /*!
    Copy constructor 
    \param A :: Object to copy
  */
{
  for(int i=0;i<ndim;i++)
    {
      arraySize[i]=A.arraySize[i];
      indexSize[i]=A.indexSize[i];
    }

}

template<int ndim>
const char*
ArrayError<ndim>::what() const throw()
  /*!
    Writes out the range and aim point
    \returns String description of error
  */
{
  std::stringstream cx;
  cx<<ExBase::what()<<":";

  for(int i=0;i<ndim;i++)
    {
      cx<<indexSize[i]<<" ("<<arraySize[i]<<") ";
    }
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


//--------------------
// CastError
//--------------------

template<typename Ptr>
CastError<Ptr>::CastError(const Ptr* B,
			  const std::string& Place)  :
  ExBase(0,Place),Base(B)
  /*!
    Constructor of an invalid line
    \param L :: Line causing the error
    \param P :: Positions of error
    \param Place :: Reason/Code item for error
  */
{}

template<typename Ptr>
CastError<Ptr>::CastError(const CastError<Ptr>& A) :
  ExBase(A),Base(A.Base)
  /*!
    Copy constructor 
    \param A :: CastError to copy
  */
{}

template<typename Ptr>
CastError<Ptr>&
CastError<Ptr>::operator=(const CastError<Ptr>& A) 
  /*!
    Assignment operator
    \param A :: CastError to copy
    \return *this
  */
{
  if (this!=&A)
    {
      ExBase::operator=(A);
      Base=A.Base;
    }
  return *this;
}

template<typename Ptr>
const char*
CastError<Ptr>::what() const throw()
  /*!
    Writes out the line and positions of the error
    \returns String description of error
  */
{
  std::stringstream cx;
  cx<<ExBase::what()<<" Cast Obj: "<<reinterpret_cast<long int>(Base)<<std::endl;
  return cx.str().c_str();
}

}   // NAMESPACE ColErr

/// \cond TEMPLATE

template class ColErr::InContainerError<std::string>;
template class ColErr::InContainerError<int>;
template class ColErr::MisMatch<int>;
template class ColErr::ArrayError<2>;
namespace TimeData{ class WorkSpace; };
template class ColErr::CastError<TimeData::WorkSpace>;
template class ColErr::RangeError<double>;
template class ColErr::RangeError<int>;

/// \endcond TEMPLATE

} // NAMESPACE Mantid
