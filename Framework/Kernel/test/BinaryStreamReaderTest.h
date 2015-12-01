#ifndef MANTID_KERNEL_BINARYSTREAMREADERTEST_H_
#define MANTID_KERNEL_BINARYSTREAMREADERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/BinaryStreamReader.h"
#include "MantidKernel/ConfigService.h"

#include <Poco/File.h>
#include <Poco/Path.h>

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
    doReadTest<int32_t>(6, sizeof(int32_t));
  }

  void test_Read_int64_t_Gives_Correct_Value() {
    moveStreamToPosition(677763);
    doReadTest<int64_t>(580, sizeof(int64_t));
  }

  void test_Read_float_Gives_Correct_Value() {
    // Move to where a float should be
    moveStreamToPosition(166);
    doReadTest<float>(787.0f, sizeof(float));
  }

  void test_Read_double_Gives_Correct_Value() {
    moveStreamToPosition(10);
    doReadTest<double>(2.0, sizeof(double));
  }

  void test_Read_String_Gives_Expected_String() {
    const size_t offset = sizeof(int32_t) + 6;
    doReadTest<std::string>("horace", offset);
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
  void doReadTest(T expectedValue, size_t expectedStreamOffset) {
    BinaryStreamReader reader(m_stream);
    auto streamPosBeg = m_stream.tellg();
    T value;
    reader >> value;
    TS_ASSERT_EQUALS(expectedValue, value);
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
