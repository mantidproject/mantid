// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ICatTestHelper.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidICat/CatalogDownloadDataFiles.h"
#include "MantidICat/CatalogGetDataFiles.h"
#include "MantidICat/CatalogSearch.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::ICat;
using namespace Mantid::API;
using namespace ICatTestHelper;

class CatalogDownloadDataFilesTest : public CxxTest::TestSuite {
public:
  // This means the constructor isn't called when running other tests
  static CatalogDownloadDataFilesTest *createSuite() { return new CatalogDownloadDataFilesTest(); }

  static void destroySuite(CatalogDownloadDataFilesTest *suite) { delete suite; }

  CatalogDownloadDataFilesTest()
      : m_fakeLogin(std::make_unique<FakeICatLogin>()), m_sessionID(m_fakeLogin->getSessionId()) {}

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(downloadobj.initialize());
    TS_ASSERT(downloadobj.isInitialized());
  }

  void testDownLoadDataFile() {
    if (!searchobj.isInitialized())
      searchobj.initialize();
    searchobj.setPropertyValue("RunRange", "100-102");
    searchobj.setPropertyValue("Instrument", "HET");
    searchobj.setPropertyValue("Session", m_sessionID);
    searchobj.setPropertyValue("OutputWorkspace", "investigations");

    TS_ASSERT_THROWS_NOTHING(searchobj.execute());
    TS_ASSERT(searchobj.isExecuted());

    if (!invstObj.isInitialized())
      invstObj.initialize();
    invstObj.setPropertyValue("InvestigationId", "13539191");
    invstObj.setPropertyValue("Session", m_sessionID);
    invstObj.setPropertyValue("OutputWorkspace",
                              "investigation"); // selected invesigation

    TS_ASSERT_THROWS_NOTHING(invstObj.execute());
    TS_ASSERT(invstObj.isExecuted());

    if (!downloadobj.isInitialized())
      downloadobj.initialize();

    downloadobj.setPropertyValue("Filenames", "HET00097.RAW");
    downloadobj.setPropertyValue("Session", m_sessionID);

    TS_ASSERT_THROWS_NOTHING(downloadobj.execute());
    TS_ASSERT(downloadobj.isExecuted());
  }

  void testDownLoadNexusFile() {
    if (!searchobj.isInitialized())
      searchobj.initialize();
    searchobj.setPropertyValue("RunRange", "17440-17556");
    searchobj.setPropertyValue("Instrument", "EMU");
    searchobj.setPropertyValue("Session", m_sessionID);
    searchobj.setPropertyValue("OutputWorkspace", "investigations");

    TS_ASSERT_THROWS_NOTHING(searchobj.execute());
    TS_ASSERT(searchobj.isExecuted());

    if (!invstObj.isInitialized())
      invstObj.initialize();

    invstObj.setPropertyValue("InvestigationId", "24070400");
    invstObj.setPropertyValue("Session", m_sessionID);
    invstObj.setPropertyValue("OutputWorkspace", "investigation");

    TS_ASSERT_THROWS_NOTHING(invstObj.execute());
    TS_ASSERT(invstObj.isExecuted());

    if (!downloadobj.isInitialized())
      downloadobj.initialize();

    downloadobj.setPropertyValue("Filenames", "EMU00017452.nxs");
    downloadobj.setPropertyValue("Session", m_sessionID);

    TS_ASSERT_THROWS_NOTHING(downloadobj.execute());
    TS_ASSERT(downloadobj.isExecuted());
  }

  void testDownLoadDataFile_Merlin() {
    if (!searchobj.isInitialized())
      searchobj.initialize();
    searchobj.setPropertyValue("RunRange", "600-601");
    searchobj.setPropertyValue("Instrument", "MERLIN");
    searchobj.setPropertyValue("Session", m_sessionID);
    searchobj.setPropertyValue("OutputWorkspace", "investigations");

    TS_ASSERT_THROWS_NOTHING(searchobj.execute());
    TS_ASSERT(searchobj.isExecuted());

    if (!invstObj.isInitialized())
      invstObj.initialize();
    invstObj.setPropertyValue("InvestigationId", "24022007");
    invstObj.setPropertyValue("Session", m_sessionID);
    invstObj.setPropertyValue("OutputWorkspace",
                              "investigation"); // selected invesigation

    TS_ASSERT_THROWS_NOTHING(invstObj.execute());
    TS_ASSERT(invstObj.isExecuted());

    if (!downloadobj.isInitialized())
      downloadobj.initialize();

    downloadobj.setPropertyValue("Filenames", "MER00599.raw");
    downloadobj.setPropertyValue("Session", m_sessionID);

    TS_ASSERT_THROWS_NOTHING(downloadobj.execute());
    TS_ASSERT(downloadobj.isExecuted());
  }

private:
  CatalogSearch searchobj;
  CatalogGetDataFiles invstObj;
  CatalogDownloadDataFiles downloadobj;

  std::unique_ptr<FakeICatLogin> m_fakeLogin;
  std::string m_sessionID;
};
