// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidMuon/LoadMuonNexus3.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using Mantid::detid_t;
using Mantid::Types::Core::DateAndTime;

class LoadMuonNexus3Test : public CxxTest::TestSuite {
public:
  void check_spectra_and_detectors(const MatrixWorkspace_sptr &output) {

    //----------------------------------------------------------------------
    // Tests to check that spectra-detector mapping is done correctly
    //----------------------------------------------------------------------
    // Check the total number of elements in the map for HET
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 192);

    // Test one to one mapping, for example spectra 6 has only 1 pixel
    TS_ASSERT_EQUALS(output->getSpectrum(6).getDetectorIDs().size(), 1);

    auto detectorgroup = output->getSpectrum(99).getDetectorIDs();
    TS_ASSERT_EQUALS(detectorgroup.size(), 1);
    TS_ASSERT_EQUALS(*detectorgroup.begin(), 100);
  }

  void testExecLoadMuonNexus2() {
    LoadMuonNexus3 nxLoad;
    nxLoad.initialize();

    // Now set required filename and output workspace name
    std::string inputFile = "argus0026287.nxs";
    nxLoad.setPropertyValue("FileName", inputFile);

    std::string outputSpace = "outer";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);

    // Test execute to read file and populate workspace
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());
    TS_ASSERT(nxLoad.isExecuted());

    // Check output workspace
    MatrixWorkspace_sptr output;
    output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace);
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);

    // Perform limited tests on the outwork workspace as this is essentially just a wrapper algorithm.
    // subset of tests performed in LoadMuonNexus2Test
    check_spectra_and_detectors(output);

    TS_ASSERT(nxLoad.getSelectedAlg() == "LoadMuonNexus");
    TS_ASSERT(nxLoad.getSelectedVersion() == 2);
  }

  void testExecLoadMuonNexus1() {
    LoadMuonNexus3 nxLoad;
    nxLoad.initialize();

    // Now set required filename and output workspace name
    std::string inputFile = "emu00006475.nxs";
    nxLoad.setPropertyValue("FileName", inputFile);

    std::string outputSpace = "outer";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);

    // Test execute to read file and populate workspace
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());
    TS_ASSERT(nxLoad.isExecuted());

    // Check output workspace group
    Mantid::API::WorkspaceGroup_sptr output = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outputSpace);
    TS_ASSERT(output->size() == 4);

    TS_ASSERT(nxLoad.getSelectedAlg() == "LoadMuonNexus");
    TS_ASSERT(nxLoad.getSelectedVersion() == 1);
  }

  void testExecLoadMuonNexusV2() {
    LoadMuonNexus3 nxLoad;
    nxLoad.initialize();

    // Now set required filename and output workspace name
    std::string inputFile = "ARGUS00073601.nxs";
    nxLoad.setPropertyValue("FileName", inputFile);

    std::string outputSpace = "outer";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);

    // Test execute to read file and populate workspace
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());
    TS_ASSERT(nxLoad.isExecuted());

    // Check output workspace group
    MatrixWorkspace_sptr output;
    output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace);
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);

    // Perform limited tests on the outwork workspace as this is essentially just a wrapper algorithm.
    // subset of tests performed in LoadMuonNexus2Test
    check_spectra_and_detectors(output);

    TS_ASSERT(nxLoad.getSelectedAlg() == "LoadMuonNexusV2");
    TS_ASSERT(nxLoad.getSelectedVersion() == 1);
  }
};
