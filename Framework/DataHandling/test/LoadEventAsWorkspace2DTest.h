// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileFinder.h"
#include "MantidDataHandling/LoadEventAsWorkspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

using Mantid::API::AlgorithmManager;
using Mantid::DataHandling::LoadEventAsWorkspace2D;

class LoadEventAsWorkspace2DTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadEventAsWorkspace2DTest *createSuite() { return new LoadEventAsWorkspace2DTest(); }
  static void destroySuite(LoadEventAsWorkspace2DTest *suite) { delete suite; }

  void test_EQSANS() {
    LoadEventAsWorkspace2D alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "unused"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "EQSANS_89157.nxs.h5"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("XWidth", "0.1"))
    TS_ASSERT(alg.execute());

    Mantid::DataObjects::Workspace2D_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);

    TS_ASSERT_EQUALS(outputWS->blocksize(), 1)
    TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->unitID(), "Wavelength")
    TS_ASSERT_EQUALS(outputWS->readY(18)[0], 2)
    TS_ASSERT_DELTA(outputWS->readE(18)[0], 1.4142135625, 1e-8)
    TS_ASSERT_EQUALS(outputWS->readX(18)[0], 2.375)
    TS_ASSERT_EQUALS(outputWS->readX(18)[1], 2.625)
  }

  void test_CNCS() {
    LoadEventAsWorkspace2D alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "unused"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "CNCS_7860_event.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("XCenterLog", "EnergyRequest"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("XWidth", "0.1"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Units", "Energy"))
    TS_ASSERT(alg.execute());

    Mantid::DataObjects::Workspace2D_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);

    TS_ASSERT_EQUALS(outputWS->blocksize(), 1)
    TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->unitID(), "Energy")
    // TS_ASSERT_EQUALS(outputWS->readY(0)[0], 1)
    // TS_ASSERT_EQUALS(outputWS->readE(0)[0], 1)
    // I don't agree with original test to have values of 1 in Y(0)[0] and E(0)[0]. CNCS_7860_event.nxs bank5
    // has 0 total counts this should count as faulty detector and the feature is spurious and should be excluded.
    TS_ASSERT_EQUALS(outputWS->readY(0)[0], 0)
    TS_ASSERT_EQUALS(outputWS->readE(0)[0], 0)
    TS_ASSERT_EQUALS(outputWS->readX(0)[0], 2.85)
    TS_ASSERT_EQUALS(outputWS->readX(0)[1], 3.15)
  }

  void test_CG3() {
    // compare loading with LoadEventAsWorkspace2D to LoadEventNexus+HFIRSANS2Wavelength

    const double wavelength = 6;
    const double wavelength_spread = 0.13235;

    // load with LoadEventAsWorkspace2D
    LoadEventAsWorkspace2D alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "unused"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "CG3_13118.nxs.h5"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("XCenter", wavelength))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("XWidth", wavelength_spread))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FilterByTOFMin", "-20000"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FilterByTOFMax", "20000"))
    TS_ASSERT(alg.execute());

    Mantid::DataObjects::Workspace2D_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);

    // load with LoadEventNexus then do the same as what HFIRSANS2Wavelength does.
    // HFIRSANS2Wavelength is a python algorithm so do Rebin+ScaleX instead
    auto load = AlgorithmManager::Instance().createUnmanaged("LoadEventNexus");
    load->initialize();
    load->setChild(true);
    load->setProperty("Filename", "CG3_13118.nxs.h5");
    load->execute();
    Mantid::API::Workspace_sptr outputWS2 = load->getProperty("OutputWorkspace");
    TS_ASSERT(outputWS2);

    auto rebin = AlgorithmManager::Instance().createUnmanaged("Rebin");
    rebin->initialize();
    rebin->setChild(true);
    rebin->setProperty("InputWorkspace", outputWS2);
    rebin->setProperty("Params", "-20000,40000,20000");
    rebin->setProperty("PreserveEvents", false);
    rebin->execute();

    Mantid::API::MatrixWorkspace_sptr outputWS3 = rebin->getProperty("OutputWorkspace");
    TS_ASSERT(outputWS3);

    auto scale1 = AlgorithmManager::Instance().createUnmanaged("ScaleX");
    scale1->initialize();
    scale1->setChild(true);
    scale1->setProperty("InputWorkspace", outputWS3);
    scale1->setProperty("Factor", wavelength * wavelength_spread / 40000);
    scale1->execute();
    outputWS3 = scale1->getProperty("OutputWorkspace");

    auto scale2 = AlgorithmManager::Instance().createUnmanaged("ScaleX");
    scale2->initialize();
    scale2->setChild(true);
    scale2->setProperty("InputWorkspace", outputWS3);
    scale2->setProperty("Factor", wavelength);
    scale2->setProperty("Operation", "Add");
    scale2->execute();
    outputWS3 = scale2->getProperty("OutputWorkspace");

    // set the expected X-axis
    outputWS3->getAxis(0)->setUnit("Wavelength");

    // compare workspaces
    auto compare = AlgorithmManager::Instance().createUnmanaged("CompareWorkspaces");
    compare->initialize();
    compare->setChild(true);
    compare->setProperty("Workspace1", outputWS);
    compare->setProperty("Workspace2", outputWS3);
    compare->execute();
    TS_ASSERT(compare->getProperty("Result"));
  }

  void test_BSS() {
    // compare loading with LoadEventAsWorkspace2D to LoadEventNexus+Integration

    // load with LoadEventAsWorkspace2D
    LoadEventAsWorkspace2D alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "unused"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "BSS_11841_event.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("XCenter", "1.54"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("XWidth", "0.1"))
    TS_ASSERT(alg.execute());

    Mantid::DataObjects::Workspace2D_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);

    // load with LoadEventNexus then do Integration
    auto load = AlgorithmManager::Instance().createUnmanaged("LoadEventNexus");
    load->initialize();
    load->setChild(true);
    load->setProperty("Filename", "BSS_11841_event.nxs");
    load->execute();
    Mantid::API::Workspace_sptr outputWS2 = load->getProperty("OutputWorkspace");
    TS_ASSERT(outputWS2);

    auto integrate = AlgorithmManager::Instance().createUnmanaged("Integration");
    integrate->initialize();
    integrate->setChild(true);
    integrate->setProperty("InputWorkspace", outputWS2);
    integrate->setProperty("RangeLower", 0.0);
    integrate->execute();

    Mantid::API::MatrixWorkspace_sptr outputWS3 = integrate->getProperty("OutputWorkspace");
    TS_ASSERT(outputWS3);

    // set the expected X-axis
    outputWS3->getAxis(0)->setUnit("Wavelength");
    const auto xBins = {1.463, 1.617};
    const auto histX = Mantid::Kernel::make_cow<Mantid::HistogramData::HistogramX>(xBins);
    for (size_t i = 0; i < outputWS3->getNumberHistograms(); i++)
      outputWS3->setSharedX(i, histX);

    // compare workspaces
    auto compare = AlgorithmManager::Instance().createUnmanaged("CompareWorkspaces");
    compare->initialize();
    compare->setChild(true);
    compare->setProperty("Workspace1", outputWS);
    compare->setProperty("Workspace2", outputWS3);
    compare->execute();
    TS_ASSERT(compare->getProperty("Result"));
  }

  void test_CGE_small_empty_entries() {
    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath("CG3_960.nxs.h5");
    // Run the algorithm
    LoadEventAsWorkspace2D alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "out_ws"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename))

    TS_ASSERT(alg.execute());
    Mantid::DataObjects::Workspace2D_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
  };

  void test_BSS_filterbytimeROI() {
    // compare loading with LoadEventAsWorkspace2D to LoadEventNexus+Integration by filtering 0.0 to 5.0s of data

    // load with LoadEventAsWorkspace2D
    LoadEventAsWorkspace2D alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "unused"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "BSS_11841_event.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("XCenter", "1.54"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("XWidth", "0.1"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FilterByTimeStart", "0.0"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FilterByTimeStop", "5.0"))
    TS_ASSERT(alg.execute());

    Mantid::DataObjects::Workspace2D_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);

    // load with LoadEventNexus then do Integration
    auto load = AlgorithmManager::Instance().createUnmanaged("LoadEventNexus");
    load->initialize();
    load->setChild(true);
    load->setProperty("Filename", "BSS_11841_event.nxs");
    load->setProperty("FilterByTimeStart", "0.0");
    load->setProperty("FilterByTimeStop", "5.0");
    load->execute();
    Mantid::API::Workspace_sptr outputWS2 = load->getProperty("OutputWorkspace");
    TS_ASSERT(outputWS2);

    auto integrate = AlgorithmManager::Instance().createUnmanaged("Integration");
    integrate->initialize();
    integrate->setChild(true);
    integrate->setProperty("InputWorkspace", outputWS2);
    integrate->setProperty("RangeLower", 0.0);
    integrate->execute();

    Mantid::API::MatrixWorkspace_sptr outputWS3 = integrate->getProperty("OutputWorkspace");
    TS_ASSERT(outputWS3);

    // set the expected X-axis
    outputWS3->getAxis(0)->setUnit("Wavelength");
    const auto xBins = {1.463, 1.617};
    const auto histX = Mantid::Kernel::make_cow<Mantid::HistogramData::HistogramX>(xBins);
    for (size_t i = 0; i < outputWS3->getNumberHistograms(); i++)
      outputWS3->setSharedX(i, histX);

    // compare workspaces
    auto compare = AlgorithmManager::Instance().createUnmanaged("CompareWorkspaces");
    compare->initialize();
    compare->setChild(true);
    compare->setProperty("Workspace1", outputWS);
    compare->setProperty("Workspace2", outputWS3);
    compare->execute();
    TS_ASSERT(compare->getProperty("Result"));
  }

  void test_BSS_filterbytimeStart() {
    // compare loading with LoadEventAsWorkspace2D to LoadEventNexus+Integration by filtering 0.0 to 5.0s of data

    // load with LoadEventAsWorkspace2D
    LoadEventAsWorkspace2D alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "unused"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "BSS_11841_event.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("XCenter", "1.54"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("XWidth", "0.1"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FilterByTimeStart", "5.0"))
    // TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FilterByTimeStop", "5.0"))
    TS_ASSERT(alg.execute());

    Mantid::DataObjects::Workspace2D_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);

    // load with LoadEventNexus then do Integration
    auto load = AlgorithmManager::Instance().createUnmanaged("LoadEventNexus");
    load->initialize();
    load->setChild(true);
    load->setProperty("Filename", "BSS_11841_event.nxs");
    load->setProperty("FilterByTimeStart", "5.0");
    // load->setProperty("FilterByTimeStop", "5.0");
    load->execute();
    Mantid::API::Workspace_sptr outputWS2 = load->getProperty("OutputWorkspace");
    TS_ASSERT(outputWS2);

    auto integrate = AlgorithmManager::Instance().createUnmanaged("Integration");
    integrate->initialize();
    integrate->setChild(true);
    integrate->setProperty("InputWorkspace", outputWS2);
    integrate->setProperty("RangeLower", 0.0);
    integrate->execute();

    Mantid::API::MatrixWorkspace_sptr outputWS3 = integrate->getProperty("OutputWorkspace");
    TS_ASSERT(outputWS3);

    // set the expected X-axis
    outputWS3->getAxis(0)->setUnit("Wavelength");
    const auto xBins = {1.463, 1.617};
    const auto histX = Mantid::Kernel::make_cow<Mantid::HistogramData::HistogramX>(xBins);
    for (size_t i = 0; i < outputWS3->getNumberHistograms(); i++)
      outputWS3->setSharedX(i, histX);

    // compare workspaces
    auto compare = AlgorithmManager::Instance().createUnmanaged("CompareWorkspaces");
    compare->initialize();
    compare->setChild(true);
    compare->setProperty("Workspace1", outputWS);
    compare->setProperty("Workspace2", outputWS3);
    compare->execute();
    TS_ASSERT(compare->getProperty("Result"));
  }

  void test_BSS_filterbytimeStop() {
    // compare loading with LoadEventAsWorkspace2D to LoadEventNexus+Integration by filtering 0.0 to 5.0s of data


  void test_BSS_filterbytimeROI() {
    // compare loading with LoadEventAsWorkspace2D to LoadEventNexus+Integration by filtering 0.0 to 5.0s of data

    // load with LoadEventAsWorkspace2D
    LoadEventAsWorkspace2D alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "unused"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "BSS_11841_event.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("XCenter", "1.54"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("XWidth", "0.1"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FilterByTimeStart", "0.0"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FilterByTimeStop", "5.0"))
    TS_ASSERT(alg.execute());

    Mantid::DataObjects::Workspace2D_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);

    // load with LoadEventNexus then do Integration
    auto load = AlgorithmManager::Instance().createUnmanaged("LoadEventNexus");
    load->initialize();
    load->setChild(true);
    load->setProperty("Filename", "BSS_11841_event.nxs");
    load->setProperty("FilterByTimeStart", "0.0");
    load->setProperty("FilterByTimeStop", "5.0");
    load->execute();
    Mantid::API::Workspace_sptr outputWS2 = load->getProperty("OutputWorkspace");
    TS_ASSERT(outputWS2);

    auto integrate = AlgorithmManager::Instance().createUnmanaged("Integration");
    integrate->initialize();
    integrate->setChild(true);
    integrate->setProperty("InputWorkspace", outputWS2);
    integrate->setProperty("RangeLower", 0.0);
    integrate->execute();

    Mantid::API::MatrixWorkspace_sptr outputWS3 = integrate->getProperty("OutputWorkspace");
    TS_ASSERT(outputWS3);

    // set the expected X-axis
    outputWS3->getAxis(0)->setUnit("Wavelength");
    const auto xBins = {1.463, 1.617};
    const auto histX = Mantid::Kernel::make_cow<Mantid::HistogramData::HistogramX>(xBins);
    for (size_t i = 0; i < outputWS3->getNumberHistograms(); i++)
      outputWS3->setSharedX(i, histX);

    // compare workspaces
    auto compare = AlgorithmManager::Instance().createUnmanaged("CompareWorkspaces");
    compare->initialize();
    compare->setChild(true);
    compare->setProperty("Workspace1", outputWS);
    compare->setProperty("Workspace2", outputWS3);
    compare->execute();
    TS_ASSERT(compare->getProperty("Result"));
  }

  void test_BSS_filterbytimeStart() {
    // compare loading with LoadEventAsWorkspace2D to LoadEventNexus+Integration by filtering from 5.0s of data
    // till the end of run when FiltertByTimeStop is not given.

    // load with LoadEventAsWorkspace2D
    LoadEventAsWorkspace2D alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "unused"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "BSS_11841_event.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("XCenter", "1.54"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("XWidth", "0.1"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FilterByTimeStart", "5.0"))
    // TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FilterByTimeStop", "5.0"))
    TS_ASSERT(alg.execute());

    Mantid::DataObjects::Workspace2D_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);

    // load with LoadEventNexus then do Integration
    auto load = AlgorithmManager::Instance().createUnmanaged("LoadEventNexus");
    load->initialize();
    load->setChild(true);
    load->setProperty("Filename", "BSS_11841_event.nxs");
    load->setProperty("FilterByTimeStart", "5.0");
    // load->setProperty("FilterByTimeStop", "5.0");
    load->execute();
    Mantid::API::Workspace_sptr outputWS2 = load->getProperty("OutputWorkspace");
    TS_ASSERT(outputWS2);

    auto integrate = AlgorithmManager::Instance().createUnmanaged("Integration");
    integrate->initialize();
    integrate->setChild(true);
    integrate->setProperty("InputWorkspace", outputWS2);
    integrate->setProperty("RangeLower", 0.0);
    integrate->execute();

    Mantid::API::MatrixWorkspace_sptr outputWS3 = integrate->getProperty("OutputWorkspace");
    TS_ASSERT(outputWS3);

    // set the expected X-axis
    outputWS3->getAxis(0)->setUnit("Wavelength");
    const auto xBins = {1.463, 1.617};
    const auto histX = Mantid::Kernel::make_cow<Mantid::HistogramData::HistogramX>(xBins);
    for (size_t i = 0; i < outputWS3->getNumberHistograms(); i++)
      outputWS3->setSharedX(i, histX);

    // compare workspaces
    auto compare = AlgorithmManager::Instance().createUnmanaged("CompareWorkspaces");
    compare->initialize();
    compare->setChild(true);
    compare->setProperty("Workspace1", outputWS);
    compare->setProperty("Workspace2", outputWS3);
    compare->execute();
    TS_ASSERT(compare->getProperty("Result"));
  }

  void test_BSS_filterbytimeStop() {
    // compare loading with LoadEventAsWorkspace2D to LoadEventNexus+Integration by filtering from start
    // till 7.1s of run when FiltertByTimeStart is not given.
    // load with LoadEventAsWorkspace2D
    LoadEventAsWorkspace2D alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "unused"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "BSS_11841_event.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("XCenter", "1.54"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("XWidth", "0.1"))
    // TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FilterByTimeStart", "0.0"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FilterByTimeStop", "7.1"))
    TS_ASSERT(alg.execute());

    Mantid::DataObjects::Workspace2D_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);

    // load with LoadEventNexus then do Integration
    auto load = AlgorithmManager::Instance().createUnmanaged("LoadEventNexus");
    load->initialize();
    load->setChild(true);
    load->setProperty("Filename", "BSS_11841_event.nxs");
    // load->setProperty("FilterByTimeStart", "0.0");
    load->setProperty("FilterByTimeStop", "7.1");
    load->execute();
    Mantid::API::Workspace_sptr outputWS2 = load->getProperty("OutputWorkspace");
    TS_ASSERT(outputWS2);

    auto integrate = AlgorithmManager::Instance().createUnmanaged("Integration");
    integrate->initialize();
    integrate->setChild(true);
    integrate->setProperty("InputWorkspace", outputWS2);
    integrate->setProperty("RangeLower", 0.0);
    integrate->execute();

    Mantid::API::MatrixWorkspace_sptr outputWS3 = integrate->getProperty("OutputWorkspace");
    TS_ASSERT(outputWS3);

    // set the expected X-axis
    outputWS3->getAxis(0)->setUnit("Wavelength");
    const auto xBins = {1.463, 1.617};
    const auto histX = Mantid::Kernel::make_cow<Mantid::HistogramData::HistogramX>(xBins);
    for (size_t i = 0; i < outputWS3->getNumberHistograms(); i++)
      outputWS3->setSharedX(i, histX);

    // compare workspaces
    auto compare = AlgorithmManager::Instance().createUnmanaged("CompareWorkspaces");
    compare->initialize();
    compare->setChild(true);
    compare->setProperty("Workspace1", outputWS);
    compare->setProperty("Workspace2", outputWS3);
    compare->execute();
    TS_ASSERT(compare->getProperty("Result"));
  }
};
