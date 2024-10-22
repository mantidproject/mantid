// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadInstrumentFromRaw.h"
#include "MantidDataHandling/LoadMappingTable.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <Poco/Path.h>
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;

class LoadMappingTableTest : public CxxTest::TestSuite {
public:
  static LoadMappingTableTest *createSuite() { return new LoadMappingTableTest(); }
  static void destroySuite(LoadMappingTableTest *suite) { delete suite; }

  LoadMappingTableTest() {
    //
    inputFile = "HET15869.raw";
    outputSpace = "outerWS";
    // initialise framework manager to allow logging
    Mantid::API::FrameworkManager::Instance();
    // Create the workspace and add it to the analysis data service
    work1 = Mantid::API::WorkspaceFactory::Instance().create("Workspace2D", 24964, 1, 1);
    Mantid::API::AnalysisDataService::Instance().add(outputSpace, work1);
  }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
  }

  void testExec() {
    // Load the instrument from RAW file
    if (!load_inst.isInitialized())
      load_inst.initialize();
    load_inst.setPropertyValue("Filename", inputFile);
    load_inst.setPropertyValue("Workspace", outputSpace);
    TS_ASSERT_THROWS_NOTHING(load_inst.execute());
    //
    // Now start test specific to LoadMappingTable
    if (!loader.isInitialized())
      loader.initialize();
    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(loader.execute(), const std::runtime_error &);
    // Now set it...
    loader.setPropertyValue("Filename", inputFile);
    loader.setPropertyValue("Workspace", outputSpace);
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    // Test one to one mapping, for example spectra 6 has only 1 pixel
    TS_ASSERT_EQUALS(work1->getSpectrum(6).getDetectorIDs().size(),
                     1); // rummap.ndet(6),1);

    // Test one to many mapping, for example 10 pixels contribute to spectra
    // 2084 (workspace index 2083)
    TS_ASSERT_EQUALS(work1->getSpectrum(2083).getDetectorIDs().size(),
                     10); // map.ndet(2084),10);

    // Check the id number of all pixels contributing
    std::set<detid_t> detectorgroup;
    detectorgroup = work1->getSpectrum(2083).getDetectorIDs();
    std::set<detid_t>::const_iterator it;
    int pixnum = 101191;
    for (it = detectorgroup.begin(); it != detectorgroup.end(); ++it)
      TS_ASSERT_EQUALS(*it, pixnum++);

    AnalysisDataService::Instance().remove(outputSpace);
    return;
  }

private:
  LoadInstrumentFromRaw load_inst;
  LoadMappingTable loader;
  std::string inputFile;
  std::string outputSpace;
  MatrixWorkspace_sptr work1;
};
