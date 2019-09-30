// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_H5UTILTEST_H_
#define MANTID_DATAHANDLING_H5UTILTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/H5Util.h"
#include "MantidKernel/System.h"

#include <H5Cpp.h>
#include <Poco/File.h>
#include <boost/numeric/conversion/cast.hpp>
#include <limits>

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

    const std::string ATTR_NAME_3("attributeName3");
    const float ATTR_VALUE_3(123.0f);
    const std::string ATTR_NAME_4("attributeName4");
    const int ATTR_VALUE_4(7);

    const std::string ATTR_NAME_5("attributeName5");
    const std::vector<float> ATTR_VALUE_5 = {12.5f, 34.6f, 455.5f};
    const std::string ATTR_NAME_6("attributeName6");
    const std::vector<int> ATTR_VALUE_6 = {12, 44, 78};

    std::map<std::string, std::string> stringAttributesScalar{
        {ATTR_NAME_1, ATTR_VALUE_1}, {ATTR_NAME_2, ATTR_VALUE_2}};

    removeFile(FILENAME);

    // Act
    { // write tests
      H5File file(FILENAME, H5F_ACC_EXCL);
      auto group = H5Util::createGroupNXS(file, GRP_NAME, "NXentry");
      H5Util::writeScalarDataSetWithStrAttributes(group, DATA_NAME, DATA_VALUE,
                                                  stringAttributesScalar);
      auto data = group.openDataSet(DATA_NAME);
      // Add the float and int attribute
      H5Util::writeNumAttribute(data, ATTR_NAME_3, ATTR_VALUE_3);
      H5Util::writeNumAttribute(data, ATTR_NAME_4, ATTR_VALUE_4);

      // Add the float and int vector attributes
      H5Util::writeNumAttribute(data, ATTR_NAME_5, ATTR_VALUE_5);
      H5Util::writeNumAttribute(data, ATTR_NAME_6, ATTR_VALUE_6);

      file.close();
    }

    // Assert
    TS_ASSERT(Poco::File(FILENAME).exists());
    std::map<std::string, float> floatAttributesScalar{
        {ATTR_NAME_3, ATTR_VALUE_3}};
    std::map<std::string, int> intAttributesScalar{{ATTR_NAME_4, ATTR_VALUE_4}};
    std::map<std::string, std::vector<float>> floatVectorAttributesScalar{
        {ATTR_NAME_5, ATTR_VALUE_5}};
    std::map<std::string, std::vector<int>> intVectorAttributesScalar{
        {ATTR_NAME_6, ATTR_VALUE_6}};
    do_assert_simple_string_data_set(
        FILENAME, GRP_NAME, DATA_NAME, DATA_VALUE, stringAttributesScalar,
        floatAttributesScalar, intAttributesScalar, floatVectorAttributesScalar,
        intVectorAttributesScalar);

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
          const boost::numeric::positive_overflow &);
      TS_ASSERT_THROWS_NOTHING(
          H5Util::readArray1DCoerce<uint32_t>(group, "array1d_int32"));

      file.close();
    }

    // cleanup
    removeFile(FILENAME);
  }

  void test_string_vector() {
    std::vector<std::string> readout;
    const std::string filename = "test_string_vec.h5";
    const std::string dataname = "test_str_vec";
    const hsize_t dims[1] = {5};
    const char *wdata[5] = {"Lets", "see", "how", "it", "goes"};

    // write a test file
    H5File file(filename, H5F_ACC_TRUNC);
    Group group = file.createGroup("entry");
    DataSpace dataspace(1, dims);
    StrType datatype(0, H5T_VARIABLE);
    DataSet dataset = group.createDataSet(dataname, datatype, dataspace);
    dataset.write(wdata, datatype);
    dataset.close();
    group.close();
    file.close();

    // check it exists
    TS_ASSERT(Poco::File(filename).exists());

    // open and read the vector
    H5File file_read(filename, H5F_ACC_RDONLY);
    Group group_read = file_read.openGroup("entry");

    readout = H5Util::readStringVector(group_read, dataname);

    for (size_t i = 0; i < readout.size(); ++i) {
      TS_ASSERT_EQUALS(readout[i], std::string(wdata[i]));
    }

    group_read.close();
    file_read.close();

    // remove the file
    removeFile(filename);
  }

private:
  void do_assert_simple_string_data_set(
      const std::string &filename, const std::string &groupName,
      const std::string &dataName, const std::string &dataValue,
      const std::map<std::string, std::string> &stringAttributes =
          std::map<std::string, std::string>(),
      const std::map<std::string, float> &floatAttributes =
          std::map<std::string, float>(),
      const std::map<std::string, int> &intAttributes =
          std::map<std::string, int>(),
      const std::map<std::string, std::vector<float>> &floatVectorAttributes =
          std::map<std::string, std::vector<float>>(),
      const std::map<std::string, std::vector<int>> &intVectorAttributes =
          std::map<std::string, std::vector<int>>()) {
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
    do_test_attributes_on_data_set(data, stringAttributes, floatAttributes,
                                   intAttributes, floatVectorAttributes,
                                   intVectorAttributes);
    file.close();
  }

  void do_test_attributes_on_data_set(
      H5::DataSet &data,
      const std::map<std::string, std::string> &stringAttributes,
      const std::map<std::string, float> &floatAttributes,
      const std::map<std::string, int> &intAttributes,
      const std::map<std::string, std::vector<float>> &floatVectorAttributes,
      const std::map<std::string, std::vector<int>> &intVectorAttributes) {
    auto numAttributes = data.getNumAttrs();
    auto expectedNumStringAttributes =
        static_cast<int>(stringAttributes.size());
    auto expectedNumFloatAttributes = static_cast<int>(floatAttributes.size());
    auto expectedNumIntAttributes = static_cast<int>(intAttributes.size());
    auto expectedNumFloatVectorAttributes =
        static_cast<int>(floatVectorAttributes.size());
    auto expectedNumIntVectorAttributes =
        static_cast<int>(intVectorAttributes.size());
    int totalNumAttributs =
        expectedNumStringAttributes + expectedNumFloatAttributes +
        expectedNumIntAttributes + expectedNumFloatVectorAttributes +
        expectedNumIntVectorAttributes;
    TSM_ASSERT_EQUALS("There should be two attributes present.",
                      totalNumAttributs, numAttributes);

    for (auto &attribute : stringAttributes) {
      auto value = H5Util::readAttributeAsString(data, attribute.first);
      TSM_ASSERT_EQUALS("Should retrieve the correct attribute value",
                        attribute.second, value);
    }

    for (auto &attribute : floatAttributes) {
      auto value = H5Util::readNumAttributeCoerce<float>(data, attribute.first);
      TSM_ASSERT_EQUALS("Should retrieve the correct attribute value",
                        attribute.second, value);
    }

    for (auto &attribute : intAttributes) {
      auto value = H5Util::readNumAttributeCoerce<int>(data, attribute.first);
      TSM_ASSERT_EQUALS("Should retrieve the correct attribute value",
                        attribute.second, value);
    }

    for (auto &attribute : floatVectorAttributes) {
      std::vector<float> value =
          H5Util::readNumArrayAttributeCoerce<float>(data, attribute.first);
      TSM_ASSERT_EQUALS("Should retrieve the correct attribute value",
                        attribute.second, value);
    }

    for (auto &attribute : intVectorAttributes) {
      std::vector<int> value =
          H5Util::readNumArrayAttributeCoerce<int>(data, attribute.first);
      TSM_ASSERT_EQUALS("Should retrieve the correct attribute value",
                        attribute.second, value);
    }

    // Test that coerced read works
    std::vector<int> expectedCoerced = {12, 34, 455};
    for (auto &attribute : floatVectorAttributes) {
      std::vector<int> value =
          H5Util::readNumArrayAttributeCoerce<int>(data, attribute.first);
      TSM_ASSERT_EQUALS("Should retrieve the correct attribute value",
                        expectedCoerced, value);
    }
  }
};

#endif /* MANTID_DATAHANDLING_H5UTILTEST_H_ */
