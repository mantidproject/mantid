#ifndef MANTID_API_FILELOADERREGISTRYTEST_H_
#define MANTID_API_FILELOADERREGISTRYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/Algorithm.h"
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

    TS_ASSERT_THROWS_NOTHING(registry.subscribe<StubAlgorithm>(FileLoaderRegistry::NonHDF));
    TS_ASSERT_EQUALS(1, registry.size());

    // We can't mock the factory as it's a singleton so make sure we clean up
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("StubAlgorithm", 1);
  }

  // ======================== Failure cases ===================================
  void test_Adding_Entry_That_Already_Exists_Throws_Error_And_Keeps_The_Size_The_Same()
  {
    FileLoaderRegistry registry;
    registry.subscribe<StubAlgorithm>(FileLoaderRegistry::NonHDF);

    TS_ASSERT_THROWS(registry.subscribe<StubAlgorithm>(FileLoaderRegistry::NonHDF), std::runtime_error);
    TS_ASSERT_EQUALS(1, registry.size());

    // We can't mock the factory as it's a singleton so make sure we clean up
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("StubAlgorithm", 1);
  }

  private:
  // Stub algorithm for test
  struct StubAlgorithm : Mantid::API::Algorithm
  {
    const std::string name() const { return "StubAlgorithm"; }
    int version() const { return 1; }
    void init() {};
    void exec() {};
  };

};


#endif /* MANTID_API_FILELOADERREGISTRYTEST_H_ */
