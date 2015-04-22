#ifndef MANTID_ALGORITHMS_CONVERTUNITSUSINGDETECTORTABLETEST_H_
#define MANTID_ALGORITHMS_CONVERTUNITSUSINGDETECTORTABLETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ConvertUnitsUsingDetectorTable.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataHandling/SaveNexusProcessed.h"




using Mantid::Algorithms::ConvertUnitsUsingDetectorTable;
using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

class ConvertUnitsUsingDetectorTableTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertUnitsUsingDetectorTableTest *createSuite() { return new ConvertUnitsUsingDetectorTableTest(); }
  static void destroySuite( ConvertUnitsUsingDetectorTableTest *suite ) { delete suite; }


  void test_Init()
  {
    ConvertUnitsUsingDetectorTable alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );
  }


  // TODO: Make this test useful
  void test_TofToLambda()
  {
     ConvertUnitsUsingDetectorTable myAlg;
     myAlg.initialize();
     TS_ASSERT(myAlg.isInitialized());

     const std::string workspaceName("_ws_testConvertUsingDetectorTable");
     int nBins = 10;
     MatrixWorkspace_sptr WS = WorkspaceCreationHelper::Create2DWorkspaceBinned(2, nBins, 500.0, 50.0);
     WS->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

     AnalysisDataService::Instance().add(workspaceName,WS);

      // Create TableWorkspace with values in it

      ITableWorkspace_sptr pars = WorkspaceFactory::Instance().createTable("TableWorkspace");
      pars->addColumn("int", "spectra");
      pars->addColumn("double", "l1");
      pars->addColumn("double", "l2");
      pars->addColumn("double", "twotheta");
      pars->addColumn("double", "efixed");
      pars->addColumn("int", "emode");

      API::TableRow row0 = pars->appendRow();
      row0 << 1 << 100.0 << 10.0 << 90.0 << 7.0 << 0;

      API::TableRow row1 = pars->appendRow();
      row1 << 2 << 1.0 << 1.0 << 90.0 << 7.0 << 0;

//      Mantid::DataHandling::SaveNexusProcessed saver;
//      saver.initialize();
//      saver.setProperty("InputWorkspace",pars);
//      saver.setPropertyValue("Filename", "pars.nxs");
//      saver.execute();

      // Set the properties
      myAlg.setRethrows(true);
      myAlg.setPropertyValue("InputWorkspace", workspaceName);
      myAlg.setPropertyValue("OutputWorkspace", workspaceName);
      myAlg.setPropertyValue("Target", "Wavelength");
      myAlg.setProperty("DetectorParameters", pars);

      myAlg.execute();

      auto outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaceName);

//      for (int j=0; j < outWS->getNumberHistograms(); ++j) {
//          for (int i=0; i < outWS->blocksize(); ++i) {
//              std::cout << "dataX[" << j << "]["<< i << "] = " << outWS->dataX(j)[i] << std::endl;
//          }
//      }

      TS_ASSERT_DELTA( outWS->dataX(0)[0], 0.017982, 0.000001 );
      TS_ASSERT_DELTA( outWS->dataX(0)[9], 0.034166, 0.000001 );
//      TS_ASSERT_DELTA( outWS->dataX(1)[0], 0.179818, 0.000001 );
//      TS_ASSERT_DELTA( outWS->dataX(1)[9], 0.017982, 0.000001 );

      AnalysisDataService::Instance().remove(workspaceName);
  }


};


#endif /* MANTID_ALGORITHMS_CONVERTUNITSUSINGDETECTORTABLETEST_H_ */
