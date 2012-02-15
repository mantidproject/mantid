#ifndef MANTID_API_MULTIPLEFILEPROPERTYTEST_H_
#define MANTID_API_MULTIPLEFILEPROPERTYTEST_H_

#include "MantidAPI/MultipleFileProperty.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include <Poco/Path.h>

using namespace Mantid;
using namespace Mantid::API;

class MultipleFilePropertyTest : public CxxTest::TestSuite
{
public:
  
  void test_setValue()
  {
    MultipleFileProperty p("Filename");
    p.setValue("REF_L_32035.nxs, CSP78173.raw");
    std::vector<std::vector<std::string> > filenames = p();
    TS_ASSERT_EQUALS( filenames.size(), 2);
    TSM_ASSERT( "Files with no path are found using ConfigService paths", Poco::Path(filenames[0][0]).isAbsolute() );
    TSM_ASSERT( "Files with no path are found using ConfigService paths", Poco::Path(filenames[1][0]).isAbsolute() );
  }
  
  void test_setValue2()
  {
    // Here we try to load some files, some of them to be added together.  We should end up with a vector of vectors as follows:
    //
    // | [dir]MUSR00015189.nxs | [dir]MUSR00015190.nxs | [dir]MUSR00015189.nxs | [dir]MUSR00015191.nxs |
    //                                                 | [dir]MUSR00015190.nxs |
    //                                                 | [dir]MUSR00015191.nxs |

    MultipleFileProperty p("Filename");
    p.setValue("MUSR15189:15190,15189-15191,15191.nxs");
    std::vector<std::vector<std::string> > fileNames = p();

    TS_ASSERT_EQUALS(fileNames[0].size(), 1);
    TS_ASSERT_EQUALS(fileNames[1].size(), 1);
    TS_ASSERT_EQUALS(fileNames[2].size(), 3);
    TS_ASSERT_EQUALS(fileNames[3].size(), 1);
  }


};


#endif /* MANTID_API_MULTIPLEFILEPROPERTYTEST_H_ */

