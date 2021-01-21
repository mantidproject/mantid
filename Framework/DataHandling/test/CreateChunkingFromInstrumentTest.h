// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataHandling/CreateChunkingFromInstrument.h"

using Mantid::DataHandling::CreateChunkingFromInstrument;
using namespace Mantid::API;

class CreateChunkingFromInstrumentTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CreateChunkingFromInstrumentTest *createSuite() { return new CreateChunkingFromInstrumentTest(); }
  static void destroySuite(CreateChunkingFromInstrumentTest *suite) { delete suite; }

  void test_Init() {
    CreateChunkingFromInstrument alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_pg3() {
    // Name of the output workspace.
    std::string outWSName("CreateChunkingFromInstrumentTest_OutputPOWGEN");

    CreateChunkingFromInstrument alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InstrumentFilename", "POWGEN_Definition_2015-08-01.xml"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ChunkBy", "Group"););
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<Workspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // Check the results
    ITableWorkspace_sptr tws = std::dynamic_pointer_cast<ITableWorkspace>(ws);
    TS_ASSERT_EQUALS(tws->columnCount(), 1);
    TS_ASSERT_EQUALS(tws->getColumnNames()[0], "BankName");
    TS_ASSERT_EQUALS(tws->rowCount(), 4);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_seq() {
    // Name of the output workspace.
    std::string outWSName("CreateChunkingFromInstrumentTest_OutputSEQ");

    CreateChunkingFromInstrument alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InstrumentName", "seq"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ChunkBy", "All"););
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaxRecursionDepth", 2););
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service. TODO: Change to your desired
    // type
    Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<Workspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // Check the results
    ITableWorkspace_sptr tws = std::dynamic_pointer_cast<ITableWorkspace>(ws);
    TS_ASSERT_EQUALS(tws->columnCount(), 1);
    TS_ASSERT_EQUALS(tws->getColumnNames()[0], "BankName");
    TS_ASSERT_EQUALS(tws->rowCount(), 0);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_seq_fails() {
    // Name of the output workspace.
    std::string outWSName("CreateChunkingFromInstrumentTest_OutputSEQ");

    // configure a version to throw an exception
    CreateChunkingFromInstrument alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InstrumentName", "seq"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ChunkNames", "B row,C row,D row"););
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaxRecursionDepth", 2););
    TS_ASSERT_THROWS_NOTHING(alg.execute()); //, std::runtime_error );
    TS_ASSERT(!alg.isExecuted());
  }
};
