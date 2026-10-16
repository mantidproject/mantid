// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidNexus/H5ValidatingAdapter.h"
#include "MantidNexus/NexusException.h"
#include "test_helper.h"

#include <H5Cpp.h>
#include <cxxtest/TestSuite.h>
#include <limits>

class H5ValidatingAdapterTest : public CxxTest::TestSuite {
private:
  Mantid::Nexus::H5ValidatingAdapter::Context context(const std::string &operation) const {
    return {operation, "H5ValidatingAdapterTest.h5", "/entry/value"};
  }

public:
  void test_invalid_handle_throws_contextual_nexus_exception() {
    try {
      Mantid::Nexus::H5ValidatingAdapter::checkId(-1, context("open dataset"));
      TS_FAIL("Expected invalid HDF5 handle to throw");
    } catch (const Mantid::Nexus::Exception &error) {
      TS_ASSERT_EQUALS(error.functionname(), "open dataset");
      TS_ASSERT_EQUALS(error.filename(), "H5ValidatingAdapterTest.h5");
      TS_ASSERT(std::string(error.what()).find("/entry/value") != std::string::npos);
    }
  }

  void test_failed_status_and_tristate_throw() {
    TS_ASSERT_THROWS(Mantid::Nexus::H5ValidatingAdapter::checkStatus(-1, context("read dataset")),
                     const Mantid::Nexus::Exception &);
    TS_ASSERT_THROWS(Mantid::Nexus::H5ValidatingAdapter::checkTriState(-1, context("query datatype")),
                     const Mantid::Nexus::Exception &);
  }

  void test_null_scalar_variable_length_string_is_distinct_from_failure() {
    NexusTest::FileResource resource("H5ValidatingAdapter_null_scalar.h5");
    H5::H5File file(resource.fullPath(), H5F_ACC_TRUNC);
    H5::StrType type(H5::PredType::C_S1, H5T_VARIABLE);
    H5::DataSpace space(H5S_SCALAR);
    H5::DataSet dataset = file.createDataSet("value", type, space);

    const auto value = Mantid::Nexus::H5ValidatingAdapter::readScalarVlenDataset(
        dataset.getId(), type.getId(), space.getId(), context("read scalar string"));
    TS_ASSERT(!value.has_value());
  }

  void test_variable_length_string_vector_preserves_null_elements() {
    NexusTest::FileResource resource("H5ValidatingAdapter_null_vector.h5");
    H5::H5File file(resource.fullPath(), H5F_ACC_TRUNC);
    H5::StrType type(H5::PredType::C_S1, H5T_VARIABLE);
    const hsize_t dims[1]{2};
    H5::DataSpace space(1, dims);
    H5::DataSet dataset = file.createDataSet("value", type, space);
    const char *input[2]{"present", nullptr};
    dataset.write(input, type);

    const auto values = Mantid::Nexus::H5ValidatingAdapter::readVlenDataset(
        dataset.getId(), type.getId(), space.getId(), context("read string vector"));
    TS_ASSERT_EQUALS(values.size(), 2);
    TS_ASSERT_EQUALS(values[0].value(), "present");
    TS_ASSERT(!values[1].has_value());
  }

  void test_null_scalar_variable_length_attribute_is_distinct_from_failure() {
    NexusTest::FileResource resource("H5ValidatingAdapter_null_attribute.h5");
    H5::H5File file(resource.fullPath(), H5F_ACC_TRUNC);
    H5::StrType type(H5::PredType::C_S1, H5T_VARIABLE);
    H5::DataSpace space(H5S_SCALAR);
    H5::Attribute attribute = file.createAttribute("value", type, space);

    const auto value = Mantid::Nexus::H5ValidatingAdapter::readScalarVlenAttribute(attribute.getId(), type.getId(),
                                                                                   context("read scalar attribute"));
    TS_ASSERT(!value.has_value());
  }

  void test_dimension_product_overflow_throws() {
    const Mantid::Nexus::DimVector dimensions{std::numeric_limits<Mantid::Nexus::dimsize_t>::max(), 2};
    TS_ASSERT_THROWS(Mantid::Nexus::H5ValidatingAdapter::elementCount(dimensions, context("size dataset")),
                     const Mantid::Nexus::Exception &);
  }

  void test_dimensions_reject_invalid_dataspace() {
    TS_ASSERT_THROWS(Mantid::Nexus::H5ValidatingAdapter::dimensions(-1, context("read dimensions")),
                     const Mantid::Nexus::Exception &);
  }

  void test_hyperslab_outside_extent_throws() {
    const hsize_t dims[2]{2, 3};
    H5::DataSpace space(2, dims);
    const Mantid::Nexus::DimVector start{1, 2};
    const Mantid::Nexus::DimVector size{2, 1};
    TS_ASSERT_THROWS(
        Mantid::Nexus::H5ValidatingAdapter::selectHyperslab(space.getId(), start, size, context("select hyperslab")),
        const Mantid::Nexus::Exception &);
  }
};
