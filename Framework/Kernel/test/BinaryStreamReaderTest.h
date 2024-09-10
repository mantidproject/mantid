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
#include "MantidKernel/Matrix.h"

#include <sstream>

using Mantid::Kernel::BinaryStreamReader;
using Mantid::Kernel::Matrix;

class BinaryStreamReaderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BinaryStreamReaderTest *createSuite() { return new BinaryStreamReaderTest(); }
  static void destroySuite(BinaryStreamReaderTest *suite) { delete suite; }

  BinaryStreamReaderTest() : CxxTest::TestSuite(), m_bytes() { createTestStream(); }

  void setUp() override { resetStreamToStart(); }

  //----------------------------------------------------------------------------
  // Successes cases
  //----------------------------------------------------------------------------
  void test_Constructor_With_Good_Stream_Does_Not_Touch_Stream() {
    BinaryStreamReader reader(m_bytes);
    TS_ASSERT_EQUALS(std::streampos(0), m_bytes.tellg());
  }

  void test_Read_int16_t_Gives_Correct_Value() { doReadSingleValueTest<int16_t>(6, sizeof(int16_t)); }

  void test_Read_int32_t_Gives_Correct_Value() {
    moveStreamToPosition(10);
    doReadSingleValueTest<int32_t>(580, sizeof(int32_t));
  }

  void test_Read_int64_t_Gives_Correct_Value() {
    moveStreamToPosition(42);
    doReadSingleValueTest<int64_t>(200, sizeof(int64_t));
  }

  void test_Read_float_Gives_Correct_Value() {
    // Move to where a float should be
    moveStreamToPosition(18);
    doReadSingleValueTest<float>(787.0f, sizeof(float));
  }

  void test_Read_double_Gives_Correct_Value() {
    moveStreamToPosition(22);
    doReadSingleValueTest<double>(2.0, sizeof(double));
  }

  void test_Read_String_Gives_Expected_String() {
    const size_t offset = sizeof(int32_t) + 6;
    doReadSingleValueTest<std::string>("mantid", offset);
  }

  void test_Read_Vector_int16_t() {
    moveStreamToPosition(30);
    const size_t nvals(3);
    std::vector<int16_t> expectedValue{2, 0, 4};
    doReadArrayValueTest(nvals, expectedValue, nvals * sizeof(int16_t));
  }

  void test_Read_Vector_int32_t() {
    moveStreamToPosition(30);
    const size_t nvals(3);
    std::vector<int32_t> expectedValue{2, 4, 6};
    doReadArrayValueTest(nvals, expectedValue, nvals * sizeof(int32_t));
  }

  void test_Read_Vector_int64_t() {
    moveStreamToPosition(42);
    std::vector<int64_t> expectedValue{200, 400, 600, 900};
    const auto nvals(expectedValue.size());
    doReadArrayValueTest(expectedValue.size(), expectedValue, nvals * sizeof(int64_t));
  }

  void test_Read_Vector_float() {
    moveStreamToPosition(74);
    std::vector<float> expectedValue{0.0f, 5.0f, 10.0f};
    const auto nvals(expectedValue.size());
    doReadArrayValueTest(nvals, expectedValue, nvals * sizeof(float));
  }

  void test_Read_Vector_double() {
    moveStreamToPosition(86);
    std::vector<double> expectedValue{10.0, 15.0, 20.0, 25.0};
    const auto nvals(expectedValue.size());
    doReadArrayValueTest(nvals, expectedValue, nvals * sizeof(double));
  }

  void test_Read_Vector_String_In_Row_Major_Order() {
    moveStreamToPosition(118);

    BinaryStreamReader reader(m_bytes);
    std::vector<std::string> expected{"abc", "def"}, value;
    std::vector<int32_t> shape{2, 3};
    reader.read(value, shape, BinaryStreamReader::MatrixOrdering::RowMajor);
    TS_ASSERT_EQUALS(expected, value);
  }

  void test_Read_Vector_String_In_Column_Major_Order() {
    moveStreamToPosition(118);

    BinaryStreamReader reader(m_bytes);
    std::vector<std::string> expected{"ace", "bdf"}, value;
    std::vector<int32_t> shape{2, 3};
    reader.read(value, shape, BinaryStreamReader::MatrixOrdering::ColumnMajor);
    TS_ASSERT_EQUALS(expected, value);
  }

  void test_Read_Matrix_Float_In_RowMajor_Order() {
    moveStreamToPosition(124);

    BinaryStreamReader reader(m_bytes);
    std::vector<int32_t> shape{2, 3};
    Matrix<float> expected(shape[0], shape[1]);
    expected[0][0] = 1.0f;
    expected[0][1] = 2.0f;
    expected[0][2] = 3.0f;
    expected[1][0] = 4.0f;
    expected[1][1] = 5.0f;
    expected[1][2] = 6.0f;

    Matrix<float> value;
    reader.read(value, shape, BinaryStreamReader::MatrixOrdering::RowMajor);
    TS_ASSERT_EQUALS(expected, value);
  }

  void test_Read_Matrix_Float_In_ColumnMajor_Order() {
    moveStreamToPosition(124);

    BinaryStreamReader reader(m_bytes);
    std::vector<int32_t> shape{2, 3};
    Matrix<float> expected(shape[0], shape[1]);
    expected[0][0] = 1.0f;
    expected[0][1] = 3.0f;
    expected[0][2] = 5.0f;
    expected[1][0] = 2.0f;
    expected[1][1] = 4.0f;
    expected[1][2] = 6.0f;

    Matrix<float> value;
    reader.read(value, shape, BinaryStreamReader::MatrixOrdering::ColumnMajor);
    TS_ASSERT_EQUALS(expected, value);
  }

  void test_Read_Matrix_Double_In_RowMajor_Order() {
    moveStreamToPosition(148);

    BinaryStreamReader reader(m_bytes);
    std::vector<int32_t> shape{2, 3};
    Matrix<double> expected(shape[0], shape[1]);
    expected[0][0] = 1.0;
    expected[0][1] = 2.0;
    expected[0][2] = 3.0;
    expected[1][0] = 4.0;
    expected[1][1] = 5.0;
    expected[1][2] = 6.0;

    Matrix<double> value;
    reader.read(value, shape, BinaryStreamReader::MatrixOrdering::RowMajor);
    TS_ASSERT_EQUALS(expected, value);
  }

  void test_Read_Matrix_Double_In_ColumnMajor_Order() {
    moveStreamToPosition(148);

    BinaryStreamReader reader(m_bytes);
    std::vector<int32_t> shape{2, 3};
    Matrix<double> expected(shape[0], shape[1]);
    expected[0][0] = 1.0;
    expected[0][1] = 3.0;
    expected[0][2] = 5.0;
    expected[1][0] = 2.0;
    expected[1][1] = 4.0;
    expected[1][2] = 6.0;

    Matrix<double> value;
    reader.read(value, shape, BinaryStreamReader::MatrixOrdering::ColumnMajor);
    TS_ASSERT_EQUALS(expected, value);
  }

  // Only test this for a single type assuming it is the same for all
  void test_Read_Vector_With_Bigger_Vector_Leaves_Size_Untouched() {
    moveStreamToPosition(30);
    BinaryStreamReader reader(m_bytes);
    auto streamPosBeg = m_bytes.tellg();

    const size_t nvals(3);
    std::vector<int32_t> values(nvals + 2, 0);
    reader.read(values, nvals);

    std::vector<int32_t> expectedValue{2, 4, 6, 0, 0};
    TS_ASSERT_EQUALS(expectedValue, values);
    TS_ASSERT_EQUALS(nvals + 2, values.size());
    auto expectedStreamOffset = nvals * sizeof(int32_t);
    TS_ASSERT_EQUALS(expectedStreamOffset, m_bytes.tellg() - streamPosBeg);
  }

  void test_Read_String_Of_Given_Size() {
    moveStreamToPosition(4);

    BinaryStreamReader reader(m_bytes);
    auto streamPosBeg = m_bytes.tellg();
    std::string value;
    const size_t nchars(3);
    reader.read(value, nchars);

    TS_ASSERT_EQUALS(nchars, value.length());
    TS_ASSERT_EQUALS("man", value);
    TS_ASSERT_EQUALS(nchars, m_bytes.tellg() - streamPosBeg);
  }

  //----------------------------------------------------------------------------
  // Failure cases
  //----------------------------------------------------------------------------
  void test_Stream_Marked_Not_Good_Throws_RuntimeError_On_Construction() {
    m_bytes.seekg(std::ios_base::end);
    // read will put it into a 'bad' state
    int i(0);
    m_bytes >> i;
    TSM_ASSERT_THROWS("Expected a runtime_error when given a bad stream", BinaryStreamReader reader(m_bytes),
                      const std::runtime_error &);
  }

private:
  template <typename T> void doReadSingleValueTest(T expectedValue, size_t expectedStreamOffset) {
    BinaryStreamReader reader(m_bytes);
    auto streamPosBeg = m_bytes.tellg();
    T value;
    reader >> value;
    TS_ASSERT_EQUALS(expectedValue, value);
    TS_ASSERT_EQUALS(expectedStreamOffset, m_bytes.tellg() - streamPosBeg);
  }

  template <typename T>
  void doReadArrayValueTest(const size_t nvals, const std::vector<T> &expectedValue, size_t expectedStreamOffset) {
    BinaryStreamReader reader(m_bytes);
    auto streamPosBeg = m_bytes.tellg();

    std::vector<T> values;
    reader.read(values, nvals);
    TS_ASSERT_EQUALS(expectedValue, values);
    TS_ASSERT_EQUALS(nvals, values.size());
    TS_ASSERT_EQUALS(expectedStreamOffset, m_bytes.tellg() - streamPosBeg);
  }

  void createTestStream() {
    // int32_t + series of characters
    int32_t length(6);
    writeSingleValueToStream<int32_t>(m_bytes, length);
    m_bytes.write("mantid", length);
    // single int64_t
    writeSingleValueToStream<int64_t>(m_bytes, 580);
    // single float
    writeSingleValueToStream<float>(m_bytes, 787.0f);
    // single double
    writeSingleValueToStream<double>(m_bytes, 2.0);
    // vector int32_t
    writeArrayValuesToStream<int32_t>(m_bytes, {2, 4, 6});
    // vector int64_t
    writeArrayValuesToStream<int64_t>(m_bytes, {200, 400, 600, 900});
    // vector float
    writeArrayValuesToStream<float>(m_bytes, {0.0f, 5.0f, 10.0f});
    // vector double
    writeArrayValuesToStream<double>(m_bytes, {10.0, 15.0, 20.0, 25.0});
    // array of characters
    m_bytes.write("abcdef", 6);
    // matrix of floats
    writeArrayValuesToStream<float>(m_bytes, {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f});
    // matrix of doubles
    writeArrayValuesToStream<double>(m_bytes, {1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
  }

  template <typename T> void writeSingleValueToStream(std::ostream &, T value) {
    m_bytes.write(reinterpret_cast<const char *>(&value), sizeof(T));
  }

  template <typename T> void writeArrayValuesToStream(std::ostream &, std::initializer_list<T> values) {
    auto length = values.size();
    std::vector<T> asVector(values);
    m_bytes.write(reinterpret_cast<const char *>(asVector.data()), length * sizeof(T));
  }

  void resetStreamToStart() {
    m_bytes.clear();
    m_bytes.seekg(std::ios_base::beg);
  }

  /// Move the stream nbytes from the beginning
  void moveStreamToPosition(size_t nbytes) { m_bytes.seekg(nbytes, std::ios_base::beg); }

  std::stringstream m_bytes;
};
