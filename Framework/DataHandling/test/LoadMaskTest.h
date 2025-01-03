// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <sstream>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/InstrumentFileFinder.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataHandling/LoadMask.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidFrameworkTestHelpers/ScopedFileHelper.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

namespace {
const std::string VULCAN_INSTR("VULCAN_Definition_2019-06-20.xml");
const std::string POWGEN_INSTR("POWGEN");
} // namespace

class LoadMaskTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadMaskTest *createSuite() { return new LoadMaskTest(); }
  static void destroySuite(LoadMaskTest *suite) { delete suite; }

  void test_LoadXML() {
    LoadMask loadfile;
    loadfile.initialize();

    loadfile.setProperty("Instrument", POWGEN_INSTR);
    loadfile.setProperty("InputFile", "testmasking.xml");
    loadfile.setProperty("OutputWorkspace", "PG3Mask");

    try {
      TS_ASSERT_EQUALS(loadfile.execute(), true);
      DataObjects::MaskWorkspace_sptr maskws =
          AnalysisDataService::Instance().retrieveWS<DataObjects::MaskWorkspace>("PG3Mask");
    } catch (std::runtime_error &e) {
      TS_FAIL(e.what());
    }
  } // test_LoadXML

  /*
   * By given a non-existing instrument's name, exception should be thrown.
   */
  void test_LoadXMLThrow() {
    LoadMask loadfile;
    loadfile.initialize();

    loadfile.setProperty("Instrument", "WhatEver");
    loadfile.setProperty("InputFile", "testmasking.xml");
    loadfile.setProperty("OutputWorkspace", "PG3Mask");

    try {
      TS_ASSERT_EQUALS(loadfile.execute(), false);
    } catch (std::runtime_error &e) {
      TS_FAIL(e.what());
    }

  } // test_LoadXML

  /*
   * Test mask by detector ID
   * For VULCAN:
   * workspaceindex:  detector ID
   * 34           :   26284
   * 1000         :   27250
   * 2000         :   28268
   */
  void test_DetectorIDs() {
    // 1. Generate masking files
    std::vector<int> banks1;
    std::vector<int> detids;
    detids.emplace_back(26284);
    detids.emplace_back(27250);
    detids.emplace_back(28268);
    auto maskDetFile = genMaskingFile("maskingdet.xml", detids, banks1);

    // 2. Run
    LoadMask loadfile;
    loadfile.initialize();

    loadfile.setProperty("Instrument", VULCAN_INSTR);
    loadfile.setProperty("InputFile", maskDetFile.getFileName());
    loadfile.setProperty("OutputWorkspace", "VULCAN_Mask_Detectors");

    TS_ASSERT_EQUALS(loadfile.execute(), true);
    DataObjects::MaskWorkspace_sptr maskws =
        AnalysisDataService::Instance().retrieveWS<DataObjects::MaskWorkspace>("VULCAN_Mask_Detectors");

    // 3. Check
    for (size_t iws = 0; iws < 6468; iws++) {
      double y = maskws->y(iws)[0];
      if (iws == 34 || iws == 1000 || iws == 1846) {
        // These 3 workspace index are masked
        TS_ASSERT_DELTA(y, 1.0, 1.0E-5);
      } else {
        // Unmasked
        TS_ASSERT_DELTA(y, 0.0, 1.0E-5);
      }
    }
  }

  void test_DetectorIDs_reuse_LoadMask_instance() {
    // 1. Generate masking files
    std::vector<int> banks1;
    std::vector<int> detids{26284, 27250};
    auto maskDetFile1 = genMaskingFile("maskingdet1.xml", detids, banks1);
    detids = {28268};
    auto maskDetFile2 = genMaskingFile("maskingdet2.xml", detids, banks1);

    // 2. Run
    LoadMask loadfile;
    loadfile.initialize();

    loadfile.setProperty("Instrument", VULCAN_INSTR);
    loadfile.setProperty("InputFile", maskDetFile1.getFileName());
    loadfile.setProperty("OutputWorkspace", "VULCAN_Mask_Detectors");

    TS_ASSERT_EQUALS(loadfile.execute(), true);
    auto maskws = AnalysisDataService::Instance().retrieveWS<DataObjects::MaskWorkspace>("VULCAN_Mask_Detectors");

    // 3. Check
    for (size_t iws = 0; iws < maskws->getNumberHistograms(); iws++) {
      double y = maskws->y(iws)[0];
      if (iws == 34 || iws == 1000) {
        // These 2 workspace index are masked
        TS_ASSERT_DELTA(y, 1.0, 1.0E-5);
      } else {
        // Unmasked
        TS_ASSERT_DELTA(y, 0.0, 1.0E-5);
      }
    }

    loadfile.setProperty("Instrument", VULCAN_INSTR);
    loadfile.setProperty("InputFile", maskDetFile2.getFileName());
    loadfile.setProperty("OutputWorkspace", "VULCAN_Mask_Detectors");

    TS_ASSERT_EQUALS(loadfile.execute(), true);
    maskws = AnalysisDataService::Instance().retrieveWS<DataObjects::MaskWorkspace>("VULCAN_Mask_Detectors");

    // 3. Check
    for (size_t iws = 0; iws < maskws->getNumberHistograms(); iws++) {
      double y = maskws->y(iws)[0];
      if (iws == 1846) {
        // This 1 workspace index is masked
        TS_ASSERT_DELTA(y, 1.0, 1.0E-5);
      } else {
        // Unmasked
        TS_ASSERT_DELTA(y, 0.0, 1.0E-5);
      }
    }
  }

  /*
   * Test mask by detector ID
   * For VULCAN:
   * workspaceindex:  detector ID  :  Spectrum No
   * 34           :   26284        :  35
   * 1000         :   27250        :  1001
   * 2000         :   28268        :  2001
   * 36-39                            37-40
   * 1001-1004                        1002-1005
   */
  void test_ISISFormat() {
    // 1. Generate masking files
    std::vector<specnum_t> singlespectra;
    singlespectra.emplace_back(35);
    singlespectra.emplace_back(1001);
    singlespectra.emplace_back(2001);
    std::vector<specnum_t> pairspectra;
    pairspectra.emplace_back(1002);
    pairspectra.emplace_back(1005);
    pairspectra.emplace_back(37);
    pairspectra.emplace_back(40);

    auto isisMaskFile = genISISMaskingFile("isismask.msk", singlespectra, pairspectra);

    // 2. Run
    LoadMask loadfile;
    loadfile.initialize();

    loadfile.setProperty("Instrument", VULCAN_INSTR);
    loadfile.setProperty("InputFile", isisMaskFile.getFileName());
    loadfile.setProperty("OutputWorkspace", "VULCAN_Mask_Detectors");

    TS_ASSERT_EQUALS(loadfile.execute(), true);
    DataObjects::MaskWorkspace_sptr maskws =
        AnalysisDataService::Instance().retrieveWS<DataObjects::MaskWorkspace>("VULCAN_Mask_Detectors");

    // 3. Check
    size_t errorcounts = 0;
    for (size_t iws = 0; iws < maskws->getNumberHistograms(); iws++) {
      double y = maskws->y(iws)[0];
      if (iws == 34 || iws == 1000 || iws == 2000 || (iws >= 36 && iws <= 39) || (iws >= 1001 && iws <= 1004)) {
        // All these workspace index are masked
        TS_ASSERT_DELTA(y, 1.0, 1.0E-5);
      } else {
        // Unmasked
        TS_ASSERT_DELTA(y, 0.0, 1.0E-5);
        if (fabs(y) > 1.0E-5) {
          errorcounts++;
          std::cout << "Workspace Index " << iws << " has a wrong set on masks\n";
        }
      }
    }
    std::cout << "Total " << errorcounts << " errors \n";
  }
  /*Calculate werid spectra number as function of index */
  size_t calc_spec_num(size_t index) {
    if ((index + 3) % 3 == 0) {
      return index + 3;
    }
    if ((index + 2) % 3 == 0) {
      return index + 1;
    }
    if ((index + 1) % 3 == 0) {
      return index - 1;
    }
    // suppress warinings
    return index;
  }

  void test_ISISWithRefWS() {
    auto ws_creator = AlgorithmManager::Instance().createUnmanaged("CreateSimulationWorkspace");
    ws_creator->initialize();
    ws_creator->setChild(true);

    ws_creator->setPropertyValue("Instrument", "MARI");
    ws_creator->setPropertyValue("BinParams", "100,100,300");
    ws_creator->setPropertyValue("OutputWorkspace", "testWS");
    ws_creator->setPropertyValue("UnitX", "TOF");

    ws_creator->execute();
    MatrixWorkspace_sptr source = ws_creator->getProperty("OutputWorkspace");
    TS_ASSERT(source);

    // modify spectra-detector map on the sample workspace to check masking
    std::vector<detid_t> detIDs = source->getInstrument()->getDetectorIDs(true);
    size_t index = 0;
    for (auto it = detIDs.rbegin(); it != detIDs.rend(); ++it) {
      const detid_t detId = *it;
      auto &spec = source->getSpectrum(index);
      Mantid::specnum_t specNo = static_cast<Mantid::specnum_t>(calc_spec_num(index));
      spec.setSpectrumNo(specNo);
      spec.setDetectorID(detId);

      index++;
    }

    auto masker = AlgorithmManager::Instance().create("MaskDetectors");
    masker->initialize();
    masker->setChild(true);
    masker->setProperty("Workspace", source);
    std::vector<int> masked_spectra(11);
    masked_spectra[0] = 10;
    masked_spectra[1] = 11;
    masked_spectra[2] = 12;
    masked_spectra[3] = 100;
    masked_spectra[4] = 110;
    masked_spectra[5] = 120;
    masked_spectra[6] = 130;
    masked_spectra[7] = 140;
    masked_spectra[8] = 200;
    masked_spectra[9] = 300;
    masked_spectra[10] = 4;
    masker->setProperty("SpectraList", masked_spectra);
    masker->execute();
    Workspace_sptr tsource = masker->getProperty("Workspace");
    source = std::dynamic_pointer_cast<MatrixWorkspace>(tsource);
    TS_ASSERT(source);

    /* This is proper way of extracting mask this but does not work from
    subproject (yet)
     and you have to delete target file manually
    auto exporter =
        AlgorithmManager::Instance().create("ExportSpectraMask");
    exporter->initialize();
    exporter->setProperty("Workspace", source);
    exporter->execute();
    */
    /*Fake export mask algorithm: */
    std::string mask_contents("4 10-12 100 110 120 130 140 200 300");
    ScopedFileHelper::ScopedFile testFile(mask_contents, "test_mask_file.msk");

    // 2. Run
    LoadMask loadMask;
    loadMask.initialize();
    loadMask.setChild(true);

    loadMask.setProperty("Instrument", "MARI");
    loadMask.setProperty("RefWorkspace", source);
    loadMask.setProperty("InputFile", testFile.getFileName());
    loadMask.setProperty("OutputWorkspace", "MaskedWithSample");

    TS_ASSERT_EQUALS(loadMask.execute(), true);

    DataObjects::MaskWorkspace_sptr maskWs = loadMask.getProperty("OutputWorkspace");

    // check that mask ws contains different spectra but the same detectors
    // masked
    std::vector<detid_t> maskSourceDet, maskTargDet;

    const auto &spectrumInfoSource = source->spectrumInfo();
    const auto &spectrumInfoTarget = maskWs->spectrumInfo();
    size_t n_steps = source->getNumberHistograms();
    for (size_t i = 0; i < n_steps; ++i) {
      bool source_masked = spectrumInfoSource.isMasked(i);
      if (source_masked) {
        const auto &detector = spectrumInfoSource.detector(i);
        const auto detectorId = detector.getID();
        maskSourceDet.emplace_back(detectorId);
      }
      bool targ_masked = (maskWs->getSpectrum(i).y()[0] > 0.5);
      if (targ_masked) {
        const auto &detector = spectrumInfoTarget.detector(i);
        const auto detectorId = detector.getID();
        maskTargDet.emplace_back(detectorId);
      }
    }
    std::sort(maskSourceDet.begin(), maskSourceDet.end());
    std::sort(maskTargDet.begin(), maskTargDet.end());

    TS_ASSERT_EQUALS(maskSourceDet.size(), maskTargDet.size());
    for (size_t i = 0; i < maskSourceDet.size(); i++) {
      TS_ASSERT_EQUALS(maskSourceDet[i], maskTargDet[i]);
    }
  }

  void test_IDF_acceptedAsFileName() {
    auto ws_creator = AlgorithmManager::Instance().createUnmanaged("CreateSimulationWorkspace");
    ws_creator->initialize();
    ws_creator->setChild(true);

    ws_creator->setPropertyValue("Instrument", "MARI");
    ws_creator->setPropertyValue("BinParams", "100,100,300");
    ws_creator->setPropertyValue("OutputWorkspace", "testWS");
    ws_creator->setPropertyValue("UnitX", "TOF");

    ws_creator->execute();
    MatrixWorkspace_sptr source = ws_creator->getProperty("OutputWorkspace");
    TS_ASSERT(source);

    std::string IDF_name = API::InstrumentFileFinder::getInstrumentFilename("MARI", "");

    /*Fake export mask algorithm: */
    std::string mask_contents("4 10-12 100 110 120 130 140 200 300");
    ScopedFileHelper::ScopedFile testFile(mask_contents, "test_mask_file.msk");

    // 2. Run
    LoadMask loadMask;
    loadMask.initialize();
    loadMask.setChild(true);

    loadMask.setProperty("Instrument", IDF_name);
    loadMask.setProperty("RefWorkspace", source);
    loadMask.setProperty("InputFile", testFile.getFileName());
    loadMask.setProperty("OutputWorkspace", "MaskedWithSample");

    TS_ASSERT_EQUALS(loadMask.execute(), true);

    DataObjects::MaskWorkspace_sptr maskWs = loadMask.getProperty("OutputWorkspace");
    TS_ASSERT(maskWs);
  }

  /*
   * Load "testingmasking.xml" and "regionofinterest.xml"
   * These two xml files will generate two opposite Workspaces, i.e.,
   * Number(masked detectors of WS1) = Number(unmasked detectors of WS2)
   *
   * by BinaryOperation
   */
  void test_Banks() {
    // 0. Generate masking files
    std::vector<int> banks1;
    banks1.emplace_back(21);
    banks1.emplace_back(22);
    banks1.emplace_back(2200);
    std::vector<int> detids;
    auto maskFile1 = genMaskingFile("masking01.xml", detids, banks1);

    std::vector<int> banks2;
    banks2.emplace_back(23);
    banks2.emplace_back(26);
    banks2.emplace_back(27);
    banks2.emplace_back(28);
    auto maskFile2 = genMaskingFile("masking02.xml", detids, banks2);

    // 1. Generate Mask Workspace
    LoadMask loadfile;
    loadfile.initialize();

    loadfile.setProperty("Instrument", VULCAN_INSTR);
    loadfile.setProperty("InputFile", maskFile1.getFileName());
    loadfile.setProperty("OutputWorkspace", "VULCAN_Mask1");

    TS_ASSERT_EQUALS(loadfile.execute(), true);
    DataObjects::MaskWorkspace_sptr maskws =
        AnalysisDataService::Instance().retrieveWS<DataObjects::MaskWorkspace>("VULCAN_Mask1");

    // 2. Generate Region of Interest Workspace
    LoadMask loadfile2;
    loadfile2.initialize();

    loadfile2.setProperty("Instrument", VULCAN_INSTR);
    loadfile2.setProperty("InputFile", maskFile2.getFileName());
    loadfile2.setProperty("OutputWorkspace", "VULCAN_Mask2");

    TS_ASSERT_EQUALS(loadfile2.execute(), true);
    DataObjects::MaskWorkspace_sptr interestws =
        AnalysisDataService::Instance().retrieveWS<DataObjects::MaskWorkspace>("VULCAN_Mask2");

    // 3. Check
    size_t sizemask = maskws->getNumberHistograms();
    size_t sizeinterest = interestws->getNumberHistograms();
    TS_ASSERT(sizemask == sizeinterest);

    if (sizemask == sizeinterest) {
      // number1: number of masked detectors of maskws
      // number2: number of used detectors of interestws
      size_t number1 = 0;
      size_t number0 = 0;
      for (size_t ih = 0; ih < 6468; ih++) { // pre-VulcanX east and west bank
        bool v1 = maskws->isMaskedIndex(ih);
        bool v2 = interestws->isMaskedIndex(ih);
        if (v1) {
          number0++;
          TS_ASSERT(!v2);
        }
        if (!v2) {
          number1++;
          TS_ASSERT(v1);
        }
      }

      TS_ASSERT_EQUALS(number0 > 0, true);
      TS_ASSERT_EQUALS(number1 > 0, true);
      TS_ASSERT_EQUALS(number1 - number0, 0);
    }

    return;
  } // test_Openfile

  void testSpecifyInstByIDF() {
    const std::string oldEmuIdf = "EMU_Definition_32detectors.xml";
    const std::string newEmuIdf = "EMU_Definition_96detectors.xml";

    const std::vector<int> detIDs = {2, 10};
    auto maskFile = genMaskingFile("emu_mask.xml", detIDs, std::vector<int>());

    auto byInstName = loadMask("EMU", maskFile.getFileName(), "LoadedByInstName");
    auto withOldIDF = loadMask(oldEmuIdf, maskFile.getFileName(), "LoadWithOldIDF");
    auto withNewIDF = loadMask(newEmuIdf, maskFile.getFileName(), "LoadWithNewIDF");

    TS_ASSERT_EQUALS(byInstName->getNumberHistograms(), 96);
    TS_ASSERT_EQUALS(withOldIDF->getNumberHistograms(), 32);
    TS_ASSERT_EQUALS(withNewIDF->getNumberHistograms(), 96);

    TS_ASSERT(byInstName->isMasked(2));
    TS_ASSERT(withOldIDF->isMasked(2));
    TS_ASSERT(withNewIDF->isMasked(2));
    TS_ASSERT(byInstName->isMasked(10));
    TS_ASSERT(withOldIDF->isMasked(10));
    TS_ASSERT(withNewIDF->isMasked(10));

    TS_ASSERT_EQUALS(byInstName->getNumberMasked(), 2);
    TS_ASSERT_EQUALS(withOldIDF->getNumberMasked(), 2);
    TS_ASSERT_EQUALS(withNewIDF->getNumberMasked(), 2);
  }

  DataObjects::MaskWorkspace_sptr loadMask(const std::string &instrument, const std::string &inputFile,
                                           const std::string &outputWsName) {
    LoadMask loadMask;
    loadMask.initialize();

    loadMask.setProperty("Instrument", instrument);
    loadMask.setProperty("InputFile", inputFile);
    loadMask.setProperty("OutputWorkspace", outputWsName);

    TS_ASSERT_EQUALS(loadMask.execute(), true);

    return AnalysisDataService::Instance().retrieveWS<DataObjects::MaskWorkspace>(outputWsName);
  }

  /*
   * Create a masking file
   */
  ScopedFileHelper::ScopedFile genMaskingFile(const std::string &maskfilename, std::vector<int> detids,
                                              const std::vector<int> &banks) {
    std::stringstream ss;

    // 1. Header
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    ss << "  <detector-masking>\n";
    ss << "    <group>\n";

    // 2. "detids" & component
    if (detids.size() > 0) {
      ss << "    <detids>";
      for (size_t i = 0; i < detids.size(); i++) {
        if (i < detids.size() - 1)
          ss << detids[i] << ",";
        else
          ss << detids[i];
      }
      ss << "</detids>\n";
    }

    for (int bank : banks) {
      ss << "<component>bank" << bank << "</component>\n";
    }

    // 4. End of file
    ss << "  </group>\n</detector-masking>\n";

    return ScopedFileHelper::ScopedFile(ss.str(), maskfilename);
  }

  /*
   * Create an ISIS format masking file
   */
  ScopedFileHelper::ScopedFile genISISMaskingFile(const std::string &maskfilename,
                                                  const std::vector<specnum_t> &singlespectra,
                                                  std::vector<specnum_t> pairspectra) {
    std::stringstream ss;

    // 1. Single spectra
    for (int i : singlespectra) {
      ss << i << " ";
    }
    ss << '\n';

    // 2. Spectra pair
    // a) Make the list really has complete pairs
    if (pairspectra.size() % 2 == 1)
      pairspectra.pop_back();

    for (size_t i = 0; i < pairspectra.size(); i += 2) {
      ss << pairspectra[i] << "-" << pairspectra[i + 1] << "  ";
    }
    ss << '\n';

    return ScopedFileHelper::ScopedFile(ss.str(), maskfilename);
  }
};

//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class LoadMaskTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadMaskTestPerformance *createSuite() { return new LoadMaskTestPerformance(); }

  static void destroySuite(LoadMaskTestPerformance *suite) { delete suite; }

  void setUp() override {
    loadFile.initialize();
    loadFile.setProperty("Instrument", POWGEN_INSTR);
    loadFile.setProperty("InputFile", "testmasking.xml");
    loadFile.setProperty("OutputWorkspace", "outputWS");
  }

  void tearDown() override { AnalysisDataService::Instance().remove("outputWS"); }

  void testDefaultLoad() { loadFile.execute(); }

private:
  LoadMask loadFile;
};
