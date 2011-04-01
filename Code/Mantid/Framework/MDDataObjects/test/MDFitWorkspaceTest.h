#ifndef H_TEST_MDFITWORKSPACE
#define H_TEST_MDFITWORKSPACE

#include <cxxtest/TestSuite.h>
#include "MantidKernel/System.h"
#include "MDDataObjects/MDDataPoints.h"
#include "MDDataObjects/MDFitWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Algorithm.h"

#include <cstdlib>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::MDDataObjects;

class MDFitWorkspaceTest :    public CxxTest::TestSuite
{

public:

  //Test for the IMDWorkspace aspects of MDWorkspace.
  void testGetNDimensions()
  {
    MDFitWorkspace ws(2,2);
    TS_ASSERT_EQUALS(ws.getNumDims(),2);
    ws.setDimension(0,"id=x,xmin=0,n=10,dx=2");
    ws.setDimension(1,"id=y,xmin=-1,xmax=1,n=10");
    IMDIterator* it = ws.createIterator();
    do
    {
      std::vector<boost::shared_ptr<Mantid::Geometry::MDPoint> > points(2);
      points[0].reset(new MDPoint(double(rand() % 100),1.0,std::vector<coordinate>(),IDetector_sptr(),IInstrument_sptr()));
      points[1].reset(new MDPoint(double(rand() % 100),1.0,std::vector<coordinate>(),IDetector_sptr(),IInstrument_sptr()));
      int i = it->getPointer();
      ws.setCell(i,points);
    }
    while(it->next());
    delete it;

    it = ws.createIterator();
    do
    {
      int i = it->getPointer();
      const SignalAggregate& cell = ws.getCell(i);
      //std::cerr << "cell " << i << " " << cell.getSignal() << ' ' << cell.getError() << ' ' << cell.getContributingPoints().size() << std::endl;
    }
    while(it->next());
    delete it;

  }



};

#endif // H_TEST_MDFITWORKSPACE
