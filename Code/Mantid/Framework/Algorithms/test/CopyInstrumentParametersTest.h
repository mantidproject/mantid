#ifndef COPYINSTRUMENTPARAMETERSTEST_H_
#define COPYINSTRUMENTPARAMETERSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadInstrument.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAlgorithms/CopyInstrumentParameters.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/MatrixWorkspace.h"
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

     int ndets = 3; 

     // Create workspace with paremeterised instrument and put into data store
     Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(ndets, 10, true);
     const std::string wsName("CopyInstParamWs1");
     AnalysisDataServiceImpl & dataStore = AnalysisDataService::Instance();
     dataStore.add(wsName, ws);

  }

  void testDifferent_BaseInstrument_Throws()
  {
  }

private:
  CopyInstrumentParameters copyInstParam;


};

#endif /*COPYINSTRUMENTPARAMETERSTEST_H_*/
