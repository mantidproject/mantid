#ifndef MANTID_ALGORITHMS_CONVERTTOMASKINGWORKSPACETEST_H_
#define MANTID_ALGORITHMS_CONVERTTOMASKINGWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidAlgorithms/ConvertToMaskingWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidDataObjects/MaskWorkspace.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class ConvertToMaskingWorkspaceTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertToMaskingWorkspaceTest *createSuite() { return new ConvertToMaskingWorkspaceTest(); }
  static void destroySuite( ConvertToMaskingWorkspaceTest *suite ) { delete suite; }


  void test_Init()
  {
    ConvertToMaskingWorkspace convert;
    TS_ASSERT_THROWS_NOTHING(convert.initialize());
    TS_ASSERT(convert.isInitialized());
  }

  void test_Convert()
  {
    // 1. Construct input workspace
    // a) Workspace
    const int numhist = 20;
    const int numbins = 1;
    double x0 = 100.0;
    double deltax = 20.0;
    DataObjects::Workspace2D_sptr inputWS =
        WorkspaceCreationHelper::Create2DWorkspaceBinned(numhist, numbins, x0, deltax);

    // b) Set value
    for (size_t i = 0; i < static_cast<size_t>(numhist); i ++){
      inputWS->dataY(i)[0] = static_cast<double>(i%2);
    }

    // c) Instrument
    Geometry::Instrument_sptr instrument(new Geometry::Instrument);
    specid_t specids[20];
    detid_t detids[20];
    for (size_t i = 0; i < static_cast<size_t>(numhist); i ++){
      specids[i] = static_cast<specid_t>(i)+1;
      detids[i] = static_cast<detid_t>(i+1000);

      std::stringstream ss;
      ss << "fakedetector" << detids[i];
      std::string detname = ss.str();
      Geometry::Detector* det = new Geometry::Detector(detname, detids[i], NULL);

      // int numcomp = instrument->add(det);
      // std::cout << "Number of components = " << numcomp << std::endl;
      //
      instrument->markAsDetector(det);
      // std::cout << "Instrument has detectors = " << instrument->getDetectorIDs().size() << std::endl;

    }

    inputWS->setInstrument(instrument);

    // d) Spectrum map
    inputWS->replaceSpectraMap(new SpectraDetectorMap(specids, detids, numhist));

    AnalysisDataService::Instance().add("testin", inputWS);

    // 2. Set up and execute
    ConvertToMaskingWorkspace alg1;
    TS_ASSERT_THROWS_NOTHING(alg1.initialize());
    TS_ASSERT_THROWS_NOTHING(alg1.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg1.setPropertyValue("OutputWorkspace", "testout"));
    TS_ASSERT_THROWS_NOTHING(alg1.execute());
    TS_ASSERT(alg1.isExecuted());

    // 3. Check result
    API::MatrixWorkspace_sptr outputWS = AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>("testout");
    bool wsexist = true;
    if (!outputWS)
      wsexist = false;
    TS_ASSERT(wsexist);

    DataObjects::MaskWorkspace_sptr maskWS = boost::dynamic_pointer_cast<DataObjects::MaskWorkspace>(outputWS);
    wsexist = true;
    if (!maskWS)
      wsexist = false;
    TS_ASSERT(wsexist);

    size_t newnumhist = maskWS->getNumberHistograms();
    TS_ASSERT_EQUALS(newnumhist, numhist);

    for (size_t i = 0; i < static_cast<size_t>(numhist); i+=2){
      TS_ASSERT_DELTA(maskWS->dataY(i)[0], 0, 1.0E-10);
    }
    for (size_t i = 1; i < static_cast<size_t>(numhist); i+=2){
      TS_ASSERT_DELTA(maskWS->dataY(i)[0], 1, 1.0E-10);
    }

    // TS_ASSERT_EQUALS(1, 22);

  }


};


#endif /* MANTID_ALGORITHMS_CONVERTTOMASKINGWORKSPACETEST_H_ */
