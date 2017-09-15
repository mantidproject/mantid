#ifndef MASKDETECTORSIFTEST_H_
#define MASKDETECTORSIFTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/MaskDetectorsIf.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidTestHelpers/ScopedFileHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <Poco/File.h>
#include <fstream>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using ScopedFileHelper::ScopedFile;

class MaskDetectorsIfTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MaskDetectorsIfTest *createSuite() {
    return new MaskDetectorsIfTest();
  }
  static void destroySuite(MaskDetectorsIfTest *suite) { delete suite; }

  void testDeselectIfNotEqual() {
    // setup and run the algorithm (includes basic checks)
    MaskDetectorsIf alg;
    setupAlgorithm(alg, "DeselectIf", "NotEqual", 2.2);
    std::ifstream file;
    runAlgorithm(alg, file);

    // specific checks
    if (file.is_open()) {
      readAndCheckLine(file, 0, 4, 0.0, 0, 1); // 2.0
      readAndCheckLine(file, 1, 5, 0.0, 0, 1); // 2.1
      readAndCheckLine(file, 2, 6, 0.0, 1, 2); // 2.2
      readAndCheckLine(file, 3, 7, 0.0, 0, 2); // 2.3
      file.close();
    }
  }

  void testDeselectIfLess() {
    // setup and run the algorithm (includes basic checks)
    MaskDetectorsIf alg;
    setupAlgorithm(alg, "DeselectIf", "Less", 2.2);
    std::ifstream file;
    runAlgorithm(alg, file);

    // specific checks
    if (file.is_open()) {
      readAndCheckLine(file, 0, 4, 0.0, 0, 1); // 2.0
      readAndCheckLine(file, 1, 5, 0.0, 0, 1); // 2.1
      readAndCheckLine(file, 2, 6, 0.0, 1, 2); // 2.2
      readAndCheckLine(file, 3, 7, 0.0, 1, 2); // 2.3
      file.close();
    }
  }

  void testDeselectIfLessEqual() {
    // setup and run the algorithm (includes basic checks)
    MaskDetectorsIf alg;
    setupAlgorithm(alg, "DeselectIf", "LessEqual", 2.2);
    std::ifstream file;
    runAlgorithm(alg, file);

    // specific checks
    if (file.is_open()) {
      readAndCheckLine(file, 0, 4, 0.0, 0, 1); // 2.0
      readAndCheckLine(file, 1, 5, 0.0, 0, 1); // 2.1
      readAndCheckLine(file, 2, 6, 0.0, 0, 2); // 2.2
      readAndCheckLine(file, 3, 7, 0.0, 1, 2); // 2.3
      file.close();
    }
  }

  void testDeselectIfGreater() {
    // setup and run the algorithm (includes basic checks)
    MaskDetectorsIf alg;
    setupAlgorithm(alg, "DeselectIf", "Greater", 2.2);
    std::ifstream file;
    runAlgorithm(alg, file);

    // specific checks
    if (file.is_open()) {
      readAndCheckLine(file, 0, 4, 0.0, 1, 1); // 2.0
      readAndCheckLine(file, 1, 5, 0.0, 1, 1); // 2.1
      readAndCheckLine(file, 2, 6, 0.0, 1, 2); // 2.2
      readAndCheckLine(file, 3, 7, 0.0, 0, 2); // 2.3
      file.close();
    }
  }

  void testDeselectIfGreaterEqual() {
    // setup and run the algorithm (includes basic checks)
    MaskDetectorsIf alg;
    setupAlgorithm(alg, "DeselectIf", "GreaterEqual", 2.2);
    std::ifstream file;
    runAlgorithm(alg, file);

    // specific checks
    if (file.is_open()) {
      readAndCheckLine(file, 0, 4, 0.0, 1, 1); // 2.0
      readAndCheckLine(file, 1, 5, 0.0, 1, 1); // 2.1
      readAndCheckLine(file, 2, 6, 0.0, 0, 2); // 2.2
      readAndCheckLine(file, 3, 7, 0.0, 0, 2); // 2.3
      file.close();
    }
  }

  void testSelectIfEqual() {
    // Create an input file where the detectors are all deselected
    // initially (so we can tell whether the SelectIf worked).
    ScopedFile inputFile = makeFakeInputFile();

    // setup and run the algorithm (includes basic checks)
    MaskDetectorsIf alg;
    setupAlgorithm(alg, "SelectIf", "Equal", 2.2, inputFile.getFileName());
    std::ifstream file;
    runAlgorithm(alg, file);

    // specific checks
    if (file.is_open()) {
      readAndCheckLine(file, 0, 4, 0.0, 0, 1); // 2.0
      readAndCheckLine(file, 1, 5, 0.0, 0, 1); // 2.1
      readAndCheckLine(file, 2, 6, 0.0, 1, 2); // 2.2
      readAndCheckLine(file, 3, 7, 0.0, 0, 2); // 2.3
      file.close();
    }
  }

private:
  // Create a fake input file. This is the same as
  // 4detector_cal_example_file.cal but with all the detectors deselected.
  ScopedFile makeFakeInputFile() {
    std::ostringstream os;

    os << "# Ariel detector file, written Sat Nov 24 16:52:56 2007\n";
    os << "# Format: number  UDET offset  select  group\n";
    os << "0          4  0.0000000  0    1\n";
    os << "1          5  0.0000000  0    1\n";
    os << "2          6  0.0000000  0    2\n";
    os << "3          7  0.0000000  0    2\n";

    return ScopedFile(os.str(), "MaskDetectorsIfTestInput.cal");
  }

  const MatrixWorkspace_sptr makeFakeWorkspace() {
    // create the workspace
    MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            1, 2, 1);

    // Default y values are all 2.0. Change them so they're different
    // for each spectrum (this gives us the values 2.0, 2.1, 2.2, ...)
    for (size_t wi = 0; wi < ws->getNumberHistograms(); ++wi) {
      ws->mutableY(wi)[0] += static_cast<double>(wi) * 0.1;
    }

    return ws;
  }

  // Initialise the algorithm and set the properties. Creates a fake
  // workspace for the input.
  void setupAlgorithm(
      MaskDetectorsIf &alg, const std::string &mode, const std::string &op,
      const double value,
      const std::string &inputFile = "4detector_cal_example_file.cal") {
    // create the workspace
    const MatrixWorkspace_sptr inWS = makeFakeWorkspace();

    // set up the algorithm
    if (!alg.isInitialized())
      alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("InputCalFile", inputFile);
    alg.setProperty("Mode", mode);
    alg.setProperty("Operator", op);
    alg.setProperty("Value", value);
    alg.setProperty("OutputCalFile", "MaskDetectorsIfTestOutput.cal");
  }

  // Run the algorithm and do some basic checks. Opens the output file
  // stream if everything is ok (leaves it closed if not).
  void runAlgorithm(MaskDetectorsIf &alg, std::ifstream &outFile) {
    // run the algorithm
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // check that the algorithm has written a file to disk
    std::string outputFile = alg.getProperty("OutputCalFile");
    bool fileExists = false;
    TS_ASSERT(fileExists = Poco::File(outputFile).exists());

    // check that we can open the file
    if (fileExists) {
      outFile.open(outputFile.c_str());
      TS_ASSERT(outFile.is_open());

      // skip the header, and check that there is still content left
      if (outFile.is_open()) {
        skipHeader(outFile);
        TS_ASSERT(!outFile.eof());

        if (outFile.eof())
          outFile.close();
      }
    }
  }

  void skipHeader(std::ifstream &file) {
    // our test file has 2 lines in the header
    std::string line;
    for (int i = 0; i < 2 && !file.eof(); ++i) {
      std::getline(file, line);
    }
  }

  // Read the next line from the given file and check
  // that the values match those given.
  void readAndCheckLine(std::ifstream &file, const int num, const int udet,
                        const double offset, const int select,
                        const int group) {
    TS_ASSERT(!file.eof());

    if (!file.eof()) {
      int i1, i2, i3, i4;
      double d1;
      file >> i1 >> i2 >> d1 >> i3 >> i4;
      TS_ASSERT_EQUALS(i1, num);
      TS_ASSERT_EQUALS(i2, udet);
      TS_ASSERT_DELTA(d1, offset, 1e-06);
      TS_ASSERT_EQUALS(i3, select);
      TS_ASSERT_EQUALS(i4, group);
    }
  }
};

#endif /*MASKDETECTORSIFTEST_H_*/
