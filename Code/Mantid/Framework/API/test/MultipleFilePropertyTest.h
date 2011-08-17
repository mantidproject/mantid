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
using namespace Mantid::API;

class MultipleFilePropertyTest : public CxxTest::TestSuite
{
public:

  void test_empty_value_not_allowed()
  {
    MultipleFileProperty p("Filename");
    TS_ASSERT_DIFFERS( p.setValue(""), "");
  }

  void test_empty_value_allowed_ifOptional()
  {
    MultipleFileProperty p("Filename", std::vector<std::string>(), true);
    TS_ASSERT_EQUALS( p.setValue(""), "");
  }

  void test_getExts()
  {
    std::vector<std::string> exts;
    exts.push_back(".nxs");
    exts.push_back(".hdf");

    MultipleFileProperty p("Filename", exts);
    TS_ASSERT_EQUALS( p.getExts()[0], ".nxs");
    TS_ASSERT_EQUALS( p.getExts()[1], ".hdf");
  }

  void test_setValue()
  {
    MultipleFileProperty p("Filename");
    p.setValue("CNCS_7860.nxs, CSP78173.raw");
    std::vector<std::string> filenames = p();
    TS_ASSERT_EQUALS( filenames.size(), 2);
    TSM_ASSERT( "Files with no path are found using ConfigService paths", Poco::Path(filenames[0]).isAbsolute() );
    TSM_ASSERT( "Files with no path are found using ConfigService paths", Poco::Path(filenames[1]).isAbsolute() );

  }


};


#endif /* MANTID_API_MULTIPLEFILEPROPERTYTEST_H_ */

