#ifndef MANTID_DATAHANDLING_H5UTILTEST_H_
#define MANTID_DATAHANDLING_H5UTILTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/System.h"
#include "MantidDataHandling/H5Util.h"

#include <boost/numeric/conversion/cast.hpp>
#include <H5Cpp.h>
#include <limits>
#include <Poco/File.h>

using namespace H5;
using namespace Mantid::DataHandling;

class H5UtilTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static H5UtilTest *createSuite() { return new H5UtilTest(); }
  static void destroySuite(H5UtilTest *suite) { delete suite; }

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
    const std::string GRP_NAME("array1d");
    const std::vector<float> array1d_float = {0, 1, 2, 3, 4};
    const std::vector<double> array1d_double = {0, 1, 2, 3, 4};
    const std::vector<int32_t> array1d_int32 = {
        0, 1, 2, 3, 4, std::numeric_limits<int32_t>::max()};
    const std::vector<uint32_t> array1d_uint32 = {
        0, 1, 2, 3, 4, std::numeric_limits<uint32_t>::max()};

    // HDF doesn't like opening existing files in write mode
    removeFile(FILENAME);

    { // write tests
      H5File file(FILENAME, H5F_ACC_EXCL);
      auto group = H5Util::createGroupNXS(file, GRP_NAME, "NXentry");
      H5Util::writeArray1D(group, "array1d_float", array1d_float);
      H5Util::writeArray1D(group, "array1d_double", array1d_double);
      H5Util::writeArray1D(group, "array1d_int32", array1d_int32);
      H5Util::writeArray1D(group, "array1d_uint32", array1d_uint32);
      file.close();
    }

    TS_ASSERT(Poco::File(FILENAME).exists());

    { // read tests
      H5File file(FILENAME, H5F_ACC_RDONLY);
      auto group = file.openGroup(GRP_NAME);

      // without conversion
      TS_ASSERT_EQUALS(H5Util::readArray1DCoerce<float>(group, "array1d_float"),
                       array1d_float);
      TS_ASSERT_EQUALS(
          H5Util::readArray1DCoerce<double>(group, "array1d_double"),
          array1d_double);
      TS_ASSERT_EQUALS(
          H5Util::readArray1DCoerce<int32_t>(group, "array1d_int32"),
          array1d_int32);
      TS_ASSERT_EQUALS(
          H5Util::readArray1DCoerce<uint32_t>(group, "array1d_uint32"),
          array1d_uint32);

      //  with conversion
      TS_ASSERT_EQUALS(
          H5Util::readArray1DCoerce<double>(group, "array1d_float"),
          array1d_double);
      TS_ASSERT_THROWS(
          H5Util::readArray1DCoerce<int32_t>(group, "array1d_uint32"),
          boost::numeric::positive_overflow);
      TS_ASSERT_THROWS_NOTHING(
          H5Util::readArray1DCoerce<uint32_t>(group, "array1d_int32"));

      file.close();
    }

    // cleanup
    removeFile(FILENAME);
  }
};

#endif /* MANTID_DATAHANDLING_H5UTILTEST_H_ */
