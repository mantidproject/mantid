#ifndef MANTID_MDALGORITHMS_CONVERTCWSDEXPTOMOMENTUMTEST_H_
#define MANTID_MDALGORITHMS_CONVERTCWSDEXPTOMOMENTUMTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidMDAlgorithms/ConvertCWSDExpToMomentum.h"

using Mantid::MDAlgorithms::ConvertCWSDExpToMomentum;
using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

namespace {
API::ITableWorkspace_sptr m_dataTableWS;
API::ITableWorkspace_sptr m_detectorTableWS;
std::vector<double> m_sourcePos;
std::vector<double> m_samplePos;
std::vector<double> m_pixelDimension;

void generateTestInputs() {
  // Create data table
  DataObjects::TableWorkspace_sptr datatable =
      boost::make_shared<DataObjects::TableWorkspace>();
  datatable->addColumn("int", "Scan No");
  datatable->addColumn("int", "Pt. No");
  datatable->addColumn("str", "File Name");
  datatable->addColumn("int", "Starting DetID");
  datatable->addColumn("int", "Monitor");
  datatable->addColumn("double", "Time");
  TableRow row0 = datatable->appendRow();
  row0 << 1 << 522 << "HB3A_exp355_scan0001_0522.xml" << 256 * 256 << 1000
       << 1.1;
  m_dataTableWS = boost::dynamic_pointer_cast<ITableWorkspace>(datatable);
  TS_ASSERT(m_dataTableWS);

  // Create detector table
  DataObjects::TableWorkspace_sptr dettable =
      boost::make_shared<DataObjects::TableWorkspace>();
  dettable->addColumn("int", "DetID");
  dettable->addColumn("double", "X");
  dettable->addColumn("double", "Y");
  dettable->addColumn("double", "Z");
  dettable->addColumn("int", "OriginalDetID");
  for (size_t i = 0; i < 256; ++i) {
    TableRow detrow = dettable->appendRow();
    double x = 0.38 + static_cast<double>(i - 128) * 0.001;
    double y = 0;
    double z = 0.38 + static_cast<double>(i - 128) * 0.001;
    int detid = static_cast<int>(i) + 1;
    detrow << detid << x << y << z << detid;
  }

  m_detectorTableWS = boost::dynamic_pointer_cast<ITableWorkspace>(dettable);
  TS_ASSERT(m_detectorTableWS);

  AnalysisDataService::Instance().addOrReplace("DataFileTable", m_dataTableWS);
  AnalysisDataService::Instance().addOrReplace("DetectorTable",
                                               m_detectorTableWS);

  // Source and sample position
  m_sourcePos.resize(3, 0.0);
  m_sourcePos[2] = -2;

  m_samplePos.resize(3, 0.0);

  m_pixelDimension.resize(8, 0.0);
}
} // namespace

class ConvertCWSDExpToMomentumTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertCWSDExpToMomentumTest *createSuite() {
    return new ConvertCWSDExpToMomentumTest();
  }
  static void destroySuite(ConvertCWSDExpToMomentumTest *suite) {
    delete suite;
  }

  void test_Init() {
    // Create tableworkspaces as inputs
    generateTestInputs();

    ConvertCWSDExpToMomentum testalg;
    testalg.initialize();
    TS_ASSERT(testalg.isInitialized());
  }

  void test_LoadConvert1File() {
    // Init and set up
    ConvertCWSDExpToMomentum testalg;
    testalg.initialize();

    testalg.setProperty("InputWorkspace", "DataFileTable");
    testalg.setProperty("CreateVirtualInstrument", true);
    testalg.setProperty("DetectorTableWorkspace", "DetectorTable");
    testalg.setProperty("SourcePosition", m_sourcePos);
    testalg.setProperty("SamplePosition", m_samplePos);
    testalg.setProperty("PixelDimension", m_pixelDimension);
    testalg.setProperty("OutputWorkspace", "QSampleMDEvents");
    testalg.setProperty("IsBaseName", false);
    testalg.setProperty("Directory", ".");

    testalg.execute();
    TS_ASSERT(testalg.isExecuted());

    API::IMDEventWorkspace_sptr outws =
        boost::dynamic_pointer_cast<IMDEventWorkspace>(
            AnalysisDataService::Instance().retrieve("QSampleMDEvents"));
    TS_ASSERT(outws);

    auto mditer = outws->createIterator();
    TS_ASSERT_EQUALS(mditer->getNumEvents(), 7400);

    size_t numexpinfo = outws->getNumExperimentInfo();
    TS_ASSERT_EQUALS(numexpinfo, 1);

    ExperimentInfo_const_sptr expinfo0 = outws->getExperimentInfo(0);
    Geometry::Instrument_const_sptr instrument = expinfo0->getInstrument();
    TS_ASSERT_EQUALS(instrument->getNumberDetectors(), 256);

    // Test the frame type
    for (size_t dim = 0; dim < outws->getNumDims(); ++dim) {
      const auto &frame = outws->getDimension(dim)->getMDFrame();
      TSM_ASSERT_EQUALS("Should be convertible to a QSample frame",
                        Mantid::Geometry::QSample::QSampleName, frame.name());
    }

    return;
  }

  void test_CopyInstrument() {
    // Init and set up
    ConvertCWSDExpToMomentum testalg;
    testalg.initialize();

    testalg.setProperty("InputWorkspace", "DataFileTable");
    testalg.setProperty("CreateVirtualInstrument", false);
    testalg.setProperty("OutputWorkspace", "QSampleMDEvents");
    testalg.setProperty("IsBaseName", false);
    testalg.setProperty("Directory", ".");

    testalg.execute();
    TS_ASSERT(testalg.isExecuted());

    API::IMDEventWorkspace_sptr outws =
        boost::dynamic_pointer_cast<IMDEventWorkspace>(
            AnalysisDataService::Instance().retrieve("QSampleMDEvents"));
    TS_ASSERT(outws);

    auto mditer = outws->createIterator();
    TS_ASSERT_EQUALS(mditer->getNumEvents(), 7400);

    size_t numexpinfo = outws->getNumExperimentInfo();
    TS_ASSERT_EQUALS(numexpinfo, 1);

    ExperimentInfo_const_sptr expinfo0 = outws->getExperimentInfo(0);
    Geometry::Instrument_const_sptr instrument = expinfo0->getInstrument();
    TS_ASSERT_EQUALS(instrument->getNumberDetectors(), 256 * 256);

    return;
  }

private:
  Geometry::Instrument_sptr createInstrument() {
    // Create a virtual instrument
    std::vector<Kernel::V3D> vec_detpos;
    vec_detpos.reserve(256);
    std::vector<detid_t> vec_detid;
    vec_detid.reserve(256);
    Kernel::V3D sourcePos(0., 0., -2.);
    Kernel::V3D samplePos(0., 0., 0.);

    for (size_t i = 0; i < 256; ++i) {
      double x = 0.38 + static_cast<double>(i - 128) * 0.001;
      double y = 0;
      double z = 0.38 + static_cast<double>(i - 128) * 0.001;
      vec_detid.emplace_back(static_cast<detid_t>(i) + 1);
      vec_detpos.emplace_back(x, y, z);
    }

    Geometry::Instrument_sptr virtualInstrument =
        Geometry::ComponentHelper::createVirtualInstrument(
            sourcePos, samplePos, vec_detpos, vec_detid);
    TS_ASSERT(virtualInstrument);

    return virtualInstrument;
  }
};

class ConvertCWSDExpToMomentumTestPerformance : public CxxTest::TestSuite {
public:
  static ConvertCWSDExpToMomentumTestPerformance *createSuite() {
    return new ConvertCWSDExpToMomentumTestPerformance();
  }
  static void destroySuite(ConvertCWSDExpToMomentumTestPerformance *suite) {
    delete suite;
  }

  void setUp() override {
    generateTestInputs();
    alg.initialize();
    alg.setProperty("InputWorkspace", "DataFileTable");
    alg.setProperty("CreateVirtualInstrument", true);
    alg.setProperty("DetectorTableWorkspace", "DetectorTable");
    alg.setProperty("SourcePosition", m_sourcePos);
    alg.setProperty("SamplePosition", m_samplePos);
    alg.setProperty("PixelDimension", m_pixelDimension);
    alg.setProperty("OutputWorkspace", "QSampleMDEvents");
    alg.setProperty("IsBaseName", false);
    alg.setProperty("Directory", ".");
  }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void testConvertCWSDExpToMomentumPerformance() {
    TS_ASSERT_THROWS_NOTHING(alg.execute());
  }

private:
  ConvertCWSDExpToMomentum alg;
};

#endif /* MANTID_MDALGORITHMS_CONVERTCWSDEXPTOMOMENTUMTEST_H_ */
