// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_LOADDNSSCDEWTEST_H_
#define MANTID_MDALGORITHMS_LOADDNSSCDEWTEST_H_

#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadDNSEvent.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <boost/range/irange.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;

class LoadDNSEventTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadDNSEventTest *createSuite() { return new LoadDNSEventTest(); }
  static void destroySuite(LoadDNSEventTest *suite) { delete suite; }
  LoadDNSEventTest() {}

  void test_initialization() {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_confidence() {
    alg.initialize();
    alg.setPropertyValue("InputFile", m_badFileName);
    FileDescriptor baddescriptor(alg.getPropertyValue("InputFile"));
    TS_ASSERT_EQUALS(0, alg.confidence(baddescriptor));
    alg.setPropertyValue("InputFile", m_fileName);
    FileDescriptor descriptor(alg.getPropertyValue("InputFile"));
    TS_ASSERT_EQUALS(80, alg.confidence(descriptor));
  }

  void test_properties() {
    alg.initialize();
    std::string outWSName("LoadDNSEventTest_OutputWS");
    TS_ASSERT_EQUALS(alg.name(), "LoadDNSEvent");
    TS_ASSERT_THROWS(alg.setProperty("ChopperChannel", 5), std::invalid_argument);
    TS_ASSERT_EQUALS(alg.getPropertyValue("ChopperChannel"), "2");
    TS_ASSERT_EQUALS(alg.getPropertyValue("NumberOfTubes"), "128");
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumberOfTubes", "1"));

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ChopperChannel", "2"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SetBinBoundary", true));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputFile", m_fileName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
  }

  void test_excecutes() {
    alg.initialize();
    std::string outWSName("LoadDNSEventTest_OutputWS");
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ChopperChannel", "2"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumberOfTubes", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SetBinBoundary", true));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputFile", m_fileName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    EventWorkspace_sptr iws;
    TS_ASSERT_THROWS_NOTHING(iws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outWSName));
    TS_ASSERT(iws);

    TS_ASSERT_EQUALS(iws->getEventType(), EventType::TOF);
    TS_ASSERT_EQUALS(iws->size(), 1024); // number of detector cells for testing reduced to 1 tube 1024 pixels

    TS_ASSERT_EQUALS(iws->getNumDims(), 2);
    TS_ASSERT_EQUALS(iws->id(), "EventWorkspace");

    //// test dimensions
    const auto tofDim = iws->getDimension(0);
    TS_ASSERT(tofDim);
    TS_ASSERT_EQUALS(tofDim->getName(), "Time-of-flight");
    TS_ASSERT_EQUALS(tofDim->getNBins(), 1);

    const auto specDim = iws->getDimension(1);
    TS_ASSERT(specDim);
    TS_ASSERT_EQUALS(specDim->getName(), "Spectrum");
    TS_ASSERT_EQUALS(specDim->getNBins(), 1024); // number of detector cells for testing reduced to 1 tube 1024 pixels
    TS_ASSERT_EQUALS(specDim->getMinimum(), 0);
    TS_ASSERT_RELATION(std::greater<double>, specDim->getMaximum(), 0);

    TS_ASSERT_EQUALS(iws->getNumberEvents(), 184);
    TS_ASSERT_EQUALS(iws->getTofMax(), 6641.4000);
    TS_ASSERT_EQUALS(iws->getSpectrum(52).getNumberEvents(), 8);
    TS_ASSERT_DELTA(iws->getDimension(0)->getMaximum(), 6641.40000, 0.001);
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_use_pre_chopper_no_bin() {
    alg.initialize();
    std::string outWSName("LoadDNSEventTest_OutputWS");
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ChopperChannel", "2"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumberOfTubes", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputFile", m_fileName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SetBinBoundary", false));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DiscardPreChopperEvents", false));
    alg.execute();
    EventWorkspace_sptr iws;
    TS_ASSERT_THROWS_NOTHING(iws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outWSName));
    TS_ASSERT_EQUALS(iws->getNumberEvents(), 231);
    TS_ASSERT_EQUALS(iws->getDimension(0)->getMaximum(), 0.00); // histogram bins not set
    AnalysisDataService::Instance().remove(outWSName);
  }

private:
  LoadDNSEvent alg;
  const std::string m_fileName = "DNS_psd_150c_first_tube.mdat";
  const std::string m_badFileName = "dnstof.d_dat"; // some random file
};
#endif /* MANTID_MDALGORITHMS_LOADDNSSCDEWEST_H_ */
