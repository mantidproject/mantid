#ifndef TOFSANSRESOLUTIONBYPIXELTEST_H_
#define TOFSANSRESOLUTIONBYPIXELTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/TOFSANSResolutionByPixel.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Instrument.h"

#include <stdexcept>

using namespace Mantid::Algorithms;
using namespace Mantid::API;


class TOFSANSResolutionByPixelTest : public CxxTest::TestSuite
{
public:

  void testName()
  {
    TS_ASSERT_EQUALS( alg.name(), "TOFSANSResolutionByPixel" )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( alg.category(), "SANS" )
  }

  void testInit()
  {
    alg.initialize();
    TS_ASSERT( alg.isInitialized() )
  }




private:
  TOFSANSResolutionByPixel alg;

};





#endif /*TOFSANSRESOLUTIONBYPIXELTEST_H_*/
