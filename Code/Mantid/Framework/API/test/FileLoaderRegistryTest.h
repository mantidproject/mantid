#ifndef MANTID_API_FILELOADERREGISTRYTEST_H_
#define MANTID_API_FILELOADERREGISTRYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FileLoaderRegistry.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidKernel/FileDescriptor.h"

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

    TS_ASSERT_THROWS_NOTHING(registry.subscribe<StubNonLoader>(FileLoaderRegistry::NonHDF));
    TS_ASSERT_EQUALS(1, registry.size());

    // We can't mock the factory as it's a singleton so make sure we clean up
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("StubNonLoader", 1);
  }

  void test_chooseLoader_Throws_For_NonExistant_File()
  {
    FileLoaderRegistry registry;

    TS_ASSERT_THROWS(registry.chooseLoader("__not_a_file.txt"), std::invalid_argument);
  }

  void test_chooseLoader_Returns_Expected_Loader_Name_For_Given_File()
  {
    FileLoaderRegistry registry;
    registry.subscribe<RawLoader>(FileLoaderRegistry::NonHDF);
    registry.subscribe<TxtLoader>(FileLoaderRegistry::NonHDF);

    const std::string filename = Mantid::API::FileFinder::Instance().getFullPath("AsciiExample.txt");

    std::string algName;
    TS_ASSERT_THROWS_NOTHING(algName = registry.chooseLoader(filename));
    TS_ASSERT_EQUALS("TxtLoader", algName);

    // We can't mock the factory as it's a singleton so make sure we clean up
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("RawLoader", 1);
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("TxtLoader", 1);
  }

  // ======================== Failure cases ===================================
  void test_Adding_Entry_That_Already_Exists_Throws_Error_And_Keeps_The_Size_The_Same()
  {
    FileLoaderRegistry registry;
    registry.subscribe<StubNonLoader>(FileLoaderRegistry::NonHDF);

    TS_ASSERT_THROWS(registry.subscribe<StubNonLoader>(FileLoaderRegistry::NonHDF), std::runtime_error);
    TS_ASSERT_EQUALS(1, registry.size());

    // We can't mock the factory as it's a singleton so make sure we clean up
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("StubNonLoader", 1);
  }

private:
  // Stub algorithm for test
  struct StubNonLoader : Mantid::API::Algorithm
  {
    const std::string name() const { return "StubNonLoader"; }
    int version() const { return 1; }
    void init() {};
    void exec() {};
  };

  // Stub algorithm for test
  struct RawLoader : Mantid::API::IFileLoader
  {
    const std::string name() const { return "RawLoader"; }
    int version() const { return 1; }
    void init() {};
    void exec() {};
    int confidence(const Mantid::Kernel::FileDescriptor & descr) const
    {
      if(descr.extension() == ".raw") return 80;
      else return 0;
    }
  };

  // Stub algorithm for test
  struct TxtLoader : Mantid::API::IFileLoader
  {
    const std::string name() const { return "TxtLoader"; }
    int version() const { return 1; }
    void init() {};
    void exec() {};
    int confidence(const Mantid::Kernel::FileDescriptor & descr) const
    {
      if(descr.extension() == ".txt") return 80;
      else return 0;
    }
  };

};


#endif /* MANTID_API_FILELOADERREGISTRYTEST_H_ */
