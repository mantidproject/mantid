#ifndef MANTID_DATAOBJECTS_REBINNEDOUTPUTTEST_H_
#define MANTID_DATAOBJECTS_REBINNEDOUTPUTTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataObjects/RebinnedOutput.h"

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::API;

class RebinnedOutputTest : public CxxTest::TestSuite
{
public:
  int nbins;
  int nhist;
  RebinnedOutput_sptr ws;

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RebinnedOutputTest *createSuite() { return new RebinnedOutputTest(); }
  static void destroySuite( RebinnedOutputTest *suite ) { delete suite; }

  RebinnedOutputTest()
  {
    nbins = 5;
    nhist = 10;
    ws = Create2DWorkspaceBinned(nhist, nbins);
  }

  static RebinnedOutput_sptr Create2DWorkspaceBinned(int nhist, int nbins,
                                                     double x0 = 0.0,
                                                     double deltax = 1.0)
  {
    MantidVecPtr x,y,e;
    x.access().resize(nbins + 1);
    y.access().resize(nbins, 2); // Value of 2.0 in all ys
    e.access().resize(nbins, sqrt(2.0));
    for (int i = 0; i < nbins + 1; ++i)
    {
      x.access()[i] = x0 + i * deltax;
    }
    RebinnedOutput_sptr retVal(new RebinnedOutput());
    retVal->initialize(nhist, nbins + 1, nbins);
    for (int i = 0; i < nhist; i++)
    {
      retVal->setX(i, x);
      retVal->setData(i, y, e);
    }

    return retVal;
  }

  void testId()
  {
    TS_ASSERT_EQUALS( ws->id(), "RebinnedOutput" );
  }

};


#endif /* MANTID_DATAOBJECTS_REBINNEDOUTPUTTEST_H_ */
