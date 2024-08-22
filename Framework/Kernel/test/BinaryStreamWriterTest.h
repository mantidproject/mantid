// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <cxxtest/ValueTraits.h>

#include "MantidKernel/BinaryStreamReader.h"
#include "MantidKernel/BinaryStreamWriter.h"
#include "MantidKernel/Matrix.h"

#include <sstream>

using Mantid::Kernel::BinaryStreamReader;
using Mantid::Kernel::BinaryStreamWriter;
using Mantid::Kernel::Matrix;

class BinaryStreamWriterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BinaryStreamWriterTest *createSuite() { return new BinaryStreamWriterTest(); }
  static void destroySuite(BinaryStreamWriterTest *suite) { delete suite; }

  BinaryStreamWriterTest() : CxxTest::TestSuite(), m_bytes() {}

  //----------------------------------------------------------------------------
  // Successes cases
  //----------------------------------------------------------------------------

  // single values
  void test_Constructor_With_Good_Stream_Does_Not_Touch_Stream() {
    BinaryStreamReader reader(m_bytes);
    TS_ASSERT_EQUALS(std::streampos(0), m_bytes.tellg());
  }

  void test_Write_int16_t_Gives_Correct_Value() { doWriteSingleValueTest<int16_t>(6); }

  void test_Write_int32_t_Gives_Correct_Value() { doWriteSingleValueTest<int32_t>(580); }

  void test_Write_int64_t_Gives_Correct_Value() {
    ;
    doWriteSingleValueTest<int64_t>(200);
  }

  void test_Write_uint16_t_Gives_Correct_Value() { doWriteSingleValueTest<uint16_t>(111); }

  void test_Write_uint32_t_Gives_Correct_Value() { doWriteSingleValueTest<uint32_t>(231); }

  void test_Write_float_Gives_Correct_Value() { doWriteSingleValueTest<float>(787.0f); }

  void test_Write_double_Gives_Correct_Value() { doWriteSingleValueTest<double>(2.0); }

  void test_Write_String_Gives_Expected_String() { doWriteSingleValueTest<std::string>("mantid"); }

  // vectors of values
  void test_Write_Vector_int16_t() {
    const size_t nvals(3);
    std::vector<int16_t> expectedValue{2, 0, 4};
    doWriteArrayValueTest(nvals, expectedValue);
  }

  void test_Write_Vector_int32_t() {
    const size_t nvals(3);
    std::vector<int32_t> expectedValue{2, 4, 6};
    doWriteArrayValueTest(nvals, expectedValue);
  }

  void test_Write_Vector_int64_t() {
    std::vector<int64_t> expectedValue{200, 400, 600, 900};
    doWriteArrayValueTest(expectedValue.size(), expectedValue);
  }

  void test_Write_Vector_float() {
    std::vector<float> expectedValue{0.0f, 5.0f, 10.0f};
    doWriteArrayValueTest(expectedValue.size(), expectedValue);
  }

  void test_Write_Vector_double() {
    std::vector<double> expectedValue{10.0, 15.0, 20.0, 25.0};
    doWriteArrayValueTest(expectedValue.size(), expectedValue);
  }

private:
  template <typename T> void doWriteSingleValueTest(T value) {
    BinaryStreamWriter writer(m_bytes);
    BinaryStreamReader reader(m_bytes);
    T readVal;
    writer << value;
    reader >> readVal;
    TS_ASSERT_EQUALS(readVal, value);
  }

  template <typename T> void doWriteArrayValueTest(const size_t nvals, const std::vector<T> &values) {
    BinaryStreamWriter writer(m_bytes);
    BinaryStreamReader reader(m_bytes);
    std::vector<T> readValues;
    writer.write(values, nvals);
    reader.read(readValues, nvals);
    TS_ASSERT_EQUALS(readValues, values);
    TS_ASSERT_EQUALS(nvals, values.size());
  }

  void resetStreamToStart() {
    m_bytes.clear();
    m_bytes.seekg(std::ios_base::beg);
  }

  std::stringstream m_bytes;
};