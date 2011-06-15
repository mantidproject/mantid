#ifndef ORBITERDATAARCHIVETEST_H_
#define ORBITERDATAARCHIVETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/OrbiterDataArchive.h"
#include "MantidAPI/ArchiveSearchFactory.h"

using namespace Mantid::DataHandling;
using namespace Mantid::API;

class OrbiterDataArchiveTest : public CxxTest::TestSuite
{
public: 

  void xtestSearch()
  {
      OrbiterDataArchive arch;
      std::string path = arch.getPath("cncs_23412_event.nxs");
      //std::cout << "(cncs23412)= " << path << std::endl;
      TS_ASSERT_EQUALS(path, "/SNS/CNCS/IPTS-4545/0/23412/NeXus/CNCS_23412_event.nxs");

      // PG3 Test case
      TS_ASSERT_EQUALS(arch.getPath("PG3_2795_event.nxs"), "/SNS/PG3/IPTS-1582/0/2795/NeXus/PG3_2795_event.nxs");

      // Test a non-existent file
      path = arch.getPath("mybeamline_666.nxs");
      TS_ASSERT(path.empty());
  }

  void testFactory()
  {
      boost::shared_ptr<IArchiveSearch> arch = ArchiveSearchFactory::Instance().create("OrbiterDataSearch");
      TS_ASSERT(arch);
  }
  
};
  
#endif /*ORBITERDATAARCHIVETEST_H_*/
