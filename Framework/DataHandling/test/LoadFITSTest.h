// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADFITSTEST_H_
#define MANTID_DATAHANDLING_LOADFITSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/LoadFITS.h"
#include "MantidAPI/FrameworkManager.h"

using namespace Mantid::API;
using namespace Mantid::DataHandling;

namespace {

const std::string g_smallFname1 = "FITS_small_01.fits";
const std::string g_smallFname2 = "FITS_small_02.fits";

const std::string g_emptyFileName = "FITS_empty_file.fits";

const size_t g_xdim = 512;
const size_t g_ydim = 512;
const size_t g_SPECTRA_COUNT = g_xdim * g_ydim;
const size_t g_SPECTRA_COUNT_ASRECT = g_ydim;

const std::string g_hdrSIMPLE = "T";
const std::string g_hdrBITPIX = "16";
const std::string g_hdrNAXIS = "2";
const std::string g_hdrNAXIS1 = "512";
const std::string g_hdrNAXIS2 = "512";
} // namespace

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
    TS_ASSERT_THROWS_NOTHING(lf.setPropertyValue("Filename", g_smallFname1));
    TS_ASSERT_THROWS(lf.execute(), const std::runtime_error &);
    TS_ASSERT(!lf.isExecuted());

    LoadFITS lf2;
    TS_ASSERT_THROWS_NOTHING(lf2.initialize());
    TS_ASSERT_THROWS_NOTHING(
        lf2.setPropertyValue("OutputWorkspace", "out_ws_name"));
    TS_ASSERT_THROWS(lf2.execute(), const std::runtime_error &);
    TS_ASSERT(!lf2.isExecuted());
  }

  void test_wrongProp() {
    LoadFITS lf;
    TS_ASSERT_THROWS_NOTHING(lf.initialize());
    TS_ASSERT_THROWS(lf.setPropertyValue("file", "anything"),
                     const std::runtime_error &);
    TS_ASSERT_THROWS(lf.setPropertyValue("output", "anything"),
                     const std::runtime_error &);
    TS_ASSERT_THROWS(lf.setPropertyValue("FITS", "anything"),
                     const std::runtime_error &);

    TS_ASSERT_THROWS(lf.setPropertyValue("BinSize", "-1"),
                     const std::invalid_argument &);
    TS_ASSERT_THROWS(lf.setPropertyValue("BinSize", "0"),
                     const std::invalid_argument &);
    TS_ASSERT_THROWS(lf.setPropertyValue("FilterNoiseLevel", "-10"),
                     const std::invalid_argument &);
    TS_ASSERT_THROWS_NOTHING(lf.setPropertyValue("FilterNoiseLevel", "0"));

    TS_ASSERT_THROWS(lf.setPropertyValue("ImageKey", "anything"),
                     const Mantid::Kernel::Exception::NotFoundError &);
    TS_ASSERT_THROWS(lf.setPropertyValue("BITPIX", "anything"),
                     const std::runtime_error &);
    TS_ASSERT_THROWS(lf.setPropertyValue("NAXIS", "anything"),
                     const std::runtime_error &);
    TS_ASSERT_THROWS(lf.setPropertyValue("NAXIS1", "anything"),
                     const std::runtime_error &);
  }

  void test_initGood() {
    TS_ASSERT_THROWS_NOTHING(algToBeTested.initialize());
    TS_ASSERT(algToBeTested.isInitialized());

    if (!algToBeTested.isInitialized())
      algToBeTested.initialize();

    outputSpace = "LoadFITSTest";
    algToBeTested.setPropertyValue("OutputWorkspace", outputSpace);

    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(algToBeTested.execute(), const std::runtime_error &);

    inputFile = g_smallFname1 + ", " + g_smallFname2;
    algToBeTested.setPropertyValue("Filename", inputFile);

    // Set the ImageKey to be 0 (this used to be required, but the key
    // should not be there any longer);
    TS_ASSERT_THROWS(algToBeTested.setProperty<int>("ImageKey", 0),
                     const Mantid::Kernel::Exception::NotFoundError &);
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
    TS_ASSERT_EQUALS(run.getLogData("SIMPLE")->value(), g_hdrSIMPLE);
    TS_ASSERT_EQUALS(run.getLogData("BITPIX")->value(), g_hdrBITPIX);
    TS_ASSERT_EQUALS(run.getLogData("NAXIS")->value(), g_hdrNAXIS);
    TS_ASSERT_EQUALS(run.getLogData("NAXIS1")->value(), g_hdrNAXIS1);
    TS_ASSERT_EQUALS(run.getLogData("NAXIS2")->value(), g_hdrNAXIS2);

    // Number of spectra
    TS_ASSERT_EQUALS(ws1->getNumberHistograms(), g_SPECTRA_COUNT);
    TS_ASSERT_EQUALS(ws2->getNumberHistograms(), g_SPECTRA_COUNT);

    // Sum the two bins from the last spectra - should be 70400
    double sumY =
        ws1->y(g_SPECTRA_COUNT - 1)[0] + ws2->y(g_SPECTRA_COUNT - 1)[0];
    TS_ASSERT_EQUALS(sumY, 275);
    // Check the sum of the error values for the last spectra in each file -
    // should be 375.183
    double sumE =
        ws1->e(g_SPECTRA_COUNT - 1)[0] + ws2->e(g_SPECTRA_COUNT - 1)[0];
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

    inputFile = g_smallFname1 + ", " + g_smallFname2;
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

      TS_ASSERT_EQUALS(ws->getNumberHistograms(), g_SPECTRA_COUNT);

      // check Y and Error
      TS_ASSERT_EQUALS(ws->y(g_SPECTRA_COUNT - 100)[0], expectedY[i]);
      TS_ASSERT_LESS_THAN(
          std::abs(ws->e(g_SPECTRA_COUNT - 100)[0] - expectedE[i]), 0.0001);
    }
  }

  void test_rebinWrong() {
    testAlg =
        Mantid::API::AlgorithmManager::Instance().create("LoadFITS" /*, 1*/);

    TS_ASSERT_THROWS_NOTHING(testAlg->initialize());
    TS_ASSERT(testAlg->isInitialized());

    inputFile = g_smallFname1 + ", " + g_smallFname2;
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

    inputFile = g_smallFname1 + ", " + g_smallFname2;
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

      TS_ASSERT_EQUALS(ws->getNumberHistograms(),
                       g_SPECTRA_COUNT_ASRECT / binSize);
    }

    // try 8, 512x512 => 64x64 image
    binSize = 8;
    testAlg =
        Mantid::API::AlgorithmManager::Instance().create("LoadFITS" /*, 1*/);

    TS_ASSERT_THROWS_NOTHING(testAlg->initialize());
    TS_ASSERT(testAlg->isInitialized());

    inputFile = g_smallFname1 + ", " + g_smallFname2;
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

      TS_ASSERT_EQUALS(ws->getNumberHistograms(),
                       g_SPECTRA_COUNT_ASRECT / binSize);
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

    inputFile = g_smallFname1 + ", " + g_smallFname2;
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

      TSM_ASSERT_EQUALS("The number of histograms should be the expected, "
                        "dimension of the image",
                        ws->getNumberHistograms(), g_SPECTRA_COUNT_ASRECT);
    }

    TSM_ASSERT_EQUALS("The output workspace group should have two workspaces",
                      out->size(), 2);

    // and finally a basic check of values in the image, to be safe
    MatrixWorkspace_sptr ws0;
    TS_ASSERT_THROWS_NOTHING(
        ws0 = boost::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(0)));

    TSM_ASSERT_EQUALS("The title of the first output workspace is not the name "
                      "of the first file",
                      ws0->getTitle(), g_smallFname1);

    size_t n = ws0->getNumberHistograms();
    TSM_ASSERT_EQUALS(
        "The value at a given spectrum and bin (first one) is not as expected",
        ws0->y(n - 1)[0], 137);

    TSM_ASSERT_EQUALS(
        "The value at a given spectrum and bin (middle one) is not as expected",
        ws0->y(n - 1)[g_SPECTRA_COUNT_ASRECT / 2], 159);

    TSM_ASSERT_EQUALS(
        "The value at a given spectrum and bin (last one) is not as expected",
        ws0->y(n - 1).back(), 142);

    MatrixWorkspace_sptr ws1;
    TS_ASSERT_THROWS_NOTHING(
        ws1 = boost::dynamic_pointer_cast<MatrixWorkspace>(out->getItem(1)));

    TSM_ASSERT_EQUALS("The title of the second output workspace is not the "
                      "name of the second file",
                      ws1->getTitle(), g_smallFname2);
    TSM_ASSERT_EQUALS(
        "The value at a given spectrum and bin (first one) is not as expected",
        ws1->y(n - 1)[0], 155);

    TSM_ASSERT_EQUALS(
        "The value at a given spectrum and bin (middle one) is not as expected",
        ws1->y(n - 1)[g_SPECTRA_COUNT_ASRECT / 2], 199);

    TSM_ASSERT_EQUALS(
        "The value at a given spectrum and bin (last one) is not as expected",
        ws1->y(n - 1).back(), 133);
  }

  void test_loadEmpty() {
    testAlg =
        Mantid::API::AlgorithmManager::Instance().create("LoadFITS" /*, 1*/);

    TS_ASSERT_THROWS_NOTHING(testAlg->initialize());
    TS_ASSERT(testAlg->isInitialized());

    outputSpace = "I_should_not_load_correctly";
    testAlg->setPropertyValue("OutputWorkspace", outputSpace);
    testAlg->setProperty("LoadAsRectImg", true);

    testAlg->setPropertyValue("Filename", g_emptyFileName);

    TS_ASSERT_THROWS_NOTHING(testAlg->execute());
    TS_ASSERT(!AnalysisDataService::Instance().doesExist(outputSpace));
  }

private:
  Mantid::API::IAlgorithm_sptr testAlg;
  LoadFITS algToBeTested;

  std::string inputFile;
  std::string outputSpace;
};

class LoadFITSTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadFITSTestPerformance *createSuite() {
    return new LoadFITSTestPerformance();
  }
  static void destroySuite(LoadFITSTestPerformance *suite) { delete suite; }

  void setUp() override { FrameworkManager::Instance(); }

  void tearDown() override {
    Mantid::API::AnalysisDataService::Instance().remove("FitsOutput");
  }

  void test_Load_Small_01() {
    LoadFITS lf;
    lf.initialize();
    lf.setProperty("Filename", g_smallFname1);
    lf.setPropertyValue("OutputWorkspace", "FitsOutput");
    lf.setProperty("LoadAsRectImg", false);
    TS_ASSERT(lf.execute());
  }
  void test_LoadAsRectImage_Small_01() {
    LoadFITS lf;
    lf.initialize();
    lf.setProperty("Filename", g_smallFname1);
    lf.setPropertyValue("OutputWorkspace", "FitsOutput");
    lf.setProperty("LoadAsRectImg", true);
    TS_ASSERT(lf.execute());
  }

  void test_Load_Small_02() {
    LoadFITS lf;
    lf.initialize();
    lf.setProperty("Filename", g_smallFname2);
    lf.setPropertyValue("OutputWorkspace", "FitsOutput");
    lf.setProperty("LoadAsRectImg", false);
    TS_ASSERT(lf.execute());
  }
  void test_LoadAsRectImage_Small_02() {
    LoadFITS lf;
    lf.initialize();
    lf.setProperty("Filename", g_smallFname2);
    lf.setPropertyValue("OutputWorkspace", "FitsOutput");
    lf.setProperty("LoadAsRectImg", true);
    TS_ASSERT(lf.execute());
  }
};

#endif // MANTID_DATAHANDLING_LOADFITSTEST_H_
