// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"
#include "Reduction/ISISEnergyTransferModelUtils.h"
#include <boost/algorithm/string.hpp>
#include <chrono>
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <thread>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::MantidWidgets::WorkspaceUtils;

class ISISEnergyTransferModelUtilsTest : public CxxTest::TestSuite {
public:
  static ISISEnergyTransferModelUtilsTest *createSuite() { return new ISISEnergyTransferModelUtilsTest(); }
  static void destroySuite(ISISEnergyTransferModelUtilsTest *suite) { delete suite; }

  void testGetCustomGroupingNumbers() {
    auto result = getCustomGroupingNumbers("1,2,3-5,6");
    std::vector<std::size_t> expected = {1, 2, 3, 5, 6};
    for (size_t i = 0; i < result.size(); i++) {
      TS_ASSERT_EQUALS(result[i], expected[i]);
    }
  }

  void testGetSampleLog() {
    auto loader = loadAlgorithm("iris26184_multi_graphite002_red", "iris26184");
    loader->execute();

    MatrixWorkspace_sptr workspace = getADSWorkspace("iris26184");

    TS_ASSERT_EQUALS(getSampleLog(workspace, {"sample", "sample_top", "sample_bottom"}, 300.0), 300.0);
    TS_ASSERT_EQUALS(getSampleLog(workspace, {"nchannels", "nspectra", "sample"}, 300.0), 2000.0);
  }

  void testLoadSampleLog() {
    TS_ASSERT_EQUALS(loadSampleLog("iris26184_multi_graphite002_red", {"sample", "sample_top", "sample_bottom"}, 300.0),
                     300.0);
    TS_ASSERT_EQUALS(loadSampleLog("iris26184_multi_graphite002_red", {"nchannels", "nspectra", "sample"}, 300.0),
                     2000.0);
  }

  void testParseInputFilesDoesNotThrowWhenProvidedAnInvalidString() {
    TS_ASSERT_THROWS_NOTHING(parseInputFiles("  "));
    TS_ASSERT_THROWS_NOTHING(parseInputFiles("  ,"));
    TS_ASSERT_THROWS_NOTHING(parseInputFiles(",C:/path/to/file2.raw"));
  }

  void testParseInputFilesReturnsThePathAndFilenameOfTheFirstFile() {
    auto const [rawFile, basename] = parseInputFiles("C:/path/to/file.raw,C:/path/to/file2.raw");

    TS_ASSERT_EQUALS(rawFile, "C:/path/to/file.raw");
    TS_ASSERT_EQUALS(basename, "file.raw");
  }

  void testCreateDetectorListReturnsAVectorWithOneValueWhenMinAndMaxAreEqual() {
    auto const detectorList = createDetectorList(3, 3);

    TS_ASSERT_EQUALS(1, detectorList.size());
    TS_ASSERT_EQUALS(3, detectorList[0]);
  }

  void testCreateDetectorListReturnsAVectorWithTheExpectedMinAndMaxValues() {
    auto const detectorList = createDetectorList(5, 9);

    TS_ASSERT_EQUALS(5, detectorList.size());
    TS_ASSERT_EQUALS(5, detectorList.front());
    TS_ASSERT_EQUALS(9, detectorList.back());
  }
};