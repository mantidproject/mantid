#ifndef MANTID_ALGORITHMS_EXTRACTSPECTRATEST_H_
#define MANTID_ALGORITHMS_EXTRACTSPECTRATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ExtractSpectra.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::ExtractSpectra;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid;

class ExtractSpectraTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExtractSpectraTest *createSuite() { return new ExtractSpectraTest(); }
  static void destroySuite( ExtractSpectraTest *suite ) { 
    AnalysisDataService::Instance().clear();
    delete suite; 
  }

  ExtractSpectraTest()
      : nSpec(5), nBins(6),
        outWSName("ExtractSpectraTest_OutputWS") {}

  void test_Init()
  {
    ExtractSpectra alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_defaults()
  {
    Parameters params;
    auto ws = runAlgorithm(params);
    if (!ws) return;
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), nSpec);
    TS_ASSERT_EQUALS(ws->blocksize(), nBins);

    TS_ASSERT_EQUALS(ws->readX(0)[0], 0.0);
    TS_ASSERT_EQUALS(ws->readX(0)[1], 1.0);
    TS_ASSERT_EQUALS(ws->readX(0)[2], 2.0);
    TS_ASSERT_EQUALS(ws->readX(0)[3], 3.0);
    TS_ASSERT_EQUALS(ws->readX(0)[4], 4.0);
    TS_ASSERT_EQUALS(ws->readX(0)[5], 5.0);
    TS_ASSERT_EQUALS(ws->readX(0)[6], 6.0);
  }

  // ---- test histo ----

  void test_x_range()
  {
    Parameters params;
    params.setXRange();

    auto ws = runAlgorithm(params);
    if (!ws) return;

    TS_ASSERT_EQUALS(ws->getNumberHistograms(), nSpec);
    params.testXRange(*ws);
  }

  void test_index_range()
  {
    Parameters params;
    params.setIndexRange();

    auto ws = runAlgorithm(params);
    if (!ws) return;

    TS_ASSERT_EQUALS(ws->blocksize(), nBins);
    params.testIndexRange(*ws);
  }

  void test_spectrum_list()
  {
    Parameters params;
    params.setSpectrumList();

    auto ws = runAlgorithm(params);
    if (!ws) return;

    TS_ASSERT_EQUALS(ws->blocksize(), nBins);
    params.testSpectrumList(*ws);
  }

  void test_index_and_spectrum_list()
  {
    Parameters params;
    params.setSpectrumList().setIndexRange();

    auto ws = runAlgorithm(params);
    if (!ws) return;

    TS_ASSERT_EQUALS(ws->blocksize(), nBins);
    params.testSpectrumList(*ws);
  }

  void test_x_range_and_spectrum_list()
  {
    Parameters params;
    params.setSpectrumList().setXRange();

    auto ws = runAlgorithm(params);
    if (!ws) return;

    params.testXRange(*ws);
    params.testSpectrumList(*ws);
  }

  void test_invalid_x_range()
  {
    Parameters params;
    params.setInvalidXRange();

    auto ws = runAlgorithm(params, false);
  }

  void test_invalid_index_range()
  {
    {
      Parameters params;
      params.setInvalidIndexRange();
      auto ws = runAlgorithm(params, false);
    }
    {
      Parameters params;
      params.setInvalidIndexRange1();
      auto ws = runAlgorithm(params, false);
    }
  }


  // ---- test event ----

  void test_x_range_event()
  {
    Parameters params("event");
    params.setXRange();

    auto ws = runAlgorithm(params);
    if (!ws) return;

    TS_ASSERT_EQUALS(ws->getNumberHistograms(), nSpec);
    params.testXRange(*ws);
  }

  void test_index_range_event()
  {
    Parameters params("event");
    params.setIndexRange();

    auto ws = runAlgorithm(params);
    if (!ws) return;

    TS_ASSERT_EQUALS(ws->blocksize(), nBins);
    params.testIndexRange(*ws);
  }

  void test_spectrum_list_event()
  {
    Parameters params("event");
    params.setSpectrumList();

    auto ws = runAlgorithm(params);
    if (!ws) return;

    TS_ASSERT_EQUALS(ws->blocksize(), nBins);
    params.testSpectrumList(*ws);
  }

  void test_index_and_spectrum_list_event()
  {
    Parameters params("event");
    params.setSpectrumList().setIndexRange();

    auto ws = runAlgorithm(params);
    if (!ws) return;

    TS_ASSERT_EQUALS(ws->blocksize(), nBins);
    params.testSpectrumList(*ws);
  }

  void test_x_range_and_spectrum_list_event()
  {
    Parameters params("event");
    params.setSpectrumList().setXRange();

    auto ws = runAlgorithm(params);
    if (!ws) return;

    params.testXRange(*ws);
    params.testSpectrumList(*ws);
  }

  void test_invalid_x_range_event()
  {
    Parameters params("event");
    params.setInvalidXRange();
    auto ws = runAlgorithm(params, true);
    // this is a bit unexpected but at least no crash
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), nSpec);
    TS_ASSERT_EQUALS(ws->blocksize(), 1);
    TS_ASSERT_EQUALS(ws->readX(0)[0], 2);
    TS_ASSERT_EQUALS(ws->readX(0)[1], 1);
  }

  void test_invalid_index_range_event()
  {
    {
      Parameters params("event");
      params.setInvalidIndexRange();
      auto ws = runAlgorithm(params, false);
    }
    {
      Parameters params("event");
      params.setInvalidIndexRange1();
      auto ws = runAlgorithm(params, false);
    }
  }
  // ---- test histo-ragged ----

  void test_x_range_ragged()
  {
    Parameters params("histo-ragged");
    params.setXRange();

    auto ws = runAlgorithm(params);
    if (!ws) return;

    TS_ASSERT_EQUALS(ws->getNumberHistograms(), nSpec);
    params.testXRange(*ws);
  }

  void test_index_range_ragged()
  {
    Parameters params("histo-ragged");
    params.setIndexRange();

    auto ws = runAlgorithm(params);
    if (!ws) return;

    TS_ASSERT_EQUALS(ws->blocksize(), nBins);
    params.testIndexRange(*ws);
  }

  void test_spectrum_list_ragged()
  {
    Parameters params("histo-ragged");
    params.setSpectrumList();

    auto ws = runAlgorithm(params);
    if (!ws) return;

    TS_ASSERT_EQUALS(ws->blocksize(), nBins);
    params.testSpectrumList(*ws);
  }

  void xtest_invalid_x_range_ragged()
  {
    Parameters params("histo-ragged");
    params.setInvalidXRange();

    auto ws = runAlgorithm(params, false);
  }

private:

  // -----------------------  helper methods ------------------------

  const size_t nSpec;
  const size_t nBins;
  const std::string outWSName;

  MatrixWorkspace_sptr createInputWorkspace(const std::string& workspaceType) const
  {
    if (workspaceType == "histo")
      return createInputWorkspaceHisto();
    else if (workspaceType == "event")
      return createInputWorkspaceEvent();
    else if (workspaceType == "histo-ragged")
      return createInputWorkspaceHistoRagged();
    throw std::runtime_error("Undefined workspace type");
  }

  MatrixWorkspace_sptr createInputWorkspaceHisto() const
  {
    // Set up a small workspace for testing
    MatrixWorkspace_sptr space = WorkspaceFactory::Instance().create("Workspace2D", nSpec, nBins+1, nBins);
    for (size_t j = 0; j < nSpec; ++j) {
      for (size_t k = 0; k <= nBins; ++k) {
        space->dataX(j)[k] = double(k);
      }
      space->dataY(j).assign(nBins, double(j));
      space->dataE(j).assign(nBins, sqrt(double(j)));
    }
    return space;
  }

  MatrixWorkspace_sptr createInputWorkspaceHistoRagged() const
  {
    // Set up a small workspace for testing
    MatrixWorkspace_sptr space = WorkspaceFactory::Instance().create("Workspace2D", nSpec, nBins+1, nBins);
    for (size_t j = 0; j < nSpec; ++j) {
      for (size_t k = 0; k <= nBins; ++k) {
        space->dataX(j)[k] = double(j + k);
      }
      space->dataY(j).assign(nBins, double(j+1));
      space->dataE(j).assign(nBins, sqrt(double(j+1)));
    }
    return space;
  }

  MatrixWorkspace_sptr createInputWorkspaceEvent() const
  {
    EventWorkspace_sptr ws = WorkspaceCreationHelper::CreateEventWorkspace(int(nSpec), int(nBins), 50, 0.0, 1., 2);
    ws->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    ws->setInstrument( ComponentCreationHelper::createTestInstrumentCylindrical(1) );
    for(size_t i = 0; i < ws->getNumberHistograms(); ++i)
    {
      ws->getSpectrum(i)->setDetectorID(detid_t(i + 1));
    }
    return ws;
  }

  struct Parameters
  {
    Parameters(const std::string& workspaceType = "histo")
        : XMin(EMPTY_DBL()), XMax(EMPTY_DBL()), StartWorkspaceIndex(0),
        EndWorkspaceIndex(EMPTY_INT()), SpectrumList(), wsType(workspaceType)
    {
    }
    double XMin;
    double XMax;
    int StartWorkspaceIndex;
    int EndWorkspaceIndex;
    std::vector<int> SpectrumList;
    std::string wsType;

    // ---- x range ----
    Parameters& setXRange()
    {
      XMin = 2.0;
      XMax = 3.1;
      return *this;
    }
    void testXRange(const MatrixWorkspace& ws) const
    {
      if (wsType == "histo-ragged")
      {
        TS_ASSERT_EQUALS(ws.blocksize(), 6);
        TS_ASSERT_EQUALS(ws.readY(0)[0], 0.0);
        TS_ASSERT_EQUALS(ws.readY(0)[1], 0.0);
        TS_ASSERT_EQUALS(ws.readY(0)[2], 1.0);
        TS_ASSERT_EQUALS(ws.readY(0)[3], 0.0);
        TS_ASSERT_EQUALS(ws.readY(0)[4], 0.0);
        TS_ASSERT_EQUALS(ws.readY(0)[5], 0.0);

        TS_ASSERT_EQUALS(ws.readY(1)[0], 0.0);
        TS_ASSERT_EQUALS(ws.readY(1)[1], 2.0);
        TS_ASSERT_EQUALS(ws.readY(1)[2], 0.0);
        TS_ASSERT_EQUALS(ws.readY(1)[3], 0.0);
        TS_ASSERT_EQUALS(ws.readY(1)[4], 0.0);
        TS_ASSERT_EQUALS(ws.readY(1)[5], 0.0);

        TS_ASSERT_EQUALS(ws.readY(2)[0], 3.0);
        TS_ASSERT_EQUALS(ws.readY(2)[1], 0.0);
        TS_ASSERT_EQUALS(ws.readY(2)[2], 0.0);
        TS_ASSERT_EQUALS(ws.readY(2)[3], 0.0);
        TS_ASSERT_EQUALS(ws.readY(2)[4], 0.0);
        TS_ASSERT_EQUALS(ws.readY(2)[5], 0.0);

        TS_ASSERT_EQUALS(ws.readY(3)[0], 0.0);
        TS_ASSERT_EQUALS(ws.readY(3)[1], 0.0);
        TS_ASSERT_EQUALS(ws.readY(3)[2], 0.0);
        TS_ASSERT_EQUALS(ws.readY(3)[3], 0.0);
        TS_ASSERT_EQUALS(ws.readY(3)[4], 0.0);
        TS_ASSERT_EQUALS(ws.readY(3)[5], 0.0);
      }
      else
      {
        TS_ASSERT_EQUALS(ws.blocksize(), 1);
        TS_ASSERT_EQUALS(ws.readX(0)[0], 2.0);
      }
    }

    // ---- index range ----
    Parameters& setIndexRange()
    {
      StartWorkspaceIndex = 1;
      EndWorkspaceIndex = 3;
      return *this;
    }
    void testIndexRange(const MatrixWorkspace& ws) const
    {
      TS_ASSERT_EQUALS(ws.getNumberHistograms(), 3);
      if (wsType == "histo")
      {
        TS_ASSERT_EQUALS(ws.readY(0)[0], 1.0);
        TS_ASSERT_EQUALS(ws.readY(1)[0], 2.0);
        TS_ASSERT_EQUALS(ws.readY(2)[0], 3.0);
      }
      else if (wsType == "event")
      {
        TS_ASSERT_EQUALS(ws.getDetector(0)->getID(), 2);
        TS_ASSERT_EQUALS(ws.getDetector(1)->getID(), 3);
        TS_ASSERT_EQUALS(ws.getDetector(2)->getID(), 4);
      }
    }

    // ---- spectrum list ----
    Parameters& setSpectrumList()
    {
      SpectrumList.resize(3);
      SpectrumList[0] = 0;
      SpectrumList[1] = 2;
      SpectrumList[2] = 4;
      return *this;
    }
    void testSpectrumList(const MatrixWorkspace& ws) const
    {
      TS_ASSERT_EQUALS(ws.getNumberHistograms(), 3);
      if (wsType == "histo")
      {
        TS_ASSERT_EQUALS(ws.readY(0)[0], 0.0);
        TS_ASSERT_EQUALS(ws.readY(1)[0], 2.0);
        TS_ASSERT_EQUALS(ws.readY(2)[0], 4.0);
      }
      else if (wsType == "event")
      {
        TS_ASSERT_EQUALS(ws.getDetector(0)->getID(), 1);
        TS_ASSERT_EQUALS(ws.getDetector(1)->getID(), 3);
        TS_ASSERT_EQUALS(ws.getDetector(2)->getID(), 5);
      }
   }

    // ---- invalid inputs ----
    void setInvalidXRange()
    {
      XMin = 2.0;
      XMax = 1.0;
    }
    void setInvalidIndexRange()
    {
      StartWorkspaceIndex = 3;
      EndWorkspaceIndex = 1;
    }
    void setInvalidIndexRange1()
    {
      StartWorkspaceIndex = 1000;
      EndWorkspaceIndex = 1002;
    }
  };

  MatrixWorkspace_sptr runAlgorithm(const Parameters& params, bool expectSuccess = true) const
  {
    auto ws = createInputWorkspace(params.wsType);
    ExtractSpectra alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", ws) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );

    if (params.XMin != EMPTY_DBL())
    {
      TS_ASSERT_THROWS_NOTHING( alg.setProperty("XMin", params.XMin) );
    }
    if (params.XMax != EMPTY_DBL())
    {
      TS_ASSERT_THROWS_NOTHING( alg.setProperty("XMax", params.XMax) );
    }
    if (params.StartWorkspaceIndex != 0)
    {
      TS_ASSERT_THROWS_NOTHING( alg.setProperty("StartWorkspaceIndex", params.StartWorkspaceIndex) );
    }
    if (params.EndWorkspaceIndex != EMPTY_INT())
    {
      TS_ASSERT_THROWS_NOTHING( alg.setProperty("EndWorkspaceIndex", params.EndWorkspaceIndex) );
    }
    if (!params.SpectrumList.empty())
    {
      TS_ASSERT_THROWS_NOTHING( alg.setProperty("SpectrumList", params.SpectrumList) );
    }

    TS_ASSERT_THROWS_NOTHING( alg.execute(); );

    if (expectSuccess)
    {
      TS_ASSERT( alg.isExecuted() );

      // Retrieve the workspace from data service. TODO: Change to your desired type
      MatrixWorkspace_sptr ws;
      TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName) );
      return ws;
    }
    else
    {
      TS_ASSERT( !alg.isExecuted() );
    }

    return MatrixWorkspace_sptr();
  }

};


#endif /* MANTID_ALGORITHMS_EXTRACTSPECTRATEST_H_ */