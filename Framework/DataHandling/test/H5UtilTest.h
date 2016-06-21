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
  // This pair of boilerplate methods prevent the suite being created
  // statically
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
    do_assert_simple_string_data_set(FILENAME, GRP_NAME, DATA_NAME, DATA_VALUE);

    // cleanup
    removeFile(FILENAME);
  }

  void test_string_data_sets_with_attributes() {
    // Arrange
    const std::string FILENAME("H5UtilTest_strings.h5");
    const std::string GRP_NAME("strings");
    const std::string DATA_NAME("simple");
    const std::string DATA_VALUE("H5Util");

    const std::string ATTR_NAME_1("attributeName1");
    const std::string ATTR_VALUE_1("attriuteValue1");
    const std::string ATTR_NAME_2("attributeName2");
    const std::string ATTR_VALUE_2("attriuteValue2");

    std::map<std::string, std::string> attributesScalar{
        {ATTR_NAME_1, ATTR_VALUE_1}, {ATTR_NAME_2, ATTR_VALUE_2}};

    removeFile(FILENAME);

    // Act
    { // write tests
      H5File file(FILENAME, H5F_ACC_EXCL);
      auto group = H5Util::createGroupNXS(file, GRP_NAME, "NXentry");
      H5Util::writeWithStrAttributes(group, DATA_NAME, DATA_VALUE,
                                     attributesScalar);
      file.close();
    }

    // Assert
    TS_ASSERT(Poco::File(FILENAME).exists());
    do_assert_simple_string_data_set(FILENAME, GRP_NAME, DATA_NAME, DATA_VALUE,
                                     attributesScalar);

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

private:
  void do_assert_simple_string_data_set(
      const std::string &filename, const std::string &groupName,
      const std::string &dataName, const std::string &dataValue,
      const std::map<std::string, std::string> &attributes =
          std::map<std::string, std::string>()) {
    TS_ASSERT(Poco::File(filename).exists());

    // read tests
    H5File file(filename, H5F_ACC_RDONLY);

    auto fullCheck = H5Util::readString(file, "/" + groupName + "/" + dataName);
    TS_ASSERT_EQUALS(fullCheck, dataValue);

    auto group = file.openGroup(groupName);
    auto groupCheck = H5Util::readString(group, dataName);
    TS_ASSERT_EQUALS(groupCheck, dataValue);

    auto data = group.openDataSet(dataName);
    auto dataCheck = H5Util::readString(data);
    TS_ASSERT_EQUALS(dataCheck, dataValue);

    // Check the attributes
    do_test_attributes_on_data_set(data, attributes);

    file.close();
  }

  void do_test_attributes_on_data_set(
      H5::DataSet &data, std::map<std::string, std::string> attributes) {
    auto numAttributes = data.getNumAttrs();
    auto expectedNumAttributes = static_cast<int>(attributes.size());
    TSM_ASSERT_EQUALS("There should be two attributes present.",
                      expectedNumAttributes, numAttributes);

    for (auto &attribute : attributes) {
      auto value = H5Util::readAttributeAsString(data, attribute.first);
      TSM_ASSERT_EQUALS("Should retrieve the correct attribute value",
                        attribute.second, value);
    }
  }
};

#endif /* MANTID_DATAHANDLING_H5UTILTEST_H_ */
