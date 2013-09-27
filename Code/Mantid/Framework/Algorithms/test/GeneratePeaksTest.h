#ifndef MANTID_ALGORITHMS_GENERATEPEAKSTEST_H_
#define MANTID_ALGORITHMS_GENERATEPEAKSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidAlgorithms/GeneratePeaks.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/FrameworkManager.h"

#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionDomain1D.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class GeneratePeaksTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GeneratePeaksTest *createSuite() { return new GeneratePeaksTest(); }
  static void destroySuite( GeneratePeaksTest *suite ) { delete suite; }

  GeneratePeaksTest()
  {
    FrameworkManager::Instance();
  }

  void test_Init()
  {
    GeneratePeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    DataObjects::TableWorkspace_sptr peakparmsws = createTestPeakParameters();

    TS_ASSERT_EQUALS(peakparmsws->rowCount(), 4);

    API::Column_sptr col0 = peakparmsws->getColumn("spectrum");
    API::Column_sptr col1 = peakparmsws->getColumn("centre");

    TS_ASSERT_EQUALS((*col0)[2], 2);
    TS_ASSERT_DELTA((*col1)[1], 8.0, 1.0E-8);

    return;
  }

  /*
   * Test to use user-provided binning parameters
   */
  void test_UserBinningParameters()
  {
    // 1. Create input
    DataObjects::TableWorkspace_sptr peakparmsws = createTestPeakParameters();

    GeneratePeaks alg;
    alg.initialize();

    // 3. Set value
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakParametersWorkspace", peakparmsws));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakFunction", "Gaussian"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("BinningParameters", "0.0, 0.01, 10.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "Test01WS"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GenerateBackground", false));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaxAllowedChi2", 100.0));

    // 4. Execute
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // 5. Get result
    API::MatrixWorkspace_const_sptr peaksws =
        boost::dynamic_pointer_cast<API::MatrixWorkspace>(AnalysisDataService::Instance().retrieve("Test01WS"));
    TS_ASSERT(peaksws);

    // 6. Check result
    TS_ASSERT_EQUALS(peaksws->getNumberHistograms(), 2);

    // a) Peak 0:
    MantidVec p0_x = peaksws->readX(0);
    MantidVec p0_y = peaksws->readY(0);
    TS_ASSERT_DELTA(p0_x[200], 2.0, 1.0E-8);
    TS_ASSERT_DELTA(p0_y[200], 5.0, 1.0E-4);

    TS_ASSERT_DELTA(p0_x[201], 2.01, 1.0E-8);
    TS_ASSERT_DELTA(p0_y[201], 4.96546, 1.0E-4);

    // b) Peak 1:
    TS_ASSERT_DELTA(p0_x[800], 8.0, 1.0E-8);
    TS_ASSERT_DELTA(p0_y[800], 10.0, 1.0E-4);

    // c) Peak 2:
    MantidVec p1_x = peaksws->readX(1);
    MantidVec p1_y = peaksws->readY(1);
    TS_ASSERT_DELTA(p1_x[400], 4.0, 1.0E-8);
    TS_ASSERT_DELTA(p1_y[400], 20.0, 1.0E-4);

    // 7. Spectrum map
    spec2index_map themap = peaksws->getSpectrumToWorkspaceIndexMap();
    size_t index0 = themap[0];
    size_t index2 = themap[2];
    TS_ASSERT_EQUALS(index0, 0);
    TS_ASSERT_EQUALS(index2, 1)

    AnalysisDataService::Instance().remove("Test01WS");

    return;
  }

  void test_FromInputWorkspace()
  {
    // 1. Create input
    DataObjects::TableWorkspace_sptr peakparmsws = createTestPeakParameters();
    API::MatrixWorkspace_sptr inputws = createTestInputWorkspace();

    GeneratePeaks alg;
    alg.initialize();

    // 3. Set value
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakParametersWorkspace", peakparmsws));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakFunction", "Gaussian"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputws));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("BinningParameters", "0.0, 0.01, 10.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "Test02WS"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GenerateBackground", false));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaxAllowedChi2", 100.0));

    // 4. Execute
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // 5. Get result
    API::MatrixWorkspace_const_sptr peaksws =
        boost::dynamic_pointer_cast<API::MatrixWorkspace>(AnalysisDataService::Instance().retrieve("Test02WS"));
    TS_ASSERT(peaksws);

    // 6. Check result
    TS_ASSERT_EQUALS(peaksws->getNumberHistograms(), 5);

    // a) Peak 0:
    MantidVec p0_x = peaksws->readX(0);
    MantidVec p0_y = peaksws->readY(0);
    TS_ASSERT_DELTA(p0_x[50], 2.0, 1.0E-8);
    TS_ASSERT_DELTA(p0_y[50], 5.0, 1.0E-4);

    TS_ASSERT_DELTA(p0_x[51], 2.02, 1.0E-8);
    TS_ASSERT_DELTA(p0_y[51], 4.86327, 1.0E-4);

    // b) Peak 1:
    TS_ASSERT_DELTA(p0_x[350], 8.0, 1.0E-8);
    TS_ASSERT_DELTA(p0_y[350], 10.0, 1.0E-4);

    // c) Peak 2:
    MantidVec p1_x = peaksws->readX(2);
    MantidVec p1_y = peaksws->readY(2);
    TS_ASSERT_DELTA(p1_x[150], 4.0, 1.0E-8);
    TS_ASSERT_DELTA(p1_y[150], 20.0, 1.0E-4);

    // 7. Spectrum map
    spec2index_map themap = peaksws->getSpectrumToWorkspaceIndexMap();
    TS_ASSERT_EQUALS(themap.size(), 5);
    size_t index0 = themap[0];
    size_t index2 = themap[2];
    TS_ASSERT_EQUALS(index0, 0);
    TS_ASSERT_EQUALS(index2, 1)

    return;
  }

  /*
   * Test to use user-provided binning parameters
   */
  void test_Background()
  {
    // 1. Create input
    DataObjects::TableWorkspace_sptr peakparmsws = createTestPeakParameters();

    GeneratePeaks alg;
    alg.initialize();

    // 3. Set value
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakParametersWorkspace", peakparmsws));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakFunction", "Gaussian"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("BinningParameters", "0.0, 0.01, 10.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "Test01WS"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GenerateBackground", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaxAllowedChi2", 100.0));

    // 4. Execute
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // 5. Get result
    API::MatrixWorkspace_const_sptr peaksws =
        boost::dynamic_pointer_cast<API::MatrixWorkspace>(AnalysisDataService::Instance().retrieve("Test01WS"));
    TS_ASSERT(peaksws);

    // 6. Check result
    TS_ASSERT_EQUALS(peaksws->getNumberHistograms(), 2);

    // a) Peak 0:
    MantidVec p0_x = peaksws->readX(0);
    MantidVec p0_y = peaksws->readY(0);
    TS_ASSERT_DELTA(p0_x[200], 2.0, 1.0E-8);
    TS_ASSERT_DELTA(p0_y[200], 10.0, 1.0E-4);

    // b) Peak 1:
    TS_ASSERT_DELTA(p0_x[800], 8.0, 1.0E-8);
    TS_ASSERT_DELTA(p0_y[800], 20.0, 1.0E-4);

    // c) Peak 2:
    MantidVec p1_x = peaksws->readX(1);
    MantidVec p1_y = peaksws->readY(1);
    TS_ASSERT_DELTA(p1_x[400], 4.0, 1.0E-8);
    TS_ASSERT_DELTA(p1_y[400], 24.0, 1.0E-4);

    // 7. Spectrum map
    spec2index_map themap = peaksws->getSpectrumToWorkspaceIndexMap();
    size_t index0 = themap[0];
    size_t index2 = themap[2];
    TS_ASSERT_EQUALS(index0, 0);
    TS_ASSERT_EQUALS(index2, 1)

    AnalysisDataService::Instance().remove("Test01WS");
    return;
  }

  /*
   * Generate a TableWorkspace containing 3 peaks on 2 spectra
   * spectra 0:  center = 2.0, width = 0.2, height = 5,  a0 = 1.0, a1 = 2.0, a2 = 0
   * spectra 0:  center = 8.0, width = 0.1, height = 10, a0 = 2.0, a1 = 1.0, a2 = 0
   * spectra 2:  center = 4.0, width = 0.4, height = 20, a0 = 4.0, a1 = 0.0, a2 = 0
   */
  DataObjects::TableWorkspace_sptr createTestPeakParameters()
  {
    // 1. Build a TableWorkspace
    DataObjects::TableWorkspace_sptr peakparms =
        boost::shared_ptr<DataObjects::TableWorkspace>(new DataObjects::TableWorkspace);
    peakparms->addColumn("int", "spectrum");
    peakparms->addColumn("double", "centre");
    peakparms->addColumn("double", "width");
    peakparms->addColumn("double", "height");
    peakparms->addColumn("double", "backgroundintercept");
    peakparms->addColumn("double", "backgroundslope");
    peakparms->addColumn("double", "A2");
    peakparms->addColumn("double", "chi2");

    // 2. Add value
    API::TableRow row0 = peakparms->appendRow();
    row0 << 0 << 2.0 << 0.2 <<  5.0 << 1.0 << 2.0 << 0.0 << 0.1;
    API::TableRow row1 = peakparms->appendRow();
    row1 << 0 << 8.0 << 0.1 << 10.0 << 2.0 << 1.0 << 0.0 << 0.2;
    API::TableRow row2 = peakparms->appendRow();
    row2 << 2 << 4.0 << 0.4 << 20.0 << 4.0 << 0.0 << 0.0 << 0.2;
    API::TableRow row3 = peakparms->appendRow();
    row3 << 2 << 4.5 << 0.4 << 20.0 << 1.0 << 9.0 << 0.0 << 1000.2;


    return peakparms;
  }

  /*
   * Create a MatrixWorkspace containing 5 spectra
   * Binning parameter = 1.0, 0.02, 9.0
   */
  API::MatrixWorkspace_sptr createTestInputWorkspace()
  {
    // 1. Create empty workspace
    double minx = 1.0;
    double maxx = 9.0;
    double dx = 0.02;
    size_t size = static_cast<size_t>((maxx-minx)/dx)+1;
    API::MatrixWorkspace_sptr inpWS = API::WorkspaceFactory::Instance().create("Workspace2D", 5, size, size-1);

    // 2. Put x values and y values
    for (size_t iw = 0; iw < inpWS->getNumberHistograms(); iw ++)
    {
      for (size_t ix = 0; ix < inpWS->dataX(iw).size(); ++ix)
      {
        inpWS->dataX(iw)[ix] = minx + double(ix)*dx;
      }
      for (size_t iy = 0; iy < inpWS->dataY(iw).size(); ++iy)
      {
        inpWS->dataY(iw)[iy] = 100.0;
      }
    }

    return inpWS;
  }

};


#endif /* MANTID_ALGORITHMS_GENERATEPEAKSTEST_H_ */
