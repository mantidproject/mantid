#ifndef CalculateDetOffsetsMultiPeaksTEST_H_
#define CalculateDetOffsetsMultiPeaksTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/DetectorInfo.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/TableRow.h"
#include "MantidAlgorithms/CalculateDetOffsetsMultiPeaks.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::Algorithms::CalculateDetOffsetsMultiPeaks;
using Mantid::DataObjects::OffsetsWorkspace_sptr;
using namespace Mantid::DataHandling;

class CalculateDetOffsetsMultiPeaksTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculateDetOffsetsMultiPeaksTest *createSuite() {
    return new CalculateDetOffsetsMultiPeaksTest();
  }
  static void destroySuite(CalculateDetOffsetsMultiPeaksTest *suite) {
    delete suite;
  }

  CalculateDetOffsetsMultiPeaksTest() {
    Mantid::API::FrameworkManager::Instance();
  }

  void testTheBasics() {
    TS_ASSERT_EQUALS(offsets.name(), "CalculateDetOffsetsMultiPeaks");
    TS_ASSERT_EQUALS(offsets.version(), 1);
  }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(offsets.initialize());
    TS_ASSERT(offsets.isInitialized());

    // load data
    myPeakPositionWorkspaceName = loadPeakPositions();

    myReferenceWorkspaceName = createReferenceWorkspace(24900);
  }

  void testFit5Peaks() {
    offsets.initialize();

    offsets.setProperty("InputWorkspace", myPeakPositionWorkspaceName);
    offsets.setProperty("DReference",
                        "1.0758, 0.89198, 0.8186, 0.728299, 0.6867");
    offsets.setProperty("OutputWorkspace", "VulcanOffsets");
    offsets.setProperty("MaskWorkspace", "VulcanMasks");
    offsets.setProperty("ReferenceWorkspace", myReferenceWorkspaceName);

    offsets.execute();
    TS_ASSERT(offsets.isExecuted());
  }

private:
  CalculateDetOffsetsMultiPeaks offsets;
  std::string myPeakPositionWorkspaceName;
  std::string myReferenceWorkspaceName;

  //----------------------------------------------------------------------------------------------
  /** Generate a workspace contains PG3_4866 5-th peak
    */
  std::string loadPeakPositions() {

    DataHandling::LoadNexusProcessed loader;
    loader.initialize();

    loader.setProperty("Filename",
                       "/home/wzz/Mantid/high_peak_pos_partial.nxs");
    loader.setProperty("OutputWorkspace", "DiamondPeakPositions");

    loader.execute();

    TS_ASSERT(
        AnalysisDataService::Instance().doesExist("DiamondPeakPositions"));

    API::MatrixWorkspace_sptr ws =
        boost::dynamic_pointer_cast<API::MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("DiamondPeakPositions"));
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 24900);

    return "DiamondPeakPositions";
  }

  std::string createReferenceWorkspace(int numhist) {
    EventWorkspace_sptr refws =
        WorkspaceCreationHelper::createEventWorkspace2(numhist, 1);
    AnalysisDataService::Instance().addOrReplace("ReferenceWorkspace", refws);
    myReferenceWorkspaceName = "ReferenceWorkspace";

    LoadInstrument loader;
    loader.initialize();
    loader.setProperty("Workspace", myReferenceWorkspaceName);
    loader.setProperty("Filename",
                       "/home/wzz/Mantid/VULCAN_Definition_2017-05-20.xml");
    Kernel::OptionalBool rewrite(true);
    loader.setProperty("RewriteSpectraMap", rewrite);

    loader.execute();

    return myReferenceWorkspaceName;
  }
};

#endif /*CalculateDetOffsetsMultiPeaksTEST_H_*/
