#ifndef SNSDATAARCHIVETEST_H_
#define SNSDATAARCHIVETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/ArchiveSearchFactory.h"
#include "MantidDataHandling/SNSDataArchive.h"

using namespace Mantid::DataHandling;
using namespace Mantid::API;

class SNSDataArchiveTest : public CxxTest::TestSuite {
public:
  void xtestSearch() {
    SNSDataArchive arch;

    // PG3 Test case
    std::set<std::string> filename;
    filename.insert("PG3_7390");
    std::vector<std::string> extension =
        std::vector<std::string>(1, "_event.nxs");
    std::string path = arch.getArchivePath(filename, extension);
    TS_ASSERT_EQUALS(path,
                     "/SNS/PG3/IPTS-2767/0/7390/NeXus/PG3_7390_histo.nxs");

    // BSS Test case
    filename.clear();
    filename.insert("BSS_18339");
    path = arch.getArchivePath(filename, extension);
    TS_ASSERT_EQUALS(path,
                     "/SNS/BSS/IPTS-6817/0/18339/NeXus/BSS_18339_event.nxs");

    // HYSA Test case
    filename.clear();
    filename.insert("HYSA_2411");
    extension = std::vector<std::string>(1, ".nxs.h5");
    path = arch.getArchivePath(filename, extension);
    TS_ASSERT_EQUALS(path, "/SNS/HYSA/IPTS-8004/nexus/HYSA_2411.nxs.h5");

    // Test a non-existent file
    filename.clear();
    filename.insert("mybeamline_666");
    extension = std::vector<std::string>(1, ".nxs");
    path = arch.getArchivePath(filename, extension);
    TS_ASSERT(path.empty());
  }

  void testFactory() {
    boost::shared_ptr<IArchiveSearch> arch =
        ArchiveSearchFactory::Instance().create("SNSDataSearch");
    TS_ASSERT(arch);
  }
};

#endif /*SNSDATAARCHIVETEST_H_*/
