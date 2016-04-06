#ifndef MANTID_DATAHANDLING_H5UTILTEST_H_
#define MANTID_DATAHANDLING_H5UTILTEST_H_

#include <cxxtest/TestSuite.h>
#include <H5Cpp.h>
#include <Poco/File.h>

#include "MantidDataHandling/H5Util.h"

using namespace H5;
using namespace Mantid::DataHandling;

class H5UtilTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static H5UtilTest *createSuite() { return new H5UtilTest(); }
  static void destroySuite( H5UtilTest *suite ) { delete suite; }

  void removeFile(const std::string &filename) {
    if (Poco::File(filename).exists())
      Poco::File(filename).remove();
  }

  void test_strings() {
    const std::string FILENAME("H5UtilTest_strings.h5");
    const std::string GRP_NAME("strings");
    const std::string DATA_NAME("simple");
    const std::string DATA_VALUE("H5Util");

    // HDF doesn't like opening existing files in write mode
    removeFile(FILENAME);

    { // write tests
      H5File file(FILENAME, H5F_ACC_EXCL);
      auto group = H5Util::createGroupNXS(file, GRP_NAME, "NXentry");
      H5Util::write(group, DATA_NAME, DATA_VALUE);
      file.close();
    }

    TS_ASSERT(Poco::File(FILENAME).exists());

    { // read tests
      H5File file(FILENAME, H5F_ACC_RDONLY);

      auto fullCheck =
          H5Util::readString(file, "/" + GRP_NAME + "/" + DATA_NAME);
      TS_ASSERT_EQUALS(fullCheck, DATA_VALUE);

      auto group = file.openGroup(GRP_NAME);
      auto groupCheck = H5Util::readString(group, DATA_NAME);
      TS_ASSERT_EQUALS(groupCheck, DATA_VALUE);

      auto data = group.openDataSet(DATA_NAME);
      auto dataCheck = H5Util::readString(data);
      TS_ASSERT_EQUALS(dataCheck, DATA_VALUE);

      file.close();
    }

    // cleanup
    removeFile(FILENAME);
  }

  void test_array1d() {
    const std::string FILENAME("H5UtilTest_array1d.h5");

    // HDF doesn't like opening existing files in write mode
    removeFile(FILENAME);

    { // write tests
      H5File file(FILENAME, H5F_ACC_EXCL);
    }

    TS_ASSERT(Poco::File(FILENAME).exists());

    { // read tests
      H5File file(FILENAME, H5F_ACC_RDONLY);
      file.close();
    }

    //    template <typename NumT>
    //    void writeArray1D(H5::Group &group, const std::string &name,
    //                    const std::vector<NumT> &values);

    //    template <typename NumT>
    //    std::vector<NumT> readArray1DCoerce(H5::DataSet &dataset,
    //                                      const H5::DataType
    //                                      &desiredDataType);

    // cleanup
    removeFile(FILENAME);
  }


};


#endif /* MANTID_DATAHANDLING_H5UTILTEST_H_ */
