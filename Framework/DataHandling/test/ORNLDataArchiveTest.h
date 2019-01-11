// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ORNLDATAARCHIVETEST_H_
#define ORNLDATAARCHIVETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/ArchiveSearchFactory.h"
#include "MantidCatalog/Exception.h"
#include "MantidCatalog/ONCat.h"
#include "MantidDataHandling/ORNLDataArchive.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/InternetHelper.h"
#include "MantidKernel/make_unique.h"
#include "MantidTestHelpers/ONCatHelper.h"

#include <map>
#include <memory>

#include <Poco/Net/HTTPResponse.h>

using Poco::Net::HTTPResponse;

using namespace Mantid::DataHandling;
using namespace Mantid::API;

using Mantid::Catalog::ONCat::ONCat;
using Mantid::Kernel::Exception::InternetError;

using Mantid::TestHelpers::make_mock_oncat_api;
using Mantid::TestHelpers::make_oncat_with_mock_api;

class ORNLDataArchiveTest : public CxxTest::TestSuite {
public:
  void testSearch() {
    ORNLDataArchive arch;

    const auto generateRunUrl = [](const std::string &facility,
                                   const std::string &instrument,
                                   const std::string &runNumber) {
      return std::string("https://oncat.ornl.gov/api/datafiles"
                         "?facility=") +
             facility + "&instrument=" + instrument +
             "&projection=location"
             "&tags=type/raw"
             "&sort_by=ingested"
             "&sort_direction=DESCENDING"
             "&ranges_q=indexed.run_number:" +
             runNumber;
    };

    auto mockAPI = make_mock_oncat_api(
        {{generateRunUrl("SNS", "PG3", "7390"),
          {HTTPResponse::HTTP_OK,
           "["
           "  {"
           "    \"location\": "
           "    \"/SNS/PG3/IPTS-2767/0/7390/NeXus/PG3_7390_event.nxs\","
           "    \"id\": \"5b9be1154e7bcae781c9ca09\","
           "    \"indexed\": {"
           "      \"run_number\": 7390"
           "    },"
           "    \"type\": \"datafile\""
           "  }"
           "]"}},
         {generateRunUrl("HFIR", "HB2C", "26506"),
          {HTTPResponse::HTTP_OK,
           "["
           "  {"
           "    \"location\": "
           "    \"/HFIR/HB2C/IPTS-7776/nexus/HB2C_26506.nxs.h5\","
           "    \"id\": \"5ba1c86a4e7bcae781440283\","
           "    \"indexed\": {"
           "      \"run_number\": 26506"
           "    },"
           "    \"type\": \"datafile\""
           "  }"
           "]"}},
         {generateRunUrl("SNS", "PG3", "9999999"),
          {HTTPResponse::HTTP_OK, "[]"}},
         {generateRunUrl("SNS", "PG3", "500"),
          {HTTPResponse::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR,
           "{\"message\" : \"Stack trace or similar...\"}"}},
         {generateRunUrl("SNS", "PG3", "200"),
          {HTTPResponse::HTTPResponse::HTTP_OK,
           "["
           "  {"
           "    \"location\": "
           "    \"/SNS/PG3/IPTS-2767/0/7390/NeXus/PG3_7390_event.nxs\","
           "    \"id\": \"5b9be115"}}});

    auto oncat = make_oncat_with_mock_api(mockAPI);
    arch.setONCat(std::move(oncat));

    // These two inputs are valid, and should return a result after making the
    // exact same underlying call to ONCat.
    TS_ASSERT_EQUALS(arch.getArchivePath({"PG3_7390"}, {"_event.nxs"}),
                     "/SNS/PG3/IPTS-2767/0/7390/NeXus/PG3_7390_event.nxs");
    TS_ASSERT_EQUALS(arch.getArchivePath({"PG3_7390_event.nxs"}, {}),
                     "/SNS/PG3/IPTS-2767/0/7390/NeXus/PG3_7390_event.nxs");

    // Make sure we support HFIR, too!
    TS_ASSERT_EQUALS(arch.getArchivePath({"HB2C_26506"}, {".nxs.h5"}),
                     "/HFIR/HB2C/IPTS-7776/nexus/HB2C_26506.nxs.h5");
    TS_ASSERT_EQUALS(arch.getArchivePath({"HB2C_26506.nxs.h5"}, {}),
                     "/HFIR/HB2C/IPTS-7776/nexus/HB2C_26506.nxs.h5");

    // Return nothing when the run has not been cataloged in ONCat.
    TS_ASSERT_EQUALS(arch.getArchivePath({"PG3_9999999"}, {"_event.nxs"}), "");

    // Mimic old behaviour by returning nothing when asking for a run known to
    // ONCat but without providing the "suffix" of the basename.
    TS_ASSERT_EQUALS(arch.getArchivePath({"PG3_7390"}, {}), "");
    TS_ASSERT_EQUALS(arch.getArchivePath({"PG3_7390"}, {""}), "");

    // Ask stupid questions, get stupid answers.
    TS_ASSERT_EQUALS(arch.getArchivePath({"PG37390"}, {"_event.nxs"}), "");
    TS_ASSERT_EQUALS(arch.getArchivePath({"PG3-7390"}, {"_event.nxs"}), "");
    TS_ASSERT_EQUALS(arch.getArchivePath({"PG3_x7390"}, {"_event.nxs"}), "");

    // When an error is returned from ONCat, this should result in an empty
    // string as if the file is not found.  The error will be logged.
    TS_ASSERT_EQUALS(arch.getArchivePath({"PG3_500"}, {"_event.nxs"}), "");

    // Make sure we don't seg fault or similar when an OK status and incomplete
    // bit of JSON has been returned.
    TS_ASSERT_EQUALS(arch.getArchivePath({"PG3_200"}, {"_event.nxs"}), "");

    // Instruments not known to Mantid, or not compatible with the archive
    // class should not return anything, either.
    TS_ASSERT_EQUALS(arch.getArchivePath({"DOESNOTEXIST_200"}, {""}), "");
    TS_ASSERT_EQUALS(arch.getArchivePath({"MERLIN_200"}, {""}), "");

    TS_ASSERT(mockAPI->allResponsesCalled());
  }

  void testFactory() {
    boost::shared_ptr<IArchiveSearch> arch =
        ArchiveSearchFactory::Instance().create("ORNLDataSearch");
    TS_ASSERT(arch);
  }
};

#endif // ORNLDATAARCHIVETEST_H_
