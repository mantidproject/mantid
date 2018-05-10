#ifndef ISISDATAARCHIVETEST_H_
#define ISISDATAARCHIVETEST_H_

#include <cxxtest/TestSuite.h>
#include <fstream>

#include "MantidAPI/ArchiveSearchFactory.h"
#include "MantidDataHandling/ISISDataArchive.h"

using namespace Mantid::DataHandling;
using namespace Mantid::API;

class ISISDataArchiveTest : public CxxTest::TestSuite {
public:
  void xtestSearch() {
    ISISDataArchive arch;

    std::set<std::string> filename;
    filename.insert("hrpd273");
    std::vector<std::string> extension = std::vector<std::string>(1, "");
    std::string path = arch.getArchivePath(filename, extension);
    std::cout << "(hrpd273)= " << path << '\n';
    TS_ASSERT_EQUALS(path.substr(path.size() - 18, 10), "cycle_98_0");

    filename.clear();
    filename.insert("hrpds70");
    path = arch.getArchivePath(filename, extension);
    TS_ASSERT(path.empty());
  }

  void testFactory() {
    boost::shared_ptr<IArchiveSearch> arch =
        ArchiveSearchFactory::Instance().create("ISISDataSearch");
    TS_ASSERT(arch);
  }
};

#endif /*ISISDATAARCHIVETEST_H_*/
