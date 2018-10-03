// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef RAWFILEINFOTEST_H_
#define RAWFILEINFOTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataHandling/RawFileInfo.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::DataHandling;

class RawFileInfoTest : public CxxTest::TestSuite {

public:
  static RawFileInfoTest *createSuite() { return new RawFileInfoTest(); }
  static void destroySuite(RawFileInfoTest *suite) { delete suite; }
  // Perform test with a GEM file
  RawFileInfoTest() : m_filetotest("LOQ48127.raw") {}

  // Test output parameters without table workspace output
  void testNoRunParameters() { runTest(false); }

  void testGetRunParameters() { runTest(true); }

  void testGetSampleParameters() { runTest(false, true); }

private:
  // Check the parameters are correct
  void runTest(bool tableToExist, bool getSampleParameters = false) {
    RawFileInfo alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_EQUALS(alg.isInitialized(), true);

    // Set the file name
    alg.setPropertyValue("Filename", m_filetotest);
    if (tableToExist) {
      alg.setPropertyValue("GetRunParameters", "1");
    }

    if (getSampleParameters) {
      alg.setPropertyValue("GetSampleParameters", "1");
    }

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT_EQUALS(alg.isExecuted(), true);

    // Check the output parameters are what we expect
    std::string title = alg.getProperty("RunTitle");
    TS_ASSERT_EQUALS(title, std::string("direct beam"));
    std::string header = alg.getProperty("RunHeader");
    TS_ASSERT_EQUALS(header,
                     std::string("LOQ 48127 LOQ team & SANS Xpre direct beam   "
                                 "           18-DEC-2008  17:58:38    10.04"));
    int spectra_count = alg.getProperty("SpectraCount"); // 7290
    TS_ASSERT_EQUALS(spectra_count, 8);

    int bin_count = alg.getProperty("TimeChannelCount"); // 5050
    TS_ASSERT_EQUALS(bin_count, 102);

    int prd_count = alg.getProperty("PeriodCount");
    TS_ASSERT_EQUALS(prd_count, 1);

    // Finally test that a workspace existence is correct
    TS_ASSERT_EQUALS(
        Mantid::API::AnalysisDataService::Instance().doesExist("Raw_RPB"),
        tableToExist);

    if (tableToExist) {
      Mantid::API::Workspace_sptr workspace =
          Mantid::API::AnalysisDataService::Instance().retrieve("Raw_RPB");
      TS_ASSERT(workspace.get());

      Mantid::API::ITableWorkspace_sptr run_table =
          boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(workspace);
      TS_ASSERT(run_table.get());

      // Check a couple of things
      int r_goodfrm = run_table->getRef<int>("r_goodfrm", 0);
      TS_ASSERT_EQUALS(r_goodfrm, 9229);

      int r_dur = run_table->getRef<int>("r_dur", 0);
      TS_ASSERT_EQUALS(r_dur, 462);

      double r_gd_prtn_chrg = run_table->getRef<double>("r_gd_prtn_chrg", 0);
      TS_ASSERT_DELTA(r_gd_prtn_chrg, 10.0409, 1e-4);

      std::string r_enddate = run_table->getRef<std::string>("r_enddate", 0);
      TS_ASSERT_EQUALS(r_enddate, "18-DEC-2008");

      // Tidy up
      Mantid::API::AnalysisDataService::Instance().remove("Raw_RPB");
    }

    if (getSampleParameters) {
      Mantid::API::Workspace_sptr workspace =
          Mantid::API::AnalysisDataService::Instance().retrieve("Raw_SPB");
      TS_ASSERT(workspace.get());

      Mantid::API::ITableWorkspace_sptr sample_table =
          boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(workspace);
      TS_ASSERT(sample_table.get());

      // Sample type
      int e_type = sample_table->getRef<int>("e_type", 0);
      TS_ASSERT_EQUALS(e_type, 1);

      // Sample geometry
      int e_geom = sample_table->getRef<int>("e_geom", 0);
      TS_ASSERT_EQUALS(e_geom, 3);

      // Sample thickness
      double e_thick = sample_table->getRef<double>("e_thick", 0);
      TS_ASSERT_EQUALS(e_thick, 2.);

      // Sample height
      double e_height = sample_table->getRef<double>("e_height", 0);
      TS_ASSERT_EQUALS(e_height, 8.);

      // Sample width
      double e_width = sample_table->getRef<double>("e_width", 0);
      TS_ASSERT_EQUALS(e_width, 8.);

      // Tidy up
      Mantid::API::AnalysisDataService::Instance().remove("Raw_SPB");
    }
  }

private:
  // This assumes the directory structure of the repository (i.e the
  // buildserver)
  const std::string m_filetotest;
};

#endif
