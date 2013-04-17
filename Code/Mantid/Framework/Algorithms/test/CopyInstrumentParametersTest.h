#ifndef COPYINSTRUMENTPARAMETERSTEST_H_
#define COPYINSTRUMENTPARAMETERSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadInstrument.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAlgorithms/CopyInstrumentParameters.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "WorkspaceCreationHelperTest.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/V3D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include <stdexcept>

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using Mantid::Geometry::IDetector_const_sptr;

class CopyInstrumentParametersTest : public CxxTest::TestSuite
{
public:

  void testName()
  {
    TS_ASSERT_EQUALS( copyInstParam.name(), "CopyInstrumentParameters" )
  }

  void testInit()
  {
    copyInstParam.initialize();
    TS_ASSERT( copyInstParam.isInitialized() )
  }

  void testExec()
  {
     // Create workspace with paremeterised instrument and put into data store
     MatrixWorkspace_sptr ws1 = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 10, true);
     const std::string wsName1("CopyInstParamWs1");
     AnalysisDataServiceImpl & dataStore = AnalysisDataService::Instance();
     dataStore.add(wsName1, ws1);
     /// Create another workspace with the same instrument and put into data store
     MatrixWorkspace_sptr ws2 = WorkspaceFactory::Instance().create( ws1 );
     const std::string wsName2("CopyInstParamWs2");
     dataStore.add(wsName2, ws2);

     // Set properties
     TS_ASSERT_THROWS_NOTHING(copyInstParam.setPropertyValue("InputWorkspace", wsName1 ));
     TS_ASSERT_THROWS_NOTHING(copyInstParam.setPropertyValue("OutputWorkspace", wsName2 ));
  }

  void testDifferent_BaseInstrument_Throws()
  {
  }

private:
  CopyInstrumentParameters copyInstParam;


};

#endif /*COPYINSTRUMENTPARAMETERSTEST_H_*/
