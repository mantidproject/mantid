#ifndef mathSupport_h
#define mathSupport_h

#include <functional>

namespace Mantid
{

#define fabs(x) std::fabs((x)*1.0)

int factorial(const int);       ///< Get a factorial number 
float ran();                    ///< Basic random number function
float ran1(int&,const int=0);   ///< Support code for random number function 
double randomNormal();          ///< Generates a normally distributed random number
//double normalDista(const double); ///< convert a number 0->1 into a normal distribute
double invErf(const double x);    ///< Inverse error function


/// Range and initialise polynominal calculation
template<typename TX,typename TY>
TY
polInterp(const TX&,const int,const std::vector<TX>&,
	  const std::vector<TY>&);


/// Calcluate a polynominal fit to a segment of data
template<typename TX,typename TY>
TY polFit(const TX&,const int,
		  typename std::vector<TX,std::allocator< TX > >::const_iterator,
		  typename std::vector<TY,std::allocator< TY > >::const_iterator);

/// Determine closes point to aim in a vector
template<typename T> 
typename std::vector<T,std::allocator<T> >::const_iterator
iteratorPos(const std::vector<T,std::allocator<T> >&,const T&);

/// Integrate on irregular grid using quadratic
template<typename T>
T intQuadratic(const typename std::vector<T,std::allocator<T> >::const_iterator&,
			   const typename std::vector<T,std::allocator<T> >::const_iterator&);

/// Integrate on irregular grid using quadratic
template<typename T>
T intQuadratic(const typename std::vector<T,std::allocator<T> >::const_iterator&,
			   const typename std::vector<T,std::allocator<T> >::const_iterator&);

template<typename T>
T derivQuadratic(const typename std::vector<T,std::allocator<T> >::const_iterator&,
				 const typename std::vector<T,std::allocator<T> >::const_iterator&);

template<typename T>
double norm(const std::vector<T>&);       ///< Calculate the norm of a vector (\f$ ||V|| \f$)
 
/// Create an index of the vector in sorted order.
template<typename T> 
void indexSort(const std::vector<T>&,std::vector<int>&);

/// Calculate the value of a quadratic at x
double quad(const double,const double,const double,const double);

/// Solve a Quadratic equation
template<typename InputIter>
int solveQuadratic(const InputIter,
       std::pair<std::complex<double>,std::complex<double> >&);

/// Solve a Cubic equation
template<typename InputIter>
int solveCubic(const InputIter,std::complex<double>&,
	       std::complex<double>&,std::complex<double>&);

/*!
  \namespace mathFunc
  \brief Holds simple functions for numerical stuff 
  \version 1.0
  \author S. Ansell
  \date January 2006
  
  Holds new math functions, we will put the above stuff in
  it later (hopefully)
*/
namespace mathFunc
{
  template<typename T> void Order(T&,T&);  ///< Simple ordering of two components
  template<typename T> void Swap(T&,T&);   ///< Simple exchange of two components
  template<typename T,typename U> void crossSort(std::vector<T>&,std::vector<U>&);
  template<typename T> 
  int binSearch(const typename std::vector<T>::const_iterator&,
		const typename std::vector<T>::const_iterator&,
		const T&);
};


/*!
  \namespace mathSupport
  \brief Holds functors to do numerical operations
  \version 1.0
  \author S. Ansell
  \date August 2005
  
  Holds all the numberical operations for comparison
  and checking needed for containers of objects.
*/

namespace mathSupport
{

  /*!
    \struct Rsol
    \brief A simple imagenary class (replace by complex)
    \version 1.0
    \author S. Ansell
  */
struct Rsol
{
  double re;    ///< real value
  double im;    ///< imag value
};

/*!
  \class PIndex 
  \author S. Ansell
  \date Aug 2005
  \version 1.0
  \brief Class  to fill an index with a progressive count
*/

template<typename T>
class PIndex
{
private:

  int count;    ///< counter

public:

  /// Constructor
  PIndex() : count(0) { }

  /// functional
  std::pair<T,int> operator()(const T& A) {  return std::pair<T,int>(A,count++); }

};

/*!
  \class PCombine
  \brief Combines two values into a  pair
  \author S. Ansell
  \date August 2006
  \version 1.0
*/

template<typename T,typename U>
class PCombine
{
public:

  /// Combination operator
  std::pair<T,U> operator()(const T& A,const U& B) 
    {  return std::pair<T,U>(A,B); }

};

/*!
  \class PSep 
  \author S. Ansell
  \date Aug 2005
  \version 1.0
  \brief Class to access the second object in index pair. 
*/
template<typename T>
class PSep
{
public:

  /// Functional to the second object
  int operator()(const std::pair<T,int>& A) {  return A.second; }

};

/*!
  \class PairFstEq
  \brief Functonal to provide equal for the first object in a pair.
  \author S. Ansell
  \date February 2006
  \version 1.0
*/

template<typename T,typename U>
 class PairFstEq : public  std::binary_function<std::pair<T,U>,T, bool >     //Note this needs to be non-constant
  {
    public:

    /// Check first items
      bool operator()(const std::pair<T,U>& A,const T& B) const
	{ 
	  return (A.first==B);
	}
  };

/*!
  \class PairSndEq
  \brief Functonal to provide equal for the second object in a pair.
  \author S. Ansell
  \date January 2006
  \version 1.0
*/

template<typename T,typename U>
 class PairSndEq : public  std::binary_function<std::pair<T,U>,U, bool >     //Note this needs to be non-constant
  {
    public:

      /// Check only second items
      bool operator()(const std::pair<T,U>& A,const U& B) const
	{ 
	  return (A.second==B);
	}
  };

/*!
  \class PairFstLess
  \brief Functonal to provide lessthan for the first object in a pair.
  \author S. Ansell
  \date June 2006
  \version 1.0
*/

template<typename T,typename U>
 class PairFstLess : 
 public  std::binary_function<std::pair<T,U>,std::pair<T,U>,bool >     //Note this needs to be non-constant
  {
    public:
    
    /// Direct comparison operator
    bool operator()(const std::pair<T,U>& A, const std::pair<T,U>& B) const    
      { 
	return (A.first<B.first);
      }
    
    /// Subsequent comparison operator
    bool operator()(const std::pair<T,U>& A, const T& B) const
	{ 
	  return (A.fist<B);
	}
    };

/*!
  \class PairSndLess
  \brief Functonal to provide lessthan for the second object in a pair.
  \author S. Ansell
  \date January 2006
  \version 1.0
*/

template<typename T,typename U>
 class PairSndLess : 
 public  std::binary_function<std::pair<T,U>,std::pair<T,U>,bool >     //Note this needs to be non-constant
  {
    public:
    
    /// Direct comparison operator
    bool operator()(const std::pair<T,U>& A, const std::pair<T,U>& B) const    
      { 
	return (A.second<B.second);
      }
    
    /// Subsequent comparison operator
    bool operator()(const std::pair<T,U>& A, const U& B) const
	{ 
	  return (A.second<B);
	}
    };


/*!
  \class absComp
  \brief Class to compare absolute numbers 
  \author S. Ansell
  \date September 2005
  \version 1.0
*/

template<typename T>
class absComp
{
public:

  /// Does the absolute comparison
  int operator()(const T& A,const T& B) 
    {
      return (fabs(A)<fabs(B));
    }

};


}   // NAMESPACE  mathSupport

}   // NAMESPACE Mantid

#endif

