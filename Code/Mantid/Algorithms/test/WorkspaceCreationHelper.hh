#ifndef WORKSPACECREATIONHELPER_H_
#define WORKSPACECREATIONHELPER_H_

#include <cmath>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/Workspace1D.h"

using namespace Mantid::DataObjects;

class WorkspaceCreationHelper
{
public:

  template<typename T>
  class FibSeries
  {
   private:
    T x1;  /// Initial value 1;
    T x2;  /// Initial value 2;
    
    public:
    
    FibSeries() : x1(1),x2(1) {}
    T operator()() { const T out(x1+x2); x1=x2; x2=out;  return out; }
  };

  static Workspace1D* Create1DWorkspaceFib(int size)
  {
    std::vector<double> x1,y1,e1;
    x1.resize(size);
    std::generate(x1.begin(),x1.end(),rand);	
    y1.resize(size);
    std::generate(y1.begin(),y1.end(),FibSeries<double>());
    e1.resize(size);
    Workspace1D* retVal = new Workspace1D;
    retVal->setX(x1);
    retVal->setData(y1,e1);
    return retVal;
  }

  static Workspace2D* Create2DWorkspace(int xlen, int ylen)
  {
    std::vector<double> x1(xlen,1),y1(xlen,2),e1(xlen,3);
    Workspace2D* retVal = new Workspace2D;
    retVal->setHistogramNumber(ylen);
    for (int i=0; i< ylen; i++)
    {
      retVal->setX(i,x1);     
      retVal->setData(i,y1,e1);
    }

    return retVal;
  }
  
  
};

#endif /*WORKSPACECREATIONHELPER_H_*/
