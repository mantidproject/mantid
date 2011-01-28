#ifndef mathSupport_h
#define mathSupport_h

#include <functional>
#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"

namespace Mantid
{

#define fabs(x) std::fabs((x)*1.0)

/// Create an index of the vector in sorted order.
template<typename T>
DLLExport void indexSort(const std::vector<T>&,std::vector<int>&);

/// Solve a Quadratic equation
template<typename InputIter>
DLLExport int solveQuadratic(InputIter,
	     std::pair<std::complex<double>,std::complex<double> >&);

/// Solve a Cubic equation
template<typename InputIter>
DLLExport int solveCubic(InputIter,std::complex<double>&,
	       std::complex<double>&,std::complex<double>&);

/**
  \brief Holds functors to do numerical operations
  \version 1.0
  \author S. Ansell
  \date August 2005

  Holds all the numerical operations for comparison
  and checking needed for containers of objects.
*/
namespace mathSupport
{

/**
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

/**
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

}   // NAMESPACE  mathSupport

}   // NAMESPACE Mantid

#endif

