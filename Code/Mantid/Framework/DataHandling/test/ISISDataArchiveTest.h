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

  void xtestSearch()
  {
    ISISDataArchive arch;
    std::string path = arch.getPath("hrpd273");
    std::cout << "(hrp273)= " << path << std::endl;
    TS_ASSERT_EQUALS(path.substr(path.size()-18,10),"cycle_98_0");
    path = arch.getPath("hrpds70");
    TS_ASSERT(path.empty());
  }

  void testFactory()
  {
    boost::shared_ptr<IArchiveSearch> arch = ArchiveSearchFactory::Instance().create("ISISDataSearch");
    TS_ASSERT(arch);
  }
  
};
  
#endif /*ISISDATAARCHIVETEST_H_*/
