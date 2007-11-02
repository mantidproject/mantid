#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cmath>
#include <vector>
#include <list>
#include <map>
#include <stack>
#include <functional>
#include <algorithm>
#include <iterator>

#include "Logger.h"
#include "Exception.h"
#include "XMLattribute.h"
#include "XMLobject.h"
#include "XMLgroup.h"
#include "XMLread.h"
#include "XMLcollect.h"
#include "IndexIterator.h"
#include "FileReport.h"
#include "GTKreport.h"
#include "OutputLog.h"
#include "Flux.h"

namespace Mantid
{

namespace Geometry
{

Logger& Flux::PLog = Logger::get("Flux");
Flux::Flux() : nCnt(0) 
  /*!
    Constructor
  */
{}

Flux::Flux(const Flux& A) : 
  nCnt(A.nCnt),I(A.I)
  /*!
    Copy Constructor
    \param A :: Flux object to copy
  */
{}

Flux&
Flux::operator=(const Flux& A)
  /*!
    Assignment operator 
    \param A :: Flux object to copy
    \return *this
  */
{
  if (this!=&A)
    {
      nCnt=A.nCnt;
      I=A.I;
    }
  return *this;
}

Flux::~Flux()
  /*!
    Destructor
  */
{}

void 
Flux::zeroSize(const int N)
  /*!
    Zero the array and set the number of bins
    \param N :: number of bins in I.
   */
{
  nCnt=0;
  I.resize(N);
  fill(I.begin(),I.end(),0.0);
  return;
}

void
Flux::addEvent(const int index,const double D)
  /*!
    Adds a value to the flux.
    \param index :: array offset to use
    \param D :: Value to add
   */
{
  if (index<0 || index>=static_cast<int>(I.size()))
    throw ColErr::IndexError(index,I.size(),"Flux::addEvent");
  I[index]+=D;
  return;
}


}   // NAMESPACE Monte Carlo

}  // NAMESPACE Mantid
