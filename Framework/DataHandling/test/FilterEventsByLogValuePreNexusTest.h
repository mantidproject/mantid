// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef FILTEREVENTSBYLOGVALUEPRENEXUSTEST_H_
#define FILTEREVENTSBYLOGVALUEPRENEXUSTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidDataHandling/FilterEventsByLogValuePreNexus.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Timer.h"
#include <Poco/File.h>
#include <cxxtest/TestSuite.h>

using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;

class FilterEventsByLogValuePreNexusTest : public CxxTest::TestSuite {

public:
  void test_Init() {
    FilterEventsByLogValuePreNexus filteralg;
    filteralg.initialize();
    TS_ASSERT(filteralg.isInitialized());
  }

  void test_NonExistingFile() {
    FilterEventsByLogValuePreNexus filteralg;
    filteralg.initialize();
    TS_ASSERT_THROWS(filteralg.setProperty(
                         "EventFilename", "this_file_doesnt_exist.blabla.data"),
                     const std::invalid_argument &);
  }

  void test_setProperty() {
    FilterEventsByLogValuePreNexus filteralg;
    filteralg.initialize();

    std::string eventfile("REF_L_32035_neutron_event.dat");
    std::string pulsefile("REF_L_32035_pulseid.dat");

    TS_ASSERT_THROWS_NOTHING(
        filteralg.setPropertyValue("EventFilename", eventfile));
    TS_ASSERT_THROWS_NOTHING(
        filteralg.setPropertyValue("UseParallelProcessing", "Parallel"));
    TS_ASSERT_THROWS_NOTHING(
        filteralg.setPropertyValue("PulseidFilename", pulsefile));
    TS_ASSERT_THROWS_NOTHING(filteralg.setPropertyValue(
        "MappingFilename", "REF_L_TS_2010_02_19.dat"));
    TS_ASSERT_THROWS_NOTHING(
        filteralg.setPropertyValue("OutputWorkspace", "REL_Splitter"));
    TS_ASSERT_THROWS_NOTHING(filteralg.setPropertyValue(
        "EventLogTableWorkspace", "LogTableWorkspace"));
    TS_ASSERT_THROWS_NOTHING(
        filteralg.setPropertyValue("FunctionMode", "Filter"));
    TS_ASSERT_THROWS_NOTHING(
        filteralg.setPropertyValue("PixelIDtoExamine", "122324"));
    TS_ASSERT_THROWS_NOTHING(
        filteralg.setPropertyValue("LogPixelIDs", "122324, 122325"));
    TS_ASSERT_THROWS_NOTHING(
        filteralg.setPropertyValue("LogPIxelTags", "A, B"));
    TS_ASSERT_THROWS_NOTHING(filteralg.setProperty("CorrectTOFtoSample", true));
  }
};

#endif /*FILTEREVENTSBYLOGVALUEPRENEXUS_H_*/
