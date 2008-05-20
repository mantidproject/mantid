#ifndef WORKSPACECREATIONHELPER_H_
#define WORKSPACECREATIONHELPER_H_

#include <cmath>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"

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
  
  static Workspace1D_sptr Create1DWorkspaceRand(int size)
  {
    std::vector<double> x1(size,1),y1,e1;
    //   x1.resize(size);
    //   std::generate(x1.begin(),x1.end(),rand);	
    y1.resize(size);
    std::generate(y1.begin(),y1.end(),rand);
    e1.resize(size);
    std::generate(e1.begin(),e1.end(),rand);
    Workspace1D_sptr retVal(new Workspace1D);
    retVal->setX(x1);
    retVal->setData(y1,e1);
    return retVal;
  }

  static Workspace1D_sptr Create1DWorkspaceFib(int size)
  {
    std::vector<double> x1(size,1),y1,e1;
    //   x1.resize(size);
    //   std::generate(x1.begin(),x1.end(),rand);	
    y1.resize(size);
    std::generate(y1.begin(),y1.end(),FibSeries<double>());
    e1.resize(size);
    Workspace1D_sptr retVal(new Workspace1D);
    retVal->setX(x1);
    retVal->setData(y1,e1);
    return retVal;
  }
  static Workspace2D_sptr Create2DWorkspace(int xlen, int ylen)
  {
    return Create2DWorkspace123(xlen, ylen);
  }

  static Workspace2D_sptr Create2DWorkspace123(int xlen, int ylen,bool isHist=0)
  {
    std::vector<double> x1(isHist?xlen+1:xlen,1),y1(xlen,2),e1(xlen,3);
    Workspace2D_sptr retVal(new Workspace2D);
    retVal->init(ylen,isHist?xlen+1:xlen,xlen);
    for (int i=0; i< ylen; i++)
    {
      retVal->setX(i,x1);     
      retVal->setData(i,y1,e1);
    }

    return retVal;
  }

  static Workspace2D_sptr Create2DWorkspace154(int xlen, int ylen,bool isHist=0)
  {
    std::vector<double> x1(isHist?xlen+1:xlen,1),y1(xlen,5),e1(xlen,4);
    Workspace2D_sptr retVal(new Workspace2D);
    retVal->init(ylen,isHist?xlen+1:xlen,xlen);
    for (int i=0; i< ylen; i++)
    {
      retVal->setX(i,x1);     
      retVal->setData(i,y1,e1);
    }

    return retVal;
  }  

  static WorkspaceSingleValue_sptr CreateWorkspaceSingleValue(double value)
  {
    WorkspaceSingleValue_sptr retVal(new WorkspaceSingleValue(value,sqrt(value)));
    return retVal;
  }

};

#endif /*WORKSPACECREATIONHELPER_H_*/
