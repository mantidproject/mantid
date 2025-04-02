// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadDetectorInfo.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidNexus/NeXusFile.hpp"

#include <Poco/File.h>
#include <boost/lexical_cast.hpp>
#include <vector>

using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::CountStandardDeviations;
using Mantid::HistogramData::LinearGenerator;

/* choose an instrument to test, we could test all instruments
 * every time but I think a detailed test on the smallest workspace
 * is enough as the other workspaces take a long time to process (Steve
 * Williams)
 */

// MARI
static const std::string RAWFILE = "MAR11001.raw";
static const double TIMEOFF = 3.9;
static const int MONITOR = 2;
static const int NUMRANDOM = 7;
static const int DETECTS[NUMRANDOM] = {4101, 4804, 1323, 1101, 3805, 1323, 3832};

namespace SmallTestDatFile {
const int NDETECTS = 6;
}

namespace {
std::string delta[] = {"4", "4.500", "4.500", "4.500", "-6.00", "0.000"};
std::string pressure[] = {"10.0000", "10.0000", "10.0000", "10.0001", "10.000", "10.0001"};
std::string wallThick[] = {"0.00080", "0.00080", "0.00080", "-0.00080", "0.00080", "9.500"};
std::string code[] = {"3", "1", "3", "3", "3", "3"};
std::string det_l2[] = {"1.5", "1.5", "1.5", "1.5", "1.5", "1.5"};
std::string det_theta[] = {"30", "35", "40", "45", "50", "55"};
std::string det_phi[] = {"-105", "-110", "-115", "-120", "-125", "-130"};

void writeSmallDatFile(const std::string &filename) {
  std::ofstream file(filename.c_str());
  const int NOTUSED = -123456;
  file << "DETECTOR.DAT writen by LoadDetectorInfoTest\n";
  file << 165888 << " " << 14 << '\n';
  file << "det no.  offset    l2     code     theta        phi         w_x     "
          "    w_y         w_z         f_x         f_y         f_z         a_x "
          "        a_y         a_z        det_1       det_2       det_3       "
          "det4\n";
  for (int detector = 0; detector < SmallTestDatFile::NDETECTS; ++detector) {
    file << detector << "\t" << delta[detector] << "\t" << det_l2[detector] << "\t" << code[detector] << "\t"
         << det_theta[detector] << "\t" << det_phi[detector] << "\t" << NOTUSED << "\t" << NOTUSED << "\t" << NOTUSED
         << "\t" << NOTUSED << "\t" << NOTUSED << "\t" << NOTUSED << "\t" << NOTUSED << "\t" << NOTUSED << "\t"
         << NOTUSED << "\t" << NOTUSED << "\t" << pressure[detector] << "\t" << wallThick[detector] << "\t" << NOTUSED
         << '\n';
  }
  file.close();
}

void writeDetNXSfile(const std::string &filename, const int nDets) {
  ::NeXus::File nxsfile(filename, NXACC_CREATE5);
  nxsfile.makeGroup("detectors.dat", "NXEntry", true);
  nxsfile.putAttr("version", "1.0");

  std::vector<int32_t> detID(nDets * 2);
  std::vector<float> timeOffsets(nDets * 2);
  std::vector<float> detCoord(nDets * 3);
  std::vector<float> detTrueSize(nDets * 3);
  std::vector<float> detFalseSize(nDets * 3);
  std::vector<float> detOrient(nDets * 3);
  std::vector<float> detStruct(nDets * 2);
  std::vector<float> detTubeIndex(nDets);

  int limit(6);
  int ic(0);
  for (int i = 0; i < nDets; i++) {
    detID[2 * i] = i;
    detID[2 * i + 1] = boost::lexical_cast<int>(code[ic]);
    timeOffsets[2 * i] = boost::lexical_cast<float>(delta[ic]);
    timeOffsets[2 * i + 1] = 0;

    detCoord[3 * i + 0] = boost::lexical_cast<float>(det_l2[ic]);
    detCoord[3 * i + 1] = boost::lexical_cast<float>(det_theta[ic]);
    detCoord[3 * i + 2] = boost::lexical_cast<float>(det_phi[ic]);

    detTrueSize[3 * i + 0] = 0.1f;
    detTrueSize[3 * i + 1] = 0.2f;
    detTrueSize[3 * i + 2] = 0.3f;

    detFalseSize[3 * i + 0] = 0.11f;
    detFalseSize[3 * i + 1] = 0.22f;
    detFalseSize[3 * i + 2] = 0.33f;

    detOrient[3 * i + 0] = static_cast<float>(i) * 0.1f;
    detOrient[3 * i + 1] = static_cast<float>(i) * 0.2f;
    detOrient[3 * i + 2] = static_cast<float>(i) * 0.3f;
    detStruct[2 * i + 0] = boost::lexical_cast<float>(pressure[ic]);
    detStruct[2 * i + 1] = boost::lexical_cast<float>(wallThick[ic]);

    detTubeIndex[i] = float(nDets + i);

    ic++;
    if (ic == limit)
      ic = 0;
  }

  ::NeXus::DimVector array_dims{nDets, 2};

  nxsfile.makeData("detID", NXnumtype::INT32, array_dims, true);
  nxsfile.putAttr("description", "DetectorID, DetectorType");
  nxsfile.putData(&detID[0]);
  nxsfile.closeData();

  nxsfile.makeData("timeOffsets", NXnumtype::FLOAT32, array_dims, true);
  nxsfile.putAttr("description", "DelayTime, DeadTime");
  nxsfile.putData(&timeOffsets[0]);
  nxsfile.closeData();

  nxsfile.makeData("detPressureAndWall", NXnumtype::FLOAT32, array_dims, true);
  nxsfile.putAttr("description", "He3_pressure_Bar, WallThicknes_m");
  nxsfile.putData(&detStruct[0]);
  nxsfile.closeData();

  array_dims[1] = 3;
  nxsfile.makeData("detSphericalCoord", NXnumtype::FLOAT32, array_dims, true);
  nxsfile.putAttr("description", "L2, Theta, Psi");
  nxsfile.putData(&detCoord[0]);
  nxsfile.closeData();

  nxsfile.makeData("detTrueSize", NXnumtype::FLOAT32, array_dims, true);
  nxsfile.putAttr("description", "W_x, W_y, W_z");
  nxsfile.putData(&detTrueSize[0]);
  nxsfile.closeData();

  nxsfile.makeData("detFalseSize", NXnumtype::FLOAT32, array_dims, true);
  nxsfile.putAttr("description", "F_x, F_y, F_z");
  nxsfile.putData(&detFalseSize[0]);
  nxsfile.closeData();

  nxsfile.makeData("detOrientation", NXnumtype::FLOAT32, array_dims, true);
  nxsfile.putAttr("description", "a_x, a_y, a_z");
  nxsfile.putData(&detOrient[0]);
  nxsfile.closeData();

  array_dims[1] = 1;
  nxsfile.makeData("detTubeIndex", NXnumtype::FLOAT32, array_dims, true);
  nxsfile.putAttr("description", "detTubeIndex");
  nxsfile.putData(&detTubeIndex[0]);
  nxsfile.closeData();

  nxsfile.closeGroup();
  nxsfile.close();
}
void writeLargeTestDatFile(const std::string &filename, const int ndets) {
  // The array is the same value across all spectra as this is meant as a
  // performance test not a validation test

  std::ofstream file(filename.c_str());
  const int NOTUSED = -123456;
  file << "DETECTOR.DAT writen by LoadDetectorInfoTest\n";
  file << 165888 << " " << 14 << '\n';
  file << "det no.  offset    l2     code     theta        phi         w_x     "
          "    w_y         w_z         f_x         f_y         f_z         a_x "
          "        a_y         a_z        det_1       det_2       det_3       "
          "det4\n";
  for (int i = 0; i < ndets; ++i) {
    file << i << "\t" << delta[0] << "\t" << det_l2[0] << "\t" << code[0] << "\t" << det_theta[0] << "\t" << det_phi[0]
         << "\t" << NOTUSED << "\t" << NOTUSED << "\t" << NOTUSED << "\t" << NOTUSED << "\t" << NOTUSED << "\t"
         << NOTUSED << "\t" << NOTUSED << "\t" << NOTUSED << "\t" << NOTUSED << "\t" << NOTUSED << "\t" << pressure[0]
         << "\t" << wallThick[0] << "\t" << NOTUSED << '\n';
  }
  file.close();
}

// Set up a small workspace for testing
void makeTestWorkspace(const int ndets, const int nbins, const std::string &ads_name) {
  auto space2D = createWorkspace<Workspace2D>(ndets, nbins + 1, nbins);
  space2D->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
  BinEdges xs(nbins + 1, LinearGenerator(0.0, 1.0));
  CountStandardDeviations errors(nbins, 1.0);
  for (int j = 0; j < ndets; ++j) {
    space2D->setBinEdges(j, xs);
    // the y values will be different for each spectra (1+index_number) but the
    // same for each bin
    space2D->setCounts(j, nbins, j + 1);
    space2D->setCountStandardDeviations(j, errors);
    auto &spec = space2D->getSpectrum(j);
    spec.setDetectorID(j);
  }

  Instrument_sptr instr(new Instrument);
  Component *samplePos = new Component("sample-pos", instr.get());
  instr->add(samplePos);
  instr->markAsSamplePos(samplePos);

  for (int i = 0; i < ndets; ++i) {
    std::ostringstream os;
    os << "det-" << i;
    Detector *d = new Detector(os.str(), i, nullptr);
    instr->add(d);
    instr->markAsDetector(d);
  }
  space2D->setInstrument(instr);

  // Register the workspace in the data service
  AnalysisDataService::Instance().add(ads_name, space2D);
}
} // namespace

class LoadDetectorInfoTest : public CxxTest::TestSuite {
public:
  static LoadDetectorInfoTest *createSuite() { return new LoadDetectorInfoTest(); }
  static void destroySuite(LoadDetectorInfoTest *suite) { delete suite; }

  LoadDetectorInfoTest()
      : m_InoutWS("loaddetectorinfotest_input_workspace"), m_DatFile("loaddetectorinfotest_filename.dat"),
        m_NXSFile("loaddetectorinfotest_filename.nxs"), m_MariWS("MARfromRaw") {
    // create a .dat file in the current directory that we'll load later
    writeSmallDatFile(m_DatFile);
    // create correspondent nxs file
    writeDetNXSfile(m_NXSFile, SmallTestDatFile::NDETECTS);
    m_rawFile = RAWFILE;
  }

  ~LoadDetectorInfoTest() override {
    Poco::File(m_DatFile).remove();
    Poco::File(m_NXSFile).remove();
  }

  void testLoadDat() { loadDatFileTestHelper(m_DatFile); }

  void testLoadNXSmisc() {
    LoadDetectorInfo loadDetInfo;
    loadDetInfo.setRethrows(true);
    loadDetInfo.initialize();

    makeTestWorkspace(SmallTestDatFile::NDETECTS, NBINS, m_InoutWS);
    loadDetInfo.setPropertyValue("Workspace", m_InoutWS);
    loadDetInfo.setPropertyValue("DataFilename", "ARGUS00073601.nxs");
    TS_ASSERT_THROWS(loadDetInfo.execute(), const std::invalid_argument &);

    AnalysisDataService::Instance().remove(m_InoutWS);
  }

  void testLoadNXS() { loadDatFileTestHelper(m_NXSFile); }

  void testLoadLibisis() { loadDatFileTestHelper("det_nxs_libisis.nxs", true); }

  void testFromRaw() {
    LoadDetectorInfo grouper;

    TS_ASSERT_THROWS_NOTHING(grouper.initialize());
    TS_ASSERT(grouper.isInitialized());

    loadRawFile();
    MatrixWorkspace_sptr WS =
        std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(m_MariWS));

    // check the X-values for a sample of spectra avoiding the monitors
    grouper.setPropertyValue("Workspace", m_MariWS);
    grouper.setPropertyValue("DataFilename", m_rawFile);
    grouper.setPropertyValue("RelocateDets", "1");

    TS_ASSERT_THROWS_NOTHING(grouper.execute());
    TS_ASSERT(grouper.isExecuted());

    const auto &pmap = WS->constInstrumentParameters();
    const auto &detInfo = WS->detectorInfo();

    // read the parameters from some random detectors, they're parameters are
    // all set to the same thing
    for (int detector : DETECTS) {
      const size_t detIndex = detInfo.indexOf(detector);

      const auto &det = detInfo.detector(detIndex);
      Parameter_sptr par = pmap.getRecursive(&det, "TubePressure");

      TS_ASSERT(par);
      TS_ASSERT_DELTA(par->value<double>(), 10.0, 1.e-6);
      par = pmap.getRecursive(&det, "TubeThickness");
      TS_ASSERT(par);

      TS_ASSERT_DELTA(par->value<double>(), 0.0008, 1.e-6);
    }

    // Test that a random detector has been moved
    const V3D pos = detInfo.position(detInfo.indexOf(DETECTS[0]));
    TS_ASSERT_EQUALS(V3D(0, 0.2406324, 4.014795), pos);

    AnalysisDataService::Instance().remove(m_MariWS);
  }

  void loadRawFile() {
    LoadRaw3 loader;
    loader.initialize();

    loader.setPropertyValue("Filename", m_rawFile);
    loader.setPropertyValue("OutputWorkspace", m_MariWS);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
  }

  void loadDatFileTestHelper(const std::string &fileName, bool singleWallPressure = false) {
    LoadDetectorInfo grouper;

    TS_ASSERT_EQUALS(grouper.name(), "LoadDetectorInfo");
    TS_ASSERT_EQUALS(grouper.version(), 1);
    TS_ASSERT_THROWS_NOTHING(grouper.initialize());
    TS_ASSERT(grouper.isInitialized());

    // Set up a small workspace for testing
    makeTestWorkspace(SmallTestDatFile::NDETECTS, NBINS, m_InoutWS);
    grouper.setPropertyValue("Workspace", m_InoutWS);
    grouper.setPropertyValue("DataFilename", fileName);
    grouper.setPropertyValue("RelocateDets", "1");

    grouper.setRethrows(true);

    TS_ASSERT_THROWS_NOTHING(grouper.execute());
    TS_ASSERT(grouper.isExecuted());

    MatrixWorkspace_const_sptr WS =
        std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(m_InoutWS));

    const auto &pmap = WS->constInstrumentParameters();
    const auto &detInfo = WS->detectorInfo();

    for (int j = 0; j < SmallTestDatFile::NDETECTS; ++j) {

      const size_t detIndex = detInfo.indexOf(j);
      const IComponent *baseComp = detInfo.detector(detIndex).getComponentID();

      Parameter_sptr par = pmap.get(baseComp, "TubePressure");
      // this is only for PSD detectors, code 3
      if (code[j] == "3") {
        TS_ASSERT(par);
        if (!par)
          return;
        TS_ASSERT_DELTA(par->value<double>(), boost::lexical_cast<double>(pressure[j]), 1.e-3);

        par = pmap.get(baseComp, "TubeThickness");
        TS_ASSERT(par);
        if (!par)
          return;
        if (!singleWallPressure || j < 3)
          TS_ASSERT_DELTA(par->value<double>(), boost::lexical_cast<double>(wallThick[j]), 1.e-3);
        const V3D pos = detInfo.position(detIndex);
        V3D expected;
        if (j == 1) // Monitors are fixed and unaffected
        {
          expected = V3D(0, 0, 0);
        } else {
          expected.spherical(boost::lexical_cast<double>(det_l2[j]), boost::lexical_cast<double>(det_theta[j]),
                             boost::lexical_cast<double>(det_phi[j]));
        }
        TS_ASSERT_EQUALS(expected.X(), pos.X());
        TS_ASSERT_EQUALS(expected.Y(), pos.Y());
        TS_ASSERT_EQUALS(expected.Z(), pos.Z());
      } else {
        TS_ASSERT(!par);
      }
    }

    AnalysisDataService::Instance().remove(m_InoutWS);
  }

private:
  const std::string m_InoutWS, m_DatFile, m_NXSFile, m_MariWS;
  std::string m_rawFile;

  enum constants { NBINS = 4, DAT_MONTOR_IND = 1 };
};

//-------------------------------------------------------------------------------------------------------------------------
// Performance test
//-------------------------------------------------------------------------------------------------------------------------

class LoadDetectorInfoTestPerformance : public CxxTest::TestSuite {
public:
  static LoadDetectorInfoTestPerformance *createSuite() { return new LoadDetectorInfoTestPerformance(); }
  static void destroySuite(LoadDetectorInfoTestPerformance *suite) { delete suite; }

  LoadDetectorInfoTestPerformance()
      : m_testfile("LoadDetectorInfoTestPerformance_largefile.dat"), m_wsName("LoadDetectorInfoTestPerformance") {}

  void setUp() override {
    // 100,000 histograms
    const int ndets(100000);
    writeLargeTestDatFile(m_testfile, ndets);
    // 1000 bins
    makeTestWorkspace(100000, 1000, m_wsName); // Adds it to the ADS
  }

  void tearDown() override {
    Poco::File(m_testfile).remove();
    AnalysisDataService::Instance().remove(m_wsName);
  }

  void test_performance_on_large_data_set() {
    LoadDetectorInfo alg;
    alg.initialize();
    alg.setPropertyValue("Workspace", m_wsName);
    alg.setPropertyValue("DataFilename", m_testfile);
    alg.setPropertyValue("RelocateDets", "1");
    alg.execute();
  }

private:
  std::string m_testfile;
  std::string m_wsName;
};
