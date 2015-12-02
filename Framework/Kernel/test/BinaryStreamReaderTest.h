#ifndef MANTID_KERNEL_BINARYSTREAMREADERTEST_H_
#define MANTID_KERNEL_BINARYSTREAMREADERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/BinaryStreamReader.h"
#include "MantidKernel/ConfigService.h"

#include <Poco/File.h>
#include <Poco/Path.h>

#include <algorithm>
#include <fstream>

using Mantid::Kernel::BinaryStreamReader;

class BinaryFileReaderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BinaryFileReaderTest *createSuite() {
    return new BinaryFileReaderTest();
  }
  static void destroySuite(BinaryFileReaderTest *suite) { delete suite; }

  BinaryFileReaderTest()
      : CxxTest::TestSuite(), m_filename("test_horace_reader.sqw"), m_stream() {
    openTestFile();
  }

  void setUp() { resetStreamToStart(); }

  //----------------------------------------------------------------------------
  // Successes cases
  //----------------------------------------------------------------------------
  void test_Constructor_With_Good_Stream_Does_Not_Touch_Stream() {
    BinaryStreamReader reader(m_stream);
    TS_ASSERT_EQUALS(std::ios_base::beg, m_stream.tellg());
  }

  void test_Read_int32_t_Gives_Correct_Value() {
    doReadSingleValueTest<int32_t>(6, sizeof(int32_t));
  }

  void test_Read_int64_t_Gives_Correct_Value() {
    moveStreamToPosition(677763);
    doReadSingleValueTest<int64_t>(580, sizeof(int64_t));
  }

  void test_Read_float_Gives_Correct_Value() {
    // Move to where a float should be
    moveStreamToPosition(166);
    doReadSingleValueTest<float>(787.0f, sizeof(float));
  }

  void test_Read_double_Gives_Correct_Value() {
    moveStreamToPosition(10);
    doReadSingleValueTest<double>(2.0, sizeof(double));
  }

  void test_Read_String_Gives_Expected_String() {
    const size_t offset = sizeof(int32_t) + 6;
    doReadSingleValueTest<std::string>("horace", offset);
  }

  void test_Read_Vector_int32_t() {
    moveStreamToPosition(466);
    const size_t nvals(2);
    std::vector<int32_t> expectedValue(nvals);
    expectedValue[0] = 4;
    expectedValue[1] = 7;

    doReadArrayValueTest(nvals, expectedValue, nvals * sizeof(int32_t));
  }

  void test_Read_Vector_int64_t() {
    moveStreamToPosition(677427);
    const size_t nvals(36);
    std::vector<int64_t> expectedValue(nvals, 0);
    expectedValue[4] = 1288490188800;
    expectedValue[22] = 386547056640;
    expectedValue[23] = 816043786240;

    doReadArrayValueTest(nvals, expectedValue, nvals * sizeof(int64_t));
  }

  void test_Read_Vector_float() {
    using std::fill;
    moveStreamToPosition(646);
    const size_t nvals(31);
    std::vector<float> expectedValue(nvals, 0.0f);
    float accumulator(0.0f);
    std::generate(begin(expectedValue) + 1, end(expectedValue),
                  [&accumulator]() {
                    accumulator += 5.0f;
                    return accumulator;
                  });

    doReadArrayValueTest(nvals, expectedValue, nvals * sizeof(float));
  }

  void test_Read_Vector_double() {
    using std::fill;
    moveStreamToPosition(10);
    const size_t nvals(1);
    std::vector<double> expectedValue(nvals, 2.0);

    doReadArrayValueTest(nvals, expectedValue, nvals * sizeof(double));
  }

  // Only test this for a single type assuming it is the same for all
  void test_Read_Vector_With_Bigger_Vector_Leaves_Size_Untouched() {
    moveStreamToPosition(466);
    BinaryStreamReader reader(m_stream);
    auto streamPosBeg = m_stream.tellg();

    const size_t nvals(2);
    std::vector<int32_t> values(nvals + 2, 0);
    reader.read(values, nvals);

    std::vector<int32_t> expectedValue(nvals + 2, 0);
    expectedValue[0] = 4;
    expectedValue[1] = 7;
    TS_ASSERT_EQUALS(expectedValue, values);
    TS_ASSERT_EQUALS(nvals + 2, values.size());
    auto expectedStreamOffset = nvals * sizeof(int32_t);
    TS_ASSERT_EQUALS(expectedStreamOffset, m_stream.tellg() - streamPosBeg);
  }

  void test_Read_String_Of_Given_Size() {
    moveStreamToPosition(677015);

    BinaryStreamReader reader(m_stream);
    auto streamPosBeg = m_stream.tellg();
    std::string value;
    const size_t nchars(7);
    reader.read(value, nchars);

    TS_ASSERT_EQUALS(nchars, value.length());
    TS_ASSERT_EQUALS("QQQE___", value);
    TS_ASSERT_EQUALS(nchars, m_stream.tellg() - streamPosBeg);
  }

  //----------------------------------------------------------------------------
  // Failure cases
  //----------------------------------------------------------------------------
  void test_Stream_Marked_Not_Good_Throws_RuntimeError_On_Construction() {
    m_stream.seekg(std::ios_base::end);
    // read will put it into a 'bad' state
    int i(0);
    m_stream >> i;
    TSM_ASSERT_THROWS("Expected a runtime_error when given a bad stream",
                      BinaryStreamReader reader(m_stream), std::runtime_error);
  }

private:
  template <typename T>
  void doReadSingleValueTest(T expectedValue, size_t expectedStreamOffset) {
    BinaryStreamReader reader(m_stream);
    auto streamPosBeg = m_stream.tellg();
    T value;
    reader >> value;
    TS_ASSERT_EQUALS(expectedValue, value);
    TS_ASSERT_EQUALS(expectedStreamOffset, m_stream.tellg() - streamPosBeg);
  }

  template <typename T>
  void doReadArrayValueTest(const size_t nvals,
                            const std::vector<T> &expectedValue,
                            size_t expectedStreamOffset) {
    BinaryStreamReader reader(m_stream);
    auto streamPosBeg = m_stream.tellg();

    std::vector<T> values;
    reader.read(values, nvals);
    TS_ASSERT_EQUALS(expectedValue, values);
    TS_ASSERT_EQUALS(nvals, values.size());
    TS_ASSERT_EQUALS(expectedStreamOffset, m_stream.tellg() - streamPosBeg);
  }

  void openTestFile() {
    using Mantid::Kernel::ConfigService;
    // The test file should be in the data search path
    const auto &dirs = ConfigService::Instance().getDataSearchDirs();
    std::string filepath;
    for (auto direc : dirs) {
      Poco::Path path(direc, m_filename);
      if (Poco::File(path).exists()) {
        filepath = path.toString();
        break;
      }
    }
    if (filepath.empty()) {
      throw std::runtime_error(
          "Unable to find test file. Check data search paths");
    } else {
      m_stream.open(filepath, std::ios_base::binary);
      if (!m_stream) {
        throw std::runtime_error(
            "Cannot open test file. Check file permissions.");
      }
    }
  }

  void resetStreamToStart() {
    m_stream.clear();
    m_stream.seekg(std::ios_base::beg);
  }

  /// Move the stream nbytes from the beginning
  void moveStreamToPosition(size_t nbytes) {
    m_stream.seekg(nbytes, std::ios_base::beg);
  }

  std::string m_filename;
  std::ifstream m_stream;
};

#endif /* MANTID_KERNEL_BINARYSTREAMREADERTEST_H_ */
