#ifndef MANTID_DATAHANDLING_LOADFITSTEST_H_
#define MANTID_DATAHANDLING_LOADFITSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataHandling/LoadFITS.h"

using namespace Mantid::API;
using namespace Mantid::DataHandling;

class LoadFITSTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadFITSTest *createSuite() { return new LoadFITSTest(); }
  static void destroySuite(LoadFITSTest *suite) { delete suite; }

  void test_algorithm() {
    std::string name = "LoadFITS";
    int version = 1;
    testAlg =
        Mantid::API::AlgorithmManager::Instance().create(name /*, version*/);
    TS_ASSERT(testAlg);
    TS_ASSERT_EQUALS(testAlg->name(), name);
    TS_ASSERT_EQUALS(testAlg->version(), version);
  }

  void test_castAlgorithm() {
    // can create
    boost::shared_ptr<LoadFITS> a;
    TS_ASSERT(a = boost::make_shared<LoadFITS>());
    // can cast to inherited interfaces and base classes

    TS_ASSERT(dynamic_cast<Mantid::DataHandling::LoadFITS *>(a.get()));
    TS_ASSERT(dynamic_cast<Mantid::API::Algorithm *>(a.get()));
    TS_ASSERT(dynamic_cast<Mantid::Kernel::PropertyManagerOwner *>(a.get()));
    TS_ASSERT(dynamic_cast<Mantid::API::IAlgorithm *>(a.get()));
    TS_ASSERT(dynamic_cast<Mantid::Kernel::IPropertyManager *>(a.get()));
  }

  void test_initAlgorithm() {
    LoadFITS lf;
    TS_ASSERT_THROWS_NOTHING(lf.initialize());
  }

  void test_propertiesMissing() {
    LoadFITS lf;
    TS_ASSERT_THROWS_NOTHING(lf.initialize());
    TS_ASSERT_THROWS_NOTHING(lf.setPropertyValue("Filename", smallFname1));
    TS_ASSERT_THROWS(lf.execute(), std::runtime_error);
    TS_ASSERT(!lf.isExecuted());

    LoadFITS lf2;
    TS_ASSERT_THROWS_NOTHING(lf2.initialize());
    TS_ASSERT_THROWS_NOTHING(
        lf2.setPropertyValue("OutputWorkspace", "out_ws_name"));
    TS_ASSERT_THROWS(lf2.execute(), std::runtime_error);
    TS_ASSERT(!lf2.isExecuted());
  }

  void test_wrongProp() {
    LoadFITS lf;
    TS_ASSERT_THROWS_NOTHING(lf.initialize());
    TS_ASSERT_THROWS(lf.setPropertyValue("file", "anything"),
                     std::runtime_error);
    TS_ASSERT_THROWS(lf.setPropertyValue("output", "anything"),
                     std::runtime_error);
    TS_ASSERT_THROWS(lf.setPropertyValue("FITS", "anything"),
                     std::runtime_error);

    TS_ASSERT_THROWS(lf.setPropertyValue("BinSize", "-1"),
                     std::invalid_argument);
    TS_ASSERT_THROWS(lf.setPropertyValue("BinSize", "0"),
                     std::invalid_argument);
    TS_ASSERT_THROWS(lf.setPropertyValue("FilterNoiseLevel", "-10"),
                     std::invalid_argument);
    TS_ASSERT_THROWS_NOTHING(lf.setPropertyValue("FilterNoiseLevel", "0"));

    TS_ASSERT_THROWS(lf.setPropertyValue("ImageKey", "anything"),
                     Mantid::Kernel::Exception::NotFoundError);
    TS_ASSERT_THROWS(lf.setPropertyValue("BITPIX", "anything"),
                     std::runtime_error);
    TS_ASSERT_THROWS(lf.setPropertyValue("NAXIS", "anything"),
                     std::runtime_error);
    TS_ASSERT_THROWS(lf.setPropertyValue("NAXIS1", "anything"),
                     std::runtime_error);
  }

  void test_initGood() {
    TS_ASSERT_THROWS_NOTHING(algToBeTested.initialize());
    TS_ASSERT(algToBeTested.isInitialized());

    if (!algToBeTested.isInitialized())
      algToBeTested.initialize();

    outputSpace = "LoadFITSTest";
    algToBeTested.setPropertyValue("OutputWorkspace", outputSpace);

    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(algToBeTested.execute(), std::runtime_error);

    inputFile = smallFname1 + ", " + smallFname2;
    algToBeTested.setPropertyValue("Filename", inputFile);

    // Set the ImageKey to be 0 (this used to be required, but the key
    // should not be there any longer);
    TS_ASSERT_THROWS(algToBeTested.setProperty<int>("ImageKey", 0),
                     Mantid::Kernel::Exception::NotFoundError);
  }

  void test_performAssertions() {
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT(algToBeTested.isExecuted());
    //  get workspace generated
    WorkspaceGroup_sptr out;
    TS_ASSERT(AnalysisDataService::Instance().doesExist(outputSpace));
    TS_ASSERT_THROWS_NOTHING(
        out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
            outputSpace));
    TS_ASSERT_EQUALS(out->getNumberOfEntries(),
                     2); // Number of time bins should equal number of files
    MatrixWorkspace_sptr ws1;
    TS_ASSERT_THROWS_NOTHING(
        ws1 = boost::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(0)));
    MatrixWorkspace_sptr ws2;
    TS_ASSERT_THROWS_NOTHING(
        ws2 = boost::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(1)));

    // basic FITS headers
    const auto run = ws1->run();
    TS_ASSERT_EQUALS(run.getLogData("SIMPLE")->value(), hdrSIMPLE);
    TS_ASSERT_EQUALS(run.getLogData("BITPIX")->value(), hdrBITPIX);
    TS_ASSERT_EQUALS(run.getLogData("NAXIS")->value(), hdrNAXIS);
    TS_ASSERT_EQUALS(run.getLogData("NAXIS1")->value(), hdrNAXIS1);
    TS_ASSERT_EQUALS(run.getLogData("NAXIS2")->value(), hdrNAXIS2);

    // Number of spectra
    TS_ASSERT_EQUALS(ws1->getNumberHistograms(), SPECTRA_COUNT);
    TS_ASSERT_EQUALS(ws2->getNumberHistograms(), SPECTRA_COUNT);

    // Sum the two bins from the last spectra - should be 70400
    double sumY =
        ws1->readY(SPECTRA_COUNT - 1)[0] + ws2->readY(SPECTRA_COUNT - 1)[0];
    TS_ASSERT_EQUALS(sumY, 275);
    // Check the sum of the error values for the last spectra in each file -
    // should be 375.183
    double sumE =
        ws1->readE(SPECTRA_COUNT - 1)[0] + ws2->readE(SPECTRA_COUNT - 1)[0];
    TS_ASSERT_LESS_THAN(std::abs(sumE - 23.4489), 0.0001); // Include a small
    // tolerance check with
    // the assert - not
    // exactly 375.183
  }

  void test_noiseFilter() {
    testAlg =
        Mantid::API::AlgorithmManager::Instance().create("LoadFITS" /*, 1*/);

    TS_ASSERT_THROWS_NOTHING(testAlg->initialize());
    TS_ASSERT(testAlg->isInitialized());

    outputSpace = "LoadFITSFiltered";
    testAlg->setPropertyValue("OutputWorkspace", outputSpace);
    testAlg->setProperty("FilterNoiseLevel", 200.0);

    inputFile = smallFname1 + ", " + smallFname2;
    testAlg->setPropertyValue("Filename", inputFile);

    TS_ASSERT_THROWS_NOTHING(testAlg->execute());

    WorkspaceGroup_sptr out;
    TS_ASSERT(AnalysisDataService::Instance().doesExist(outputSpace));
    TS_ASSERT_THROWS_NOTHING(
        out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
            outputSpace));
    const int nws = 2;
    TS_ASSERT_EQUALS(out->getNumberOfEntries(), nws);

    double expectedY[nws] = {144, 149.0};
    double expectedE[nws] = {12, 12.2066};
    for (int i = 0; i < out->getNumberOfEntries(); ++i) {
      MatrixWorkspace_sptr ws;
      TS_ASSERT_THROWS_NOTHING(
          ws = boost::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(i)));

      TS_ASSERT_EQUALS(ws->getNumberHistograms(), SPECTRA_COUNT);

      // check Y and Error
      TS_ASSERT_EQUALS(ws->readY(SPECTRA_COUNT - 100)[0], expectedY[i]);
      TS_ASSERT_LESS_THAN(
          std::abs(ws->readE(SPECTRA_COUNT - 100)[0] - expectedE[i]), 0.0001);
    }
  }

  void test_rebinWrong() {
    testAlg =
        Mantid::API::AlgorithmManager::Instance().create("LoadFITS" /*, 1*/);

    TS_ASSERT_THROWS_NOTHING(testAlg->initialize());
    TS_ASSERT(testAlg->isInitialized());

    inputFile = smallFname1 + ", " + smallFname2;
    testAlg->setPropertyValue("Filename", inputFile);
    testAlg->setProperty("BinSize", 3);
    // this should fail - width and height not multiple of 3
    outputSpace = "LoadFITSx3";
    testAlg->setPropertyValue("OutputWorkspace", outputSpace);

    TS_ASSERT_THROWS_NOTHING(testAlg->execute());
    TS_ASSERT(!AnalysisDataService::Instance().doesExist(outputSpace));
  }

  void test_rebinOK() {
    testAlg =
        Mantid::API::AlgorithmManager::Instance().create("LoadFITS" /*, 1*/);

    TS_ASSERT_THROWS_NOTHING(testAlg->initialize());
    TS_ASSERT(testAlg->isInitialized());

    inputFile = smallFname1 + ", " + smallFname2;
    testAlg->setPropertyValue("Filename", inputFile);
    int binSize = 2;
    testAlg->setProperty("BinSize", 2);
    testAlg->setProperty("LoadAsRectImg", true);
    outputSpace = "LoadFITSx2";
    testAlg->setPropertyValue("OutputWorkspace", outputSpace);

    TS_ASSERT_THROWS_NOTHING(testAlg->execute());

    WorkspaceGroup_sptr out;
    TS_ASSERT(AnalysisDataService::Instance().doesExist(outputSpace));
    TS_ASSERT_THROWS_NOTHING(
        out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
            outputSpace));
    TS_ASSERT_EQUALS(out->getNumberOfEntries(), 2);

    for (int i = 0; i < out->getNumberOfEntries(); ++i) {
      MatrixWorkspace_sptr ws;
      TS_ASSERT_THROWS_NOTHING(
          ws = boost::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(i)));

      TS_ASSERT_EQUALS(ws->getNumberHistograms(), SPECTRA_COUNT_ASRECT / binSize);
    }

    // try 8, 512x512 => 64x64 image
    binSize = 8;
    testAlg =
      Mantid::API::AlgorithmManager::Instance().create("LoadFITS" /*, 1*/);

    TS_ASSERT_THROWS_NOTHING(testAlg->initialize());
    TS_ASSERT(testAlg->isInitialized());

    inputFile = smallFname1 + ", " + smallFname2;
    testAlg->setPropertyValue("Filename", inputFile);
    testAlg->setProperty("BinSize", binSize);
    testAlg->setProperty("LoadAsRectImg", true);
    outputSpace = "LoadFITSx8";
    testAlg->setPropertyValue("OutputWorkspace", outputSpace);

    TS_ASSERT_THROWS_NOTHING(testAlg->execute());

    TS_ASSERT(AnalysisDataService::Instance().doesExist(outputSpace));
    TS_ASSERT_THROWS_NOTHING(
        out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
            outputSpace));
    TS_ASSERT_EQUALS(out->getNumberOfEntries(), 2);

    for (int i = 0; i < out->getNumberOfEntries(); ++i) {
      MatrixWorkspace_sptr ws;
      TS_ASSERT_THROWS_NOTHING(
          ws = boost::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(i)));

      TS_ASSERT_EQUALS(ws->getNumberHistograms(), SPECTRA_COUNT_ASRECT / binSize);
    }

  }

  void test_loadAsRect() {
    testAlg =
        Mantid::API::AlgorithmManager::Instance().create("LoadFITS" /*, 1*/);

    TS_ASSERT_THROWS_NOTHING(testAlg->initialize());
    TS_ASSERT(testAlg->isInitialized());

    outputSpace = "LoadFITSRect";
    testAlg->setPropertyValue("OutputWorkspace", outputSpace);
    testAlg->setProperty("LoadAsRectImg", true);

    inputFile = smallFname1 + ", " + smallFname2;
    testAlg->setPropertyValue("Filename", inputFile);

    TS_ASSERT_THROWS_NOTHING(testAlg->execute());

    WorkspaceGroup_sptr out;
    TS_ASSERT(AnalysisDataService::Instance().doesExist(outputSpace));
    TS_ASSERT_THROWS_NOTHING(
        out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
            outputSpace));
    TS_ASSERT_EQUALS(out->getNumberOfEntries(), 2);

    for (int i = 0; i < out->getNumberOfEntries(); ++i) {
      MatrixWorkspace_sptr ws;
      TS_ASSERT_THROWS_NOTHING(
          ws = boost::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(i)));

      TS_ASSERT_EQUALS(ws->getNumberHistograms(), SPECTRA_COUNT_ASRECT);
    }
  }

private:
  Mantid::API::IAlgorithm_sptr testAlg;
  LoadFITS algToBeTested;

  std::string inputFile;
  std::string outputSpace;
  static const std::string smallFname1;
  static const std::string smallFname2;

  const static size_t xdim = 512;
  const static size_t ydim = 512;
  const static size_t SPECTRA_COUNT = xdim * ydim;
  const static size_t SPECTRA_COUNT_ASRECT = ydim;
  // FITS headers
  const static std::string hdrSIMPLE;
  const static std::string hdrBITPIX;
  const static std::string hdrNAXIS;
  const static std::string hdrNAXIS1;
  const static std::string hdrNAXIS2;
};

const std::string LoadFITSTest::smallFname1 = "FITS_small_01.fits";
const std::string LoadFITSTest::smallFname2 = "FITS_small_02.fits";

const std::string LoadFITSTest::hdrSIMPLE = "T";
const std::string LoadFITSTest::hdrBITPIX = "16";
const std::string LoadFITSTest::hdrNAXIS = "2";
const std::string LoadFITSTest::hdrNAXIS1 = "512";
const std::string LoadFITSTest::hdrNAXIS2 = "512";

#endif // MANTID_DATAHANDLING_LOADFITSTEST_H_
