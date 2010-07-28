#ifndef ISISDATAARCHIVETEST_H_
#define ISISDATAARCHIVETEST_H_

#include <fstream>
#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/ISISDataArchive.h"
#include "MantidAPI/ArchiveSearchFactory.h"

using namespace Mantid::DataHandling;
using namespace Mantid::API;

class ISISDataArchiveTest : public CxxTest::TestSuite
{
public: 

  void t1estSearch()
  {
    ISISDataArchive arch;
    std::string path = arch.getPath("hrpd273");
    TS_ASSERT_EQUALS(path.substr(path.size()-18,10),"cycle_98_0");
    path = arch.getPath("hrpds70");
    TS_ASSERT(path.empty());
  }

  void t1estFactory()
  {
    boost::shared_ptr<IArchiveSearch> arch = ArchiveSearchFactory::Instance().create("ISIS");
    TS_ASSERT(arch);
    std::string path = arch->getPath("hrpd273");
    TS_ASSERT_EQUALS(path.substr(path.size()-18,10),"cycle_98_0");
  }
  
};
  
#endif /*ISISDATAARCHIVETEST_H_*/
