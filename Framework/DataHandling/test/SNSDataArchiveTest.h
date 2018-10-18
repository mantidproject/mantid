// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SNSDATAARCHIVETEST_H_
#define SNSDATAARCHIVETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/ArchiveSearchFactory.h"
#include "MantidDataHandling/SNSDataArchive.h"
#include "MantidCatalog/Exception.h"
#include "MantidCatalog/ONCat.h"
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

class SNSDataArchiveTest : public CxxTest::TestSuite {
public:
  void testSearch() {
    SNSDataArchive arch;

    const auto generatePowgenRunUrl = [](const std::string &runNumber)  {
      return std::string(
          "https://oncat.ornl.gov/api/datafiles"
          "?facility=SNS"
          "&instrument=PG3"
          "&projection=location"
          "&tags=type/raw"
          "&sort_by=ingested"
          "&sort_direction=DESCENDING"
          "&ranges_q=indexed.run_number:") + runNumber;
    };

    auto mockAPI = make_mock_oncat_api(
        {{generatePowgenRunUrl("7390"),
          std::make_pair(
              HTTPResponse::HTTP_OK,
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
              "]")},
         {generatePowgenRunUrl("9999999"),
          std::make_pair(HTTPResponse::HTTP_OK, "[]")},
         {generatePowgenRunUrl("500"),
          std::make_pair(
              HTTPResponse::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR,
              "{\"message\" : \"Stack trace or similar...\"}")},
         {generatePowgenRunUrl("200"),
          std::make_pair(
              HTTPResponse::HTTPResponse::HTTP_OK,
              "["
              "  {"
              "    \"location\": "
              "    \"/SNS/PG3/IPTS-2767/0/7390/NeXus/PG3_7390_event.nxs\","
              "    \"id\": \"5b9be115")}});

    auto oncat = make_oncat_with_mock_api(mockAPI);
    arch.setONCat(std::move(oncat));

    // These two inputs are valid, and should return a result after making the
    // exact same underlying call to ONCat.
    TS_ASSERT_EQUALS(arch.getArchivePath({"PG3_7390"}, {"_event.nxs"}),
                     "/SNS/PG3/IPTS-2767/0/7390/NeXus/PG3_7390_event.nxs");
    TS_ASSERT_EQUALS(arch.getArchivePath({"PG3_7390_event.nxs"}, {}),
                     "/SNS/PG3/IPTS-2767/0/7390/NeXus/PG3_7390_event.nxs");

    // Return nothing when the run has not been cataloged in ONCat.
    TS_ASSERT_EQUALS(arch.getArchivePath({"PG3_9999999"}, {"_event.nxs"}), "");

    // Mimic old behaviour by returning nothing when asking for a run known to
    // ONCat but without providing the "suffix" of the basename.
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

    TS_ASSERT(mockAPI->allResponsesCalled());
  }

  void testFactory() {
    boost::shared_ptr<IArchiveSearch> arch =
        ArchiveSearchFactory::Instance().create("SNSDataSearch");
    TS_ASSERT(arch);
  }
};

#endif // SNSDATAARCHIVETEST_H_
