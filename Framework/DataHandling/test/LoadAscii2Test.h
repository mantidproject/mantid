#ifndef LOADASCIITEST_H_
#define LOADASCIITEST_H_

#include "cxxtest/TestSuite.h"
#include "MantidDataHandling/LoadAscii2.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/SaveAscii2.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Unit.h"

#include <Poco/File.h>

#include <fstream>

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using Mantid::HistogramData::HistogramDx;

class LoadAscii2Test : public CxxTest::TestSuite {
public:
  static LoadAscii2Test *createSuite() { return new LoadAscii2Test(); }
  static void destroySuite(LoadAscii2Test *suite) { delete suite; }

  LoadAscii2Test() {
    m_filename = "LoadAscii2Test";
    m_ext = ".txt";
    m_testno = 0;
  }

  ~LoadAscii2Test() override {}

  void testProperties() {
    m_testno++;
    LoadAscii2 testLoad;
    TS_ASSERT_EQUALS("LoadAscii", testLoad.name());
    TS_ASSERT_EQUALS(2, testLoad.version());
    TS_ASSERT_EQUALS("DataHandling\\Text", testLoad.category());
  }
  // the Poco::File.remove() is always in a TS_ASERT as i need to make sure the
  // loader has released the file.
  void testConfidence() {
    m_testno++;
    LoadAscii2 testLoad;
    testLoad.initialize();
    m_abspath = writeTestFile(3);
    // descriptor keeps an open handle until destructor call, so the destructor
    // must run before i can remove it
    auto *descriptor = new FileDescriptor(m_abspath);
    TS_ASSERT_EQUALS(10, testLoad.confidence(*descriptor));
    delete descriptor;
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_Three_Column_Example_With_No_Header() {
    m_testno++;
    m_abspath = writeTestFile(3, false);
    runTest(3);
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_Three_Column_With_Header_Info() {
    m_testno++;
    m_abspath = writeTestFile(3);
    runTest(3);
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_Two_Column_Example_With_No_Header() {
    m_testno++;
    m_abspath = writeTestFile(2, false);
    runTest(2);
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_Two_Column_With_Header_Info() {
    m_testno++;
    m_abspath = writeTestFile(2);
    runTest(2);
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_Four_Column_Example_With_No_Header() {
    m_testno++;
    m_abspath = writeTestFile(4, false);
    runTest(4);
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_Four_Column_Example_With_HeaderInfo() {
    m_testno++;
    m_abspath = writeTestFile(4);
    runTest(4);
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_Four_Column_With_HeaderInfo_CommentChange() {
    m_testno++;
    m_abspath = writeTestFile(4, true, "~");
    runTest(4, false, "~");
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_Four_Column_With_HeaderInfo_NonScientific() {
    m_testno++;
    m_abspath = writeTestFile(4, true, "#", false, 7);
    runTest(4);
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_Four_Column_With_Different_Separator() {
    m_testno++;
    m_abspath = writeTestFile(4, true, "#", true, 6, "Space");
    runTest(4, true, "#", "Space");
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_Custom_Separators() {
    m_testno++;
    m_abspath = writeTestFile(4, true, "#", true, 6, "UserDefined", "~");
    runTest(4, false, "#", "UserDefined", false, "~");
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_Spacing_Around_Separators() {
    m_testno++;
    m_abspath = writeTestFile(4, true, "#", true, 6, "UserDefined",
                              " , "); // space comma space
    // this should work as the load will look for commas and strip out excess
    // spaces
    runTest(4);
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_Double_Spacing_Separators() {
    m_testno++;
    m_abspath = writeTestFile(4, true, "#", true, 6, "UserDefined",
                              "  "); // double space
    // this should work as the load will strip out excess spaces
    runTest(4, true, "#", "Space");
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_fail_five_columns() {
    m_testno++;
    std::ofstream file(getTestFileName().c_str());
    m_abspath = getAbsPath();
    file << std::scientific;
    file << "# X , Y, E, DX, Z\n";
    for (int i = 0; i < 5; i++) {
      file << i << '\n';
      for (int j = 0; j < 4; j++) {
        file << 1.5 * j / 0.9 << "," << (i + 1) * (2. + 4. * (1.5 * j / 0.9))
             << "," << 1 << "," << 0 << ","
             << (i + 5) * (6. + 3. * (1.7 * j / 0.8)) << '\n';
      }
    }
    file.unsetf(std::ios_base::floatfield);
    file.close();
    runTest(4, false, "#", "CSV", true); // cols doesn't matter here
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_fail_one_column() {
    m_testno++;
    std::ofstream file(getTestFileName().c_str());
    m_abspath = getAbsPath();
    file << std::scientific;
    file << "# X\n";
    for (int i = 0; i < 5; i++) {
      file << i << '\n';
      for (int j = 0; j < 4; j++) {
        file << 1.5 * j / 0.9 << '\n';
      }
    }
    file.unsetf(std::ios_base::floatfield);
    file.close();
    runTest(1, false, "#", "CSV", true); // cols doesn't matter here
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_fail_mismatching_bins() {
    m_testno++;
    std::ofstream file(getTestFileName().c_str());
    m_abspath = getAbsPath();
    file << std::scientific;
    file << "# X , Y, E, DX\n";
    for (int i = 0; i < 5; i++) {
      file << i << '\n';
      for (int j = 0; j < 4; j++) {
        if (!(i == 3 && j == 2)) {
          file << 1.5 * j / 0.9 << "," << (i + 1) * (2. + 4. * (1.5 * j / 0.9))
               << "," << 1 << "," << 0 << '\n';
        }
      }
    }
    file.unsetf(std::ios_base::floatfield);
    file.close();
    runTest(4, false, "#", "CSV", true); // cols doesn't matter here
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_fail_mismatching_columns() {
    m_testno++;
    std::ofstream file(getTestFileName().c_str());
    m_abspath = getAbsPath();
    file << std::scientific;
    file << "# X , Y, E, DX\n";
    for (int i = 0; i < 5; i++) {
      file << i << '\n';
      for (int j = 0; j < 4; j++) {
        if (!(i == 3 && j == 2)) {
          file << 1.5 * j / 0.9 << "," << (i + 1) * (2. + 4. * (1.5 * j / 0.9))
               << "," << 1 << "," << 0 << '\n';
        } else {
          file << 1.5 * j / 0.9 << "," << (i + 1) * (2. + 4. * (1.5 * j / 0.9))
               << "," << 1 << '\n';
        }
      }
    }
    file.unsetf(std::ios_base::floatfield);
    file.close();
    runTest(4, false, "#", "CSV", true); // cols doesn't matter here
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_fail_line_start_letter() {
    m_testno++;
    std::ofstream file(getTestFileName().c_str());
    m_abspath = getAbsPath();
    file << std::scientific;
    file << "# X , Y, E, DX\n";
    for (int i = 0; i < 5; i++) {
      file << i << '\n';
      for (int j = 0; j < 4; j++) {
        if (!(i == 3 && j == 2)) {
          file << 1.5 * j / 0.9 << "," << (i + 1) * (2. + 4. * (1.5 * j / 0.9))
               << "," << 1 << "," << 0 << '\n';
        } else {
          // used e to make sure it'd not get mistaken for a scientific index
          file << "e" << 1.5 * j / 0.9 << ","
               << (i + 1) * (2. + 4. * (1.5 * j / 0.9)) << "," << 1 << "," << 0
               << '\n';
        }
      }
    }
    file.unsetf(std::ios_base::floatfield);
    file.close();
    runTest(4, false, "#", "CSV", true); // cols doesn't matter here
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_fail_line_start_noncomment_symbol() {
    m_testno++;
    std::ofstream file(getTestFileName().c_str());
    m_abspath = getAbsPath();
    file << std::scientific;
    file << "# X , Y, E, DX\n";
    for (int i = 0; i < 5; i++) {
      file << i << '\n';
      for (int j = 0; j < 4; j++) {
        if (!(i == 3 && j == 2)) {
          file << 1.5 * j / 0.9 << "," << (i + 1) * (2. + 4. * (1.5 * j / 0.9))
               << "," << 1 << "," << 0 << '\n';
        } else {
          file << "@" << 1.5 * j / 0.9 << ","
               << (i + 1) * (2. + 4. * (1.5 * j / 0.9)) << "," << 1 << "," << 0
               << '\n';
        }
      }
    }
    file.unsetf(std::ios_base::floatfield);
    file.close();
    runTest(4, false, "#", "CSV", true); // cols doesn't matter here
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_fail_line_mixed_letter_number() {
    m_testno++;
    std::ofstream file(getTestFileName().c_str());
    m_abspath = getAbsPath();
    file << std::scientific;
    file << "# X , Y, E, DX\n";
    for (int i = 0; i < 5; i++) {
      file << i << '\n';
      for (int j = 0; j < 4; j++) {
        if (!(i == 3 && j == 2)) {
          file << 1.5 * j / 0.9 << "," << (i + 1) * (2. + 4. * (1.5 * j / 0.9))
               << "," << 1 << "," << 0 << '\n';
        } else {
          // used e to make sure it'd not get mistaken for a scientific index
          file << 1.5 * j / 0.9 << "," << (i + 1) * (2. + 4. * (1.5 * j / 0.9))
               << "e"
               << "," << 1 << "," << 0 << '\n';
        }
      }
    }
    file.unsetf(std::ios_base::floatfield);
    file.close();
    runTest(4, false, "#", "CSV", true); // cols doesn't matter here
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_fail_line_mixed_symbol_number() {
    m_testno++;
    std::ofstream file(getTestFileName().c_str());
    m_abspath = getAbsPath();
    file << std::scientific;
    file << "# X , Y, E, DX\n";
    for (int i = 0; i < 5; i++) {
      file << i << '\n';
      for (int j = 0; j < 4; j++) {
        if (!(i == 3 && j == 2)) {
          file << 1.5 * j / 0.9 << "," << (i + 1) * (2. + 4. * (1.5 * j / 0.9))
               << "," << 1 << "," << 0 << '\n';
        } else {
          file << 1.5 * j / 0.9 << "," << (i + 1) * (2. + 4. * (1.5 * j / 0.9))
               << "/"
               << "," << 1 << "," << 0 << '\n';
        }
      }
    }
    file.unsetf(std::ios_base::floatfield);
    file.close();
    runTest(4, false, "#", "CSV", true); // cols doesn't matter here
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

  void test_fail_spectra_ID_inclusion_inconisitant() {
    m_testno++;
    std::ofstream file(getTestFileName().c_str());
    m_abspath = getAbsPath();

    file << std::scientific;
    file << "# X , Y, E, DX\n";
    for (int i = 0; i < 5; i++) {
      if (i != 3) {
        file << i << '\n';
      } else {
        file << '\n';
      }
      for (int j = 0; j < 4; j++) {
        file << 1.5 * j / 0.9 << "," << (i + 1) * (2. + 4. * (1.5 * j / 0.9))
             << "," << 1 << "," << 0 << '\n';
      }
    }
    file.unsetf(std::ios_base::floatfield);
    file.close();
    runTest(4, false, "#", "CSV", true); // cols doesn't matter here
    TS_ASSERT_THROWS_NOTHING(Poco::File(m_abspath).remove());
  }

private:
  const std::string getTestFileName() const {
    return m_filename + boost::lexical_cast<std::string>(m_testno) + m_ext;
  }
  std::string getAbsPath() {
    SaveAscii2 save;
    save.initialize();
    save.setPropertyValue("Filename", getTestFileName());
    return save.getPropertyValue("Filename");
  }
  // Write the test file
  std::string writeTestFile(const int cols, const bool header = true,
                            const std::string &comment = "#",
                            const bool scientific = true,
                            const int precision = -1,
                            const std::string &sep = "CSV",
                            const std::string &custsep = "") {
    SaveAscii2 save;
    save.initialize();
    save.setPropertyValue("Filename", getTestFileName());
    if (cols < 3) {
      // saveascii2 doens't save 2 column files it has to be made manually
      std::ofstream file(getTestFileName().c_str());
      if (scientific) {
        file << std::scientific;
      }
      if (header) {
        file << comment << "X , Y\n";
      }
      for (int i = 0; i < 5; i++) {
        file << i << '\n';
        for (int j = 0; j < 4; j++) {
          file << 1.5 * j / 0.9 << "," << (i + 1) * (2. + 4. * (1.5 * j / 0.9))
               << '\n';
        }
      }
      file.unsetf(std::ios_base::floatfield);
      file.close();
    } else {
      Mantid::DataObjects::Workspace2D_sptr wsToSave =
          boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(
              WorkspaceFactory::Instance().create("Workspace2D", 5, 4, 4));
      for (int i = 0; i < 5; i++) {
        auto &X = wsToSave->mutableX(i);
        auto &Y = wsToSave->mutableY(i);
        auto &E = wsToSave->mutableE(i);
        for (int j = 0; j < 4; j++) {
          X[j] = 1.5 * j / 0.9;
          Y[j] = (i + 1) * (2. + 4. * X[j]);
          E[j] = 1.;
        }
        if (cols == 4)
          wsToSave->setPointStandardDeviations(i, 4, 1.0);
      }
      const std::string name = "SaveAsciiWS";
      AnalysisDataService::Instance().add(name, wsToSave);

      save.initialize();
      save.isInitialized();
      if (precision > -1) {
        save.setPropertyValue("Precision",
                              boost::lexical_cast<std::string>(precision));
      }
      save.setPropertyValue("InputWorkspace", name);
      save.setPropertyValue("CommentIndicator", comment);
      save.setPropertyValue("ScientificFormat",
                            boost::lexical_cast<std::string>(scientific));
      save.setPropertyValue("ColumnHeader",
                            boost::lexical_cast<std::string>(header));
      save.setPropertyValue("WriteXError",
                            boost::lexical_cast<std::string>(cols == 4));
      save.setPropertyValue("Separator", sep);
      save.setPropertyValue("CustomSeparator", custsep);
      save.execute();

      AnalysisDataService::Instance().remove(name);
    }
    return save.getPropertyValue("Filename");
  }

  Mantid::API::MatrixWorkspace_sptr
  runTest(const int cols, const bool dataCheck = true,
          const std::string &comment = "#", const std::string &sep = "CSV",
          const bool execThrows = false, const std::string &custsep = "") {
    using Mantid::DataHandling::LoadAscii2;
    using namespace Mantid::API;

    LoadAscii2 loader;
    loader.initialize();
    loader.setRethrows(true);
    const std::string outputName(getTestFileName());
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", m_abspath));
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("OutputWorkspace", outputName));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Separator", sep));
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("CustomSeparator", custsep));
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("CommentIndicator", comment));

    if (execThrows) {
      TS_ASSERT_THROWS_ANYTHING(loader.execute());
    } else {
      TS_ASSERT_THROWS_NOTHING(loader.execute());

      TS_ASSERT_EQUALS(loader.isExecuted(), true);

      // Check the workspace
      AnalysisDataServiceImpl &dataStore = AnalysisDataService::Instance();
      if (dataStore.doesExist(outputName)) {
        TS_ASSERT_EQUALS(dataStore.doesExist(outputName), true);
        Workspace_sptr output;
        TS_ASSERT_THROWS_NOTHING(output = dataStore.retrieve(outputName));
        MatrixWorkspace_sptr outputWS =
            boost::dynamic_pointer_cast<MatrixWorkspace>(output);
        if (outputWS) {
          if (dataCheck) {
            checkData(outputWS, cols);
            // Test output
            TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->caption(), "Energy");
            TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->label(), "meV");
          }
          // Check if filename is saved
          TS_ASSERT_EQUALS(loader.getPropertyValue("Filename"),
                           outputWS->run().getProperty("Filename")->value());
        } else {
          TS_FAIL(outputName + " does not exist.");
        }
        dataStore.remove(outputName);
        return outputWS;
      } else {
        TS_FAIL("Cannot retrieve output workspace");
      }
    }
    MatrixWorkspace_sptr outputWS;
    return outputWS;
  }

  void checkData(const Mantid::API::MatrixWorkspace_sptr outputWS,
                 const int cols) {
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 5);
    TS_ASSERT_EQUALS(outputWS->blocksize(), 4);

    TS_ASSERT_DELTA(outputWS->x(0)[0], 0, 1e-6);
    TS_ASSERT_DELTA(outputWS->y(0)[0], 2, 1e-6);

    TS_ASSERT_DELTA(outputWS->x(0)[1], 1.666667, 1e-6);
    TS_ASSERT_DELTA(outputWS->y(0)[1], 8.666667, 1e-6);

    TS_ASSERT_DELTA(outputWS->x(1)[2], 3.333333, 1e-6);
    TS_ASSERT_DELTA(outputWS->y(1)[2], 30.66667, 1e-6);

    TS_ASSERT_DELTA(outputWS->x(3)[3], 5, 1e-6);
    TS_ASSERT_DELTA(outputWS->y(3)[3], 88, 1e-6);
    if (cols == 3 || cols == 4) {
      TS_ASSERT_DELTA(outputWS->e(0)[0], 1, 1e-6);

      TS_ASSERT_DELTA(outputWS->e(0)[1], 1, 1e-6);

      TS_ASSERT_DELTA(outputWS->e(1)[2], 1, 1e-6);

      TS_ASSERT_DELTA(outputWS->e(3)[3], 1, 1e-6);
    } else {
      TS_ASSERT_DELTA(outputWS->e(0)[0], 0, 1e-6);

      TS_ASSERT_DELTA(outputWS->e(0)[1], 0, 1e-6);

      TS_ASSERT_DELTA(outputWS->e(1)[2], 0, 1e-6);

      TS_ASSERT_DELTA(outputWS->e(3)[3], 0, 1e-6);
    }
    if (cols == 4) {
      TS_ASSERT_DELTA(outputWS->dx(0)[0], 1, 1e-6);

      TS_ASSERT_DELTA(outputWS->dx(0)[1], 1, 1e-6);

      TS_ASSERT_DELTA(outputWS->dx(1)[2], 1, 1e-6);

      TS_ASSERT_DELTA(outputWS->dx(3)[3], 1, 1e-6);
    }
  }
  std::string m_filename;
  std::string m_abspath;
  std::string m_ext;
  size_t m_testno;
};

class LoadAscii2TestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {
    setupFile();
    loadAlg.initialize();

    TS_ASSERT_THROWS_NOTHING(loadAlg.setPropertyValue("Filename", filename));
    loadAlg.setPropertyValue("OutputWorkspace", outputName);
    loadAlg.setPropertyValue("Separator", sep);
    loadAlg.setPropertyValue("CustomSeparator", custsep);
    loadAlg.setPropertyValue("CommentIndicator", comment);

    loadAlg.setRethrows(true);
  }

  void testLoadAscii2Performance() {
    TS_ASSERT_THROWS_NOTHING(loadAlg.execute());
  }

  void tearDown() override {
    TS_ASSERT_THROWS_NOTHING(Poco::File(filename).remove());
    AnalysisDataService::Instance().remove(outputName);
  }

private:
  LoadAscii2 loadAlg;

  const std::string outputName = "outWs";
  std::string filename;

  // Common saving/loading parameters
  const std::string sep = "CSV";
  const std::string custsep = "";
  const std::string comment = "#";

  void setupFile() {
    constexpr int numVecs = 100;
    constexpr int xyLen = 100;

    Mantid::DataObjects::Workspace2D_sptr wsToSave =
        boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(
            WorkspaceFactory::Instance().create("Workspace2D", numVecs, xyLen,
                                                xyLen));

    const std::string name = "SaveAsciiWS";
    AnalysisDataService::Instance().add(name, wsToSave);

    SaveAscii2 save;
    save.initialize();

    save.initialize();
    TS_ASSERT_EQUALS(save.isInitialized(), true);

    const bool scientific = true;

    save.setPropertyValue("Filename", "testFile");
    save.setPropertyValue("InputWorkspace", name);
    save.setPropertyValue("CommentIndicator", comment);
    save.setPropertyValue("ScientificFormat",
                          boost::lexical_cast<std::string>(scientific));
    save.setPropertyValue("ColumnHeader",
                          boost::lexical_cast<std::string>(true));
    save.setPropertyValue("WriteXError",
                          boost::lexical_cast<std::string>(false));
    save.setPropertyValue("Separator", sep);
    save.setPropertyValue("CustomSeparator", custsep);
    save.setRethrows(true);

    TS_ASSERT_THROWS_NOTHING(save.execute());

    AnalysisDataService::Instance().remove(name);
    filename = save.getPropertyValue("Filename");
  }
};

#endif // LOADASCIITEST_H_
