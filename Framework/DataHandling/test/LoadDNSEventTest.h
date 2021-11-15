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
#include "Poco/Path.h"
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
  bool hasVTPDirectory;
  std::string origVTPDirectory;
  const std::string vtpDirectoryKey = "instrumentDefinition.vtp.directory";

  void setUp() { // DNS file slow to create geometry cache so use a pregenerated vtp file.
    std::string foundFile =
        Kernel::ConfigService::Instance().getFullPath("DNS-PSD03880f4077f70955e27452d25f5225b2327af287.vtp", true, 0);
    hasVTPDirectory = ConfigService::Instance().hasProperty(vtpDirectoryKey);
    origVTPDirectory = ConfigService::Instance().getString(vtpDirectoryKey);
    ConfigService::Instance().setString(vtpDirectoryKey, Poco::Path(foundFile).parent().toString());
  }

  void tearDown() {
    if (hasVTPDirectory)
      ConfigService::Instance().setString(vtpDirectoryKey, origVTPDirectory);
    else
      ConfigService::Instance().remove(vtpDirectoryKey);
  }

  LoadDNSEventTest() {}

  std::shared_ptr<LoadDNSEvent> makeAlgorithm(bool doesThrow = true) {
    std::shared_ptr<LoadDNSEvent> alg(new LoadDNSEvent());
    alg->setRethrows(doesThrow);
    alg->initialize();
    TS_ASSERT(alg->isInitialized());
    return alg;
  }

  std::shared_ptr<LoadDNSEvent> makeAlgorithm(const std::string &inputFile, const std::string &outputWorkspace,
                                              bool doesThrow = true) {
    auto alg = makeAlgorithm(doesThrow);
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("InputFile", inputFile));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("OutputWorkspace", outputWorkspace));
    return alg;
  }

  std::shared_ptr<LoadDNSEvent> makeAlgorithm(const std::string &inputFile, uint32_t chopperChannel,
                                              bool SetBinBoundary, const std::string &outputWorkspace,
                                              bool doesThrow = true) {
    auto alg = makeAlgorithm(inputFile, outputWorkspace, doesThrow);
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("chopperChannel", chopperChannel));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("SetBinBoundary", SetBinBoundary));
    return alg;
  }

  void test_Confidence() {
    LoadDNSEvent alg;
    alg.initialize();
    alg.setPropertyValue("InputFile", m_fileName);
    FileDescriptor descriptor(alg.getPropertyValue("InputFile"));
    TS_ASSERT_EQUALS(80, alg.confidence(descriptor));
    alg.setPropertyValue("InputFile", m_badFileName);
    TS_ASSERT_EQUALS(0, alg.confidence(descriptor));
  }

  void test_Init() {
    LoadDNSEvent alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  // void test_Name() {
  //  LoadDNSEvent alg;
  //  TS_ASSERT_EQUALS(alg.name(), "LoadDNSEvent");
  //}

  // void test_Properties() {
  //  auto alg = makeAlgorithm();
  //  TS_ASSERT_EQUALS(alg->getPropertyValue("chopperChannel"), "2");
  //  TS_ASSERT_THROWS(alg->setProperty("chopperChannel", 5), std::invalid_argument);
  //}

  // void test_Executes_1() {
  //  std::string outWSName("LoadDNSEventTest_OutputWS");
  //  auto alg = makeAlgorithm(m_fileName, 0, false, outWSName);
  //  TS_ASSERT_THROWS_NOTHING(alg->execute(););
  //  TS_ASSERT(alg->isExecuted());
  //}

  // void test_Executes_2() {
  //  std::string outWSName("LoadDNSEventTest_OutputWS");

  //  auto alg = makeAlgorithm(m_fileName, 2, false, outWSName);
  //  TS_ASSERT_THROWS_NOTHING(alg->execute(););
  //  TS_ASSERT(alg->isExecuted());
  //}

  // void test_ThrowsOnBadFile() {
  //  std::string outWSName("LoadDNSEventTest_OutputWS");

  //  auto alg = makeAlgorithm(m_badFileName, 2, false, outWSName);
  //  TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
  //  TS_ASSERT(!alg->isExecuted());
  //}

  // void test_DataWSStructure() {
  //  std::string outWSName("LoadDNSEventTest_OutputWS");

  //  auto alg = makeAlgorithm(m_fileName, 0, false, outWSName);
  //  alg->execute();

  //  // Retrieve the workspace from data service.
  //  EventWorkspace_sptr iws;
  //  TS_ASSERT_THROWS_NOTHING(iws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outWSName));
  //  TS_ASSERT(iws);

  //  TS_ASSERT_EQUALS(iws->getEventType(), EventType::TOF);
  //  TS_ASSERT_EQUALS(iws->size(), 1024 * 128); // number of detector cells

  //  TS_ASSERT_EQUALS(iws->getNumDims(), 2);
  //  TS_ASSERT_EQUALS(iws->id(), "EventWorkspace");

  //  // test dimensions
  //  const auto tofDim = iws->getDimension(0);
  //  TS_ASSERT(tofDim);
  //  TS_ASSERT_EQUALS(tofDim->getName(), "Time-of-flight");
  //  TS_ASSERT_EQUALS(tofDim->getNBins(), 1);

  //  const auto specDim = iws->getDimension(1);
  //  TS_ASSERT(specDim);
  //  TS_ASSERT_EQUALS(specDim->getName(), "Spectrum");
  //  TS_ASSERT_EQUALS(specDim->getNBins(),
  //                   1024 * 128); // number of detector cells
  //  TS_ASSERT_RELATION(std::greater<double>, specDim->getMinimum(), 0);
  //  TS_ASSERT_RELATION(std::greater<double>, specDim->getMaximum(), 0);

  //  // test event count:
  //  const auto rng = boost::irange(static_cast<size_t>(0), iws->size());
  //  const size_t eventCount = std::accumulate(rng.begin(), rng.end(), static_cast<size_t>(0), [&](auto a, auto b) {
  //    return a + iws->getSpectrum(b).getEvents().size();
  //  });
  //  TS_ASSERT_EQUALS(eventCount, 9998)
  //  TS_ASSERT_EQUALS(iws->getNumberEvents(), 9998);
  //  TS_ASSERT_EQUALS(iws->getTofMax(), 99471.3);
  //  TS_ASSERT_EQUALS(iws->getSpectrum(32217).getNumberEvents(), 808);
  //  TS_ASSERT_EQUALS(iws->getDimension(0)->getMaximum(), 0.00); // histogram bins not set
  //  AnalysisDataService::Instance().remove(outWSName);
  //}

  // void test_DiscardPreChopperEvents() {
  //  std::string outWSName("LoadDNSEventTest_OutputWS");
  //  auto alg = makeAlgorithm(m_fileName, 0, false, outWSName);
  //  TS_ASSERT_THROWS_NOTHING(alg->setProperty("DiscardPreChopperEvents", false));
  //  alg->execute();
  //  EventWorkspace_sptr iws;
  //  TS_ASSERT_THROWS_NOTHING(iws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outWSName));
  //  TS_ASSERT_EQUALS(iws->getNumberEvents(), 10520);
  //}

  // void test_SetBinBoundary() {
  //  std::string outWSName("LoadDNSEventTest_OutputWS");
  //  auto alg = makeAlgorithm(m_fileName, 0, true, outWSName);
  //  alg->execute();
  //  EventWorkspace_sptr iws;
  //  TS_ASSERT_THROWS_NOTHING(iws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outWSName));
  //  TS_ASSERT_DELTA(iws->getDimension(0)->getMaximum(), 99471.296, 0.001);
  //}

private:
  const std::string m_fileName = "DNS_psd_pulser_ON473_31.mdat";

  const std::string m_badFileName = "dnstof.d_dat"; // some random file
};
#endif /* MANTID_MDALGORITHMS_LOADDNSSCDEWEST_H_ */
