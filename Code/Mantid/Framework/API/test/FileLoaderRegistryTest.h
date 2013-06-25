#ifndef MANTID_API_FILELOADERREGISTRYTEST_H_
#define MANTID_API_FILELOADERREGISTRYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/FileLoaderRegistry.h"

using Mantid::API::FileLoaderRegistry;

class FileLoaderRegistryTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FileLoaderRegistryTest *createSuite() { return new FileLoaderRegistryTest(); }
  static void destroySuite( FileLoaderRegistryTest *suite ) { delete suite; }

  void test_Construction_Gives_Empty_Registry()
  {
    FileLoaderRegistry registry;

    TS_ASSERT_EQUALS(0, registry.size());
  }

  void test_Subscribing_Entry_That_Does_Not_Exist_Increases_Size_By_One()
  {
    FileLoaderRegistry registry;

    TS_ASSERT_THROWS_NOTHING(registry.subscribe("LoadEventNexus"));
    TS_ASSERT_EQUALS(1, registry.size());
  }

  // ======================== Failure cases ===================================
  void test_Adding_Entry_That_Already_Exists_Throws_Error_And_Keeps_The_Size_The_Same()
  {
    FileLoaderRegistry registry;
    registry.subscribe("LoadEventNexus");

    TS_ASSERT_THROWS(registry.subscribe("LoadEventNexus"), std::invalid_argument);
    TS_ASSERT_EQUALS(1, registry.size());
  }

  void test_Finding_A_Loader_Throws_Invalid_Argument_If_Filename_Does_Not_Point_To_Valid_File()
  {
    FileLoaderRegistry registry;

    TS_ASSERT_THROWS(registry.findLoader(""), std::invalid_argument);
    TS_ASSERT_THROWS(registry.findLoader("__notafile.txt__"), std::invalid_argument);
  }

};


#endif /* MANTID_API_FILELOADERREGISTRYTEST_H_ */
