// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace.h"
#include "Reduction/ISISEnergyTransferModelUtils.h"

#include <boost/algorithm/string.hpp>
#include <chrono>
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <thread>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;

class ISISEnergyTransferModelUtilsTest : public CxxTest::TestSuite {
public:
  ISISEnergyTransferModelUtilsTest() = default;

  void teatCreateRangeString() {
    TS_ASSERT_EQUALS(createRangeString(1, 10), "1-10");
    TS_ASSERT_EQUALS(createRangeString(3, 7), "3-7");
  }

  void testCreateGroupString() {
    TS_ASSERT_EQUALS(createGroupString(1, 10), "1-10");
    TS_ASSERT_EQUALS(createGroupString(3, 5), "3-7");
  }

  void testCreateGroupingString() {
    TS_ASSERT_EQUALS(createGroupingString(2, 2, 1), "1-2,3-4");
    TS_ASSERT_EQUALS(createGroupingString(3, 3, 2), "2-4,5-7,8-10");
    TS_ASSERT_EQUALS(createGroupingString(1, 2, 4), "4-4,5-5");
    TS_ASSERT_EQUALS(createGroupingString(1, 1, 8), "8-8");
  }

  void testCreateDetectorGroupingString() {
    TS_ASSERT_EQUALS(createDetectorGroupingString(2, 2, 4, 1), "1-2,3-4");
    TS_ASSERT_EQUALS(createDetectorGroupingString(3, 4, 13, 2), "2-4,5-7,8-10,11-13,14-14");
    TS_ASSERT_EQUALS(createDetectorGroupingString(1, 1, 3, 3), "3-3");

    TS_ASSERT_EQUALS(createDetectorGroupingString(4, 2, 1), "1-2,3-4");
    TS_ASSERT_EQUALS(createDetectorGroupingString(9, 4, 3), "3-4,5-6,7-8,9-10,11-11");
    TS_ASSERT_EQUALS(createDetectorGroupingString(11, 3, 2), "2-4,5-7,8-10,11-12");
  }

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

    MatrixWorkspace_sptr workspace = getADSMatrixWorkspace("iris26184");

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