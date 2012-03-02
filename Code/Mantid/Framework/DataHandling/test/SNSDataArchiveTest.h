#ifndef SNSDATAARCHIVETEST_H_
#define SNSDATAARCHIVETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/SNSDataArchive.h"
#include "MantidAPI/ArchiveSearchFactory.h"

using namespace Mantid::DataHandling;
using namespace Mantid::API;

class SNSDataArchiveTest : public CxxTest::TestSuite
{
public: 

  void xtestSearch()
  {
      SNSDataArchive arch;
      std::string path = arch.getPath("PG3_7390_event.nxs");
      TS_ASSERT_EQUALS(path, "/SNS/PG3/IPTS-2767/0/7390/NeXus/PG3_7390_histo.nxs");

      // PG3 Test case
      TS_ASSERT_EQUALS(arch.getPath("BSS_18339_event.nxs"), "/SNS/BSS/IPTS-6817/0/18339/NeXus/BSS_18339_event.nxs");

      // Test a non-existent file
      path = arch.getPath("mybeamline_666.nxs");
      TS_ASSERT(path.empty());
  }

  void testFactory()
  {
      boost::shared_ptr<IArchiveSearch> arch = ArchiveSearchFactory::Instance().create("SNSDataSearch");
      TS_ASSERT(arch);
  }
  
};
  
#endif /*SNSDATAARCHIVETEST_H_*/
