#ifndef MANTID_MDEVENTS_MDHISTOWORKSPACEITERATORTEST_H_
#define MANTID_MDEVENTS_MDHISTOWORKSPACEITERATORTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDEvents/MDHistoWorkspaceIterator.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;

class MDHistoWorkspaceIteratorTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDHistoWorkspaceIteratorTest *createSuite() { return new MDHistoWorkspaceIteratorTest(); }
  static void destroySuite( MDHistoWorkspaceIteratorTest *suite ) { delete suite; }


  void test_Something()
  {
  }


};


#endif /* MANTID_MDEVENTS_MDHISTOWORKSPACEITERATORTEST_H_ */

