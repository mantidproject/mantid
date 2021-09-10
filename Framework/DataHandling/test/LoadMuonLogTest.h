// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

// These includes seem to make the difference between initialization of the
// workspace names (workspace2D/1D etc), instrument classes and not for this
// test case.
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadMuonLog.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <Poco/Path.h>
#include <vector>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using Mantid::DataHandling::LoadMuonLog;

class LoadMuonLogTest : public CxxTest::TestSuite {
public:
  void testInit() {
    TS_ASSERT(!loader.isInitialized());
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
  }

  void testExecWithNexusDatafile() {
    // if ( !loader.isInitialized() ) loader.initialize();

    LoadMuonLog loaderNexusFile;
    loaderNexusFile.initialize();

    // Path to test input file assumes Test directory checked out from SVN
    inputFile = "emu00006473.nxs";
    loaderNexusFile.setPropertyValue("Filename", inputFile);

    outputSpace = "LoadMuonLogTest-nexusdatafile";
    TS_ASSERT_THROWS(loaderNexusFile.setPropertyValue("Workspace", outputSpace), const std::invalid_argument &)
    // Create an empty workspace and put it in the AnalysisDataService
    MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);

    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(outputSpace, ws));

    loaderNexusFile.setChild(true);
    loaderNexusFile.execute();

    TS_ASSERT(loaderNexusFile.isExecuted());

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace));

    // std::shared_ptr<Sample> sample = output->getSample();

    // obtain the expected log data which was read from the Nexus file (NXlog)

    Property *l_property = output->run().getLogData(std::string("BEAMLOG_CURRENT"));
    TimeSeriesProperty<double> *l_timeSeriesDouble1 = dynamic_cast<TimeSeriesProperty<double> *>(l_property);
    std::string timeSeriesString = l_timeSeriesDouble1->value();
    TS_ASSERT_EQUALS(timeSeriesString.substr(0, 27), "2006-Nov-21 07:03:08  182.8");

    l_property = output->run().getLogData(std::string("BEAMLOG_FREQ"));
    TimeSeriesProperty<double> *l_timeSeriesDouble = dynamic_cast<TimeSeriesProperty<double> *>(l_property);
    timeSeriesString = l_timeSeriesDouble->value();
    TS_ASSERT_EQUALS(timeSeriesString.substr(0, 24), "2006-Nov-21 07:03:08  50");

    l_property = output->run().getLogData(std::string("Temp_Cryostat"));
    TS_ASSERT_EQUALS(l_property->units(), "K");
  }

private:
  LoadMuonLog loader;
  std::string inputFile;
  std::string outputSpace;
  std::string inputSpace;
};
