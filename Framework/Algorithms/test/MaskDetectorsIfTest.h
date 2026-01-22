// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/MaskDetectorsIf.h"
#include "MantidFrameworkTestHelpers/ScopedFileHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include <filesystem>
#include <fstream>
#include <limits>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using ScopedFileHelper::ScopedFile;

class MaskDetectorsIfTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MaskDetectorsIfTest *createSuite() { return new MaskDetectorsIfTest(); }
  static void destroySuite(MaskDetectorsIfTest *suite) { delete suite; }

  void testCalFileDeselectIfNotEqual() {
    // setup and run the algorithm (includes basic checks)
    MaskDetectorsIf alg;
    // create the workspace
    setupAlgorithmForCalFiles(alg, "DeselectIf", "NotEqual", 2.2);
    std::ifstream file;
    runAlgorithmForCalFiles(alg, file);

    // specific checks
    if (file.is_open()) {
      readAndCheckLine(file, 0, 4, 0.0, 0, 1); // 2.0
      readAndCheckLine(file, 1, 5, 0.0, 0, 1); // 2.1
      readAndCheckLine(file, 2, 6, 0.0, 1, 2); // 2.2
      readAndCheckLine(file, 3, 7, 0.0, 0, 2); // 2.3
      file.close();
    }
  }

  void testCalFileDeselectIfLess() {
    // setup and run the algorithm (includes basic checks)
    MaskDetectorsIf alg;
    setupAlgorithmForCalFiles(alg, "DeselectIf", "Less", 2.2);
    std::ifstream file;
    runAlgorithmForCalFiles(alg, file);

    // specific checks
    if (file.is_open()) {
      readAndCheckLine(file, 0, 4, 0.0, 0, 1); // 2.0
      readAndCheckLine(file, 1, 5, 0.0, 0, 1); // 2.1
      readAndCheckLine(file, 2, 6, 0.0, 1, 2); // 2.2
      readAndCheckLine(file, 3, 7, 0.0, 1, 2); // 2.3
      file.close();
    }
  }

  void testCalFileDeselectIfLessEqual() {
    // setup and run the algorithm (includes basic checks)
    MaskDetectorsIf alg;
    setupAlgorithmForCalFiles(alg, "DeselectIf", "LessEqual", 2.2);
    std::ifstream file;
    runAlgorithmForCalFiles(alg, file);

    // specific checks
    if (file.is_open()) {
      readAndCheckLine(file, 0, 4, 0.0, 0, 1); // 2.0
      readAndCheckLine(file, 1, 5, 0.0, 0, 1); // 2.1
      readAndCheckLine(file, 2, 6, 0.0, 0, 2); // 2.2
      readAndCheckLine(file, 3, 7, 0.0, 1, 2); // 2.3
      file.close();
    }
  }

  void testCalFileDeselectIfGreater() {
    // setup and run the algorithm (includes basic checks)
    MaskDetectorsIf alg;
    setupAlgorithmForCalFiles(alg, "DeselectIf", "Greater", 2.2);
    std::ifstream file;
    runAlgorithmForCalFiles(alg, file);

    // specific checks
    if (file.is_open()) {
      readAndCheckLine(file, 0, 4, 0.0, 1, 1); // 2.0
      readAndCheckLine(file, 1, 5, 0.0, 1, 1); // 2.1
      readAndCheckLine(file, 2, 6, 0.0, 1, 2); // 2.2
      readAndCheckLine(file, 3, 7, 0.0, 0, 2); // 2.3
      file.close();
    }
  }

  void testCalFileDeselectIfGreaterEqual() {
    // setup and run the algorithm (includes basic checks)
    MaskDetectorsIf alg;
    setupAlgorithmForCalFiles(alg, "DeselectIf", "GreaterEqual", 2.2);
    std::ifstream file;
    runAlgorithmForCalFiles(alg, file);

    // specific checks
    if (file.is_open()) {
      readAndCheckLine(file, 0, 4, 0.0, 1, 1); // 2.0
      readAndCheckLine(file, 1, 5, 0.0, 1, 1); // 2.1
      readAndCheckLine(file, 2, 6, 0.0, 0, 2); // 2.2
      readAndCheckLine(file, 3, 7, 0.0, 0, 2); // 2.3
      file.close();
    }
  }

  void testCalFileSelectIfEqual() {
    // Create an input file where the detectors are all deselected
    // initially (so we can tell whether the SelectIf worked).
    ScopedFile inputFile = makeFakeInputFile();

    // setup and run the algorithm (includes basic checks)
    MaskDetectorsIf alg;
    setupAlgorithmForCalFiles(alg, "SelectIf", "Equal", 2.2, inputFile.getFileName());
    std::ifstream file;
    runAlgorithmForCalFiles(alg, file);

    // specific checks
    if (file.is_open()) {
      readAndCheckLine(file, 0, 4, 0.0, 0, 1); // 2.0
      readAndCheckLine(file, 1, 5, 0.0, 0, 1); // 2.1
      readAndCheckLine(file, 2, 6, 0.0, 1, 2); // 2.2
      readAndCheckLine(file, 3, 7, 0.0, 0, 2); // 2.3
      file.close();
    }
  }

  void testMaskWorkspaceDeselectIfNotEqual() {
    auto correctMasking = [](MatrixWorkspace const &ws, const size_t wsIndex) { return ws.y(wsIndex).front() == 2.2; };
    MaskDetectorsIf alg;
    MatrixWorkspace_sptr inWS = makeFakeWorkspace();
    maskAllDetectors(inWS);
    setupAlgorithmForOutputWorkspace(alg, inWS, "DeselectIf", "NotEqual", 2.2);
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    checkOutputWorkspace(alg, correctMasking);
  }

  void testMaskWorkspaceSelectIfEqual() {
    auto correctMasking = [](MatrixWorkspace const &ws, const size_t wsIndex) { return ws.y(wsIndex).front() == 2.2; };
    MaskDetectorsIf alg;
    MatrixWorkspace_sptr inWS = makeFakeWorkspace();
    setupAlgorithmForOutputWorkspace(alg, inWS, "SelectIf", "Equal", 2.2);
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    checkOutputWorkspace(alg, correctMasking);
  }

  void testMaskWorkspaceDeselectIfLess() {
    auto correctMasking = [](MatrixWorkspace const &ws, const size_t wsIndex) { return ws.y(wsIndex).front() >= 2.2; };
    MaskDetectorsIf alg;
    MatrixWorkspace_sptr inWS = makeFakeWorkspace();
    maskAllDetectors(inWS);
    setupAlgorithmForOutputWorkspace(alg, inWS, "DeselectIf", "Less", 2.2);
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    checkOutputWorkspace(alg, correctMasking);
  }

  void testMaskWorkspaceSelectIfGreater() {
    auto correctMasking = [](MatrixWorkspace const &ws, const size_t wsIndex) { return ws.y(wsIndex).front() > 2.2; };
    MaskDetectorsIf alg;
    MatrixWorkspace_sptr inWS = makeFakeWorkspace();
    setupAlgorithmForOutputWorkspace(alg, inWS, "SelectIf", "Greater", 2.2);
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    checkOutputWorkspace(alg, correctMasking);
  }

  void testMaskWorkspaceSelectIfNotFinite() {
    auto correctMasking = [](MatrixWorkspace const &ws, const size_t wsIndex) {
      return !std::isfinite(ws.y(wsIndex).front());
    };
    MaskDetectorsIf alg;
    MatrixWorkspace_sptr inWS = makeFakeWorkspace();
    // add some non finite values
    inWS->mutableY(1)[0] = std::numeric_limits<double>::quiet_NaN();
    inWS->mutableY(3)[0] = std::numeric_limits<double>::infinity();

    setupAlgorithmForOutputWorkspace(alg, inWS, "SelectIf", "NotFinite", 0.0);
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    checkOutputWorkspace(alg, correctMasking);
  }

  void testStartEndWorkspaceIndex() {
    auto correctMasking = [](MatrixWorkspace const &ws, const size_t wsIndex) {
      if (wsIndex < 1 || wsIndex > 2)
        return false;
      return ws.y(wsIndex).front() > 2.2;
    };
    MaskDetectorsIf alg;
    MatrixWorkspace_sptr inWS = makeFakeWorkspace();
    setupAlgorithmForOutputWorkspace(alg, inWS, "SelectIf", "Greater", 2.2, 1, 2);
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    checkOutputWorkspace(alg, correctMasking);
  }

private:
  constexpr static int numBanks{1};
  constexpr static int numPixels{2};
  constexpr static int numBins{1};
  constexpr static int numHist{numBanks * numPixels * numPixels};

  template <typename T> void checkOutputWorkspace(MaskDetectorsIf &alg, T correctMasking) {
    MatrixWorkspace_sptr inW = alg.getProperty("InputWorkspace");
    MatrixWorkspace_sptr mask = alg.getProperty("OutputWorkspace");
    TS_ASSERT(mask)
    TS_ASSERT_EQUALS(mask->getNumberHistograms(), numHist)
    const auto &spectrumInfo = mask->spectrumInfo();
    for (size_t i = 0; i < numHist; ++i) {
      TS_ASSERT_EQUALS(spectrumInfo.isMasked(i), correctMasking(*inW, i))
      if (std::isfinite(mask->y(i).front())) {
        if (spectrumInfo.isMasked(i)) {
          TS_ASSERT_EQUALS(mask->y(i).front(), 0.);
        } else {
          TS_ASSERT_EQUALS(mask->y(i).front(), inW->y(i).front());
        }
      }
    }
  }

  // Create a fake input file. This is the same as
  // 4detector_cal_example_file.cal but with all the detectors deselected.
  static ScopedFile makeFakeInputFile() {
    std::ostringstream os;

    os << "# Ariel detector file, written Sat Nov 24 16:52:56 2007\n";
    os << "# Format: number  UDET offset  select  group\n";
    os << "0          4  0.0000000  0    1\n";
    os << "1          5  0.0000000  0    1\n";
    os << "2          6  0.0000000  0    2\n";
    os << "3          7  0.0000000  0    2\n";

    return ScopedFile(os.str(), "MaskDetectorsIfTestInput.cal");
  }

  static const MatrixWorkspace_sptr makeFakeWorkspace() {
    // create the workspace
    MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(numBanks, numPixels, numBins);

    // Default y values are all 2.0. Change them so they're different
    // for each spectrum (this gives us the values 2.0, 2.1, 2.2, ...)
    for (size_t wi = 0; wi < ws->getNumberHistograms(); ++wi) {
      ws->mutableY(wi)[0] += static_cast<double>(wi) * 0.1;
    }

    return ws;
  }

  static void maskAllDetectors(MatrixWorkspace_sptr &ws) {
    auto &detectorInfo = ws->mutableDetectorInfo();
    for (size_t i = 0; i < detectorInfo.size(); ++i) {
      detectorInfo.setMasked(i, true);
    }
  }

  // Initialise the algorithm and set the properties. Creates a fake
  // workspace for the input.
  static void setupAlgorithmForCalFiles(MaskDetectorsIf &alg, const std::string &mode, const std::string &op,
                                        const double value,
                                        const std::string &inputFile = "4detector_cal_example_file.cal") {
    // create the workspace
    const MatrixWorkspace_sptr inWS = makeFakeWorkspace();

    // set up the algorithm
    if (!alg.isInitialized())
      alg.initialize();
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("InputCalFile", inputFile);
    alg.setProperty("Mode", mode);
    alg.setProperty("Operator", op);
    alg.setProperty("Value", value);
    alg.setProperty("OutputCalFile", "MaskDetectorsIfTestOutput.cal");
  }

  // Initialise the algorithm and set the properties. Creates a fake
  // workspace for the input.
  static void setupAlgorithmForOutputWorkspace(MaskDetectorsIf &alg, const MatrixWorkspace_sptr &inWS,
                                               const std::string &mode, const std::string &op, const double value,
                                               int startIx = 0, int endIx = EMPTY_INT()) {
    // set up the algorithm
    if (!alg.isInitialized())
      alg.initialize();
    alg.setRethrows(true);
    alg.setChild(true);
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("Mode", mode);
    alg.setProperty("Operator", op);
    alg.setProperty("Value", value);
    alg.setProperty("StartWorkspaceIndex", startIx);
    alg.setProperty("EndWorkspaceIndex", endIx);
    alg.setProperty("OutputWorkspace", "_unused_for_child");
  }

  // Run the algorithm and do some basic checks. Opens the output file
  // stream if everything is ok (leaves it closed if not).
  static void runAlgorithmForCalFiles(MaskDetectorsIf &alg, std::ifstream &outFile) {
    // run the algorithm
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // check that the algorithm has written a file to disk
    std::string outputFile = alg.getProperty("OutputCalFile");
    bool fileExists = false;
    TS_ASSERT(fileExists = std::filesystem::exists(outputFile));

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

  static void skipHeader(std::ifstream &file) {
    // our test file has 2 lines in the header
    std::string line;
    for (int i = 0; i < 2 && !file.eof(); ++i) {
      std::getline(file, line);
    }
  }

  // Read the next line from the given file and check
  // that the values match those given.
  static void readAndCheckLine(std::ifstream &file, const int num, const int udet, const double offset,
                               const int select, const int group) {
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
