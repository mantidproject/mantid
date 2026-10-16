// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "H5ErrorHandling.h"

#include <cxxtest/TestSuite.h>
#include <filesystem>
#include <limits>

class LegacyH5ErrorHandlingTest : public CxxTest::TestSuite {
private:
  Mantid::LegacyNexus::H5ErrorHandling::Context context(const std::string &operation) const {
    return {operation, "/entry/value"};
  }

public:
  void test_failed_hdf5_results_are_reported_as_failure() {
    bool result = true;
    TS_ASSERT(!Mantid::LegacyNexus::H5ErrorHandling::checkId(-1, context("open dataset")));
    TS_ASSERT(!Mantid::LegacyNexus::H5ErrorHandling::checkStatus(-1, context("read dataset")));
    TS_ASSERT(!Mantid::LegacyNexus::H5ErrorHandling::checkTriState(-1, result, context("query datatype")));
  }

  void test_dimension_product_overflow_is_reported_as_failure() {
    const std::vector<hsize_t> dimensions{std::numeric_limits<hsize_t>::max(), 2};
    std::size_t count = 0;
    TS_ASSERT(!Mantid::LegacyNexus::H5ErrorHandling::elementCount(dimensions, count, context("size dataset")));
  }

  void test_invalid_hyperslab_is_rejected_before_hdf5_access() {
    const hsize_t dimensions[2]{2, 3};
    const hid_t dataspace = H5Screate_simple(2, dimensions, nullptr);
    const std::vector<hsize_t> start{1, 2};
    const std::vector<hsize_t> size{2, 1};
    TS_ASSERT(
        !Mantid::LegacyNexus::H5ErrorHandling::selectHyperslab(dataspace, start, size, context("select hyperslab")));
    H5Sclose(dataspace);
  }

  void test_unwritten_scalar_variable_length_dataset_is_read_as_empty() {
    const std::string filename{"LegacyH5ErrorHandling_null_dataset.h5"};
    std::filesystem::remove(filename);
    const hid_t file = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    const hid_t datatype = H5Tcopy(H5T_C_S1);
    H5Tset_size(datatype, H5T_VARIABLE);
    const hid_t dataspace = H5Screate(H5S_SCALAR);
    const hid_t dataset = H5Dcreate2(file, "value", datatype, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    std::string value{"not empty"};
    TS_ASSERT(Mantid::LegacyNexus::H5ErrorHandling::readScalarVlenDataset(dataset, datatype, dataspace, value,
                                                                          context("read scalar string")));
    TS_ASSERT(value.empty());

    H5Dclose(dataset);
    H5Sclose(dataspace);
    H5Tclose(datatype);
    H5Fclose(file);
    std::filesystem::remove(filename);
  }

  void test_unwritten_scalar_variable_length_attribute_is_read_as_empty() {
    const std::string filename{"LegacyH5ErrorHandling_null_attribute.h5"};
    std::filesystem::remove(filename);
    const hid_t file = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    const hid_t datatype = H5Tcopy(H5T_C_S1);
    H5Tset_size(datatype, H5T_VARIABLE);
    const hid_t dataspace = H5Screate(H5S_SCALAR);
    const hid_t attribute = H5Acreate2(file, "value", datatype, dataspace, H5P_DEFAULT, H5P_DEFAULT);

    std::string value{"not empty"};
    TS_ASSERT(Mantid::LegacyNexus::H5ErrorHandling::readScalarVlenAttribute(attribute, datatype, value,
                                                                            context("read scalar attribute")));
    TS_ASSERT(value.empty());

    H5Aclose(attribute);
    H5Sclose(dataspace);
    H5Tclose(datatype);
    H5Fclose(file);
    std::filesystem::remove(filename);
  }

  void test_variable_length_attribute_array_preserves_null_elements() {
    const std::string filename{"LegacyH5ErrorHandling_null_attribute_array.h5"};
    std::filesystem::remove(filename);
    const hid_t file = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    const hid_t datatype = H5Tcopy(H5T_C_S1);
    H5Tset_size(datatype, H5T_VARIABLE);
    const hsize_t dimensions[1]{2};
    const hid_t dataspace = H5Screate_simple(1, dimensions, nullptr);
    const hid_t attribute = H5Acreate2(file, "value", datatype, dataspace, H5P_DEFAULT, H5P_DEFAULT);
    const char *input[2]{"present", nullptr};
    H5Awrite(attribute, datatype, input);

    std::vector<std::string> value;
    TS_ASSERT(Mantid::LegacyNexus::H5ErrorHandling::readVlenAttribute(attribute, datatype, dataspace, value,
                                                                      context("read string attribute array")));
    TS_ASSERT_EQUALS(value.size(), 2);
    TS_ASSERT_EQUALS(value[0], "present");
    TS_ASSERT(value[1].empty());

    H5Aclose(attribute);
    H5Sclose(dataspace);
    H5Tclose(datatype);
    H5Fclose(file);
    std::filesystem::remove(filename);
  }
};
