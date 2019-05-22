// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_SINQ_POLDITRUNCATEDATATEST_H_
#define MANTID_SINQ_POLDITRUNCATEDATATEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidSINQ/PoldiTruncateData.h"
#include "MantidSINQ/PoldiUtilities/PoldiMockInstrumentHelpers.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidHistogramData/LinearGenerator.h"

using namespace Mantid::Poldi;
using namespace Mantid::API;

using ::testing::Return;
using Mantid::HistogramData::LinearGenerator;
using Mantid::HistogramData::Points;

class PoldiTruncateDataTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PoldiTruncateDataTest *createSuite() {
    return new PoldiTruncateDataTest();
  }
  static void destroySuite(PoldiTruncateDataTest *suite) { delete suite; }

  PoldiTruncateDataTest() { FrameworkManager::Instance(); }

  void testInit() {
    PoldiTruncateData truncate;
    truncate.initialize();

    TS_ASSERT_EQUALS(truncate.getProperties().size(), 3);
    TS_ASSERT(truncate.existsProperty("InputWorkspace"));
    TS_ASSERT(truncate.existsProperty("ExtraCountsWorkspaceName"));
    TS_ASSERT(truncate.existsProperty("OutputWorkspace"));
  }

  void testSetChopper() {
    boost::shared_ptr<MockChopper> chopper = boost::make_shared<MockChopper>();

    TestablePoldiTruncateData truncate;

    TS_ASSERT(!truncate.m_chopper);

    truncate.setChopper(chopper);

    TS_ASSERT_EQUALS(truncate.m_chopper, chopper);
  }

  void testSetTimeBinWidth() {
    TestablePoldiTruncateData truncate;

    TS_ASSERT_EQUALS(truncate.m_timeBinWidth, 0.0);

    truncate.setTimeBinWidth(3.0);

    TS_ASSERT_EQUALS(truncate.m_timeBinWidth, 3.0);
  }

  void testSetActualBinCount() {
    TestablePoldiTruncateData truncate;
    TS_ASSERT_EQUALS(truncate.m_actualBinCount, 0);

    truncate.setActualBinCount(200);
    TS_ASSERT_EQUALS(truncate.m_actualBinCount, 200);
  }

  void testSetTimeBinWidthFromWorkspace() {
    // workspace with delta x = 1.0, 3 bins (= 4 boundaries)
    MatrixWorkspace_sptr matrixWs =
        WorkspaceCreationHelper::create2DWorkspaceWhereYIsWorkspaceIndex(1, 3);

    TestablePoldiTruncateData truncate;
    TS_ASSERT_THROWS_NOTHING(truncate.setTimeBinWidthFromWorkspace(matrixWs));
    TS_ASSERT_EQUALS(truncate.m_timeBinWidth, 1.0);
    TS_ASSERT_EQUALS(truncate.m_actualBinCount, 4);

    MatrixWorkspace_sptr invalidSpectra;
    TS_ASSERT_THROWS(truncate.setTimeBinWidthFromWorkspace(invalidSpectra),
                     const std::invalid_argument &);

    // matrix workspace with one bin
    MatrixWorkspace_sptr invalidBins =
        WorkspaceCreationHelper::create2DWorkspace123(1, 1);

    TS_ASSERT_THROWS(truncate.setTimeBinWidthFromWorkspace(invalidBins),
                     const std::invalid_argument &);
  }

  void testCalculateBinCount() {
    boost::shared_ptr<MockChopper> chopper = boost::make_shared<MockChopper>();
    EXPECT_CALL(*chopper, cycleTime()).Times(1).WillRepeatedly(Return(1500.0));

    TestablePoldiTruncateData truncate;
    TS_ASSERT_THROWS(truncate.getCalculatedBinCount(), const std::invalid_argument &);

    truncate.setChopper(chopper);
    TS_ASSERT_THROWS(truncate.getCalculatedBinCount(), const std::invalid_argument &);

    truncate.setTimeBinWidth(-10.0);
    TS_ASSERT_THROWS(truncate.getCalculatedBinCount(), const std::invalid_argument &);

    truncate.setTimeBinWidth(0.0);
    TS_ASSERT_THROWS(truncate.getCalculatedBinCount(), const std::invalid_argument &);

    truncate.setTimeBinWidth(3.0);

    size_t calculatedBinCount = 0;
    TS_ASSERT_THROWS_NOTHING(calculatedBinCount =
                                 truncate.getCalculatedBinCount());

    TS_ASSERT_EQUALS(calculatedBinCount, 500);
  }

  void testGetMaximumTimeValue() {
    boost::shared_ptr<MockChopper> chopper = boost::make_shared<MockChopper>();
    EXPECT_CALL(*chopper, cycleTime()).Times(1).WillRepeatedly(Return(1500.0));

    TestablePoldiTruncateData truncate;
    truncate.setChopper(chopper);
    truncate.setTimeBinWidth(3.0);

    size_t calculatedBinCount = truncate.getCalculatedBinCount();

    // throws, because actual bin count is smaller than calculated bin count
    TS_ASSERT_THROWS(truncate.getMaximumTimeValue(calculatedBinCount),
                     const std::invalid_argument &);

    truncate.setActualBinCount(500);
    TS_ASSERT_EQUALS(truncate.getMaximumTimeValue(calculatedBinCount),
                     499.0 * 3.0);
  }

  void testGetMinimumExtraTimeValue() {
    boost::shared_ptr<MockChopper> chopper = boost::make_shared<MockChopper>();
    EXPECT_CALL(*chopper, cycleTime()).Times(1).WillRepeatedly(Return(1500.0));

    TestablePoldiTruncateData truncate;
    truncate.setChopper(chopper);
    truncate.setTimeBinWidth(3.0);

    size_t calculatedBinCount = truncate.getCalculatedBinCount();

    // throws, because actual bin count is smaller than calculated bin count
    TS_ASSERT_THROWS(truncate.getMinimumExtraTimeValue(calculatedBinCount),
                     const std::invalid_argument &);

    // still throws - there are no extra bins.
    truncate.setActualBinCount(500);
    TS_ASSERT_THROWS(truncate.getMinimumExtraTimeValue(calculatedBinCount),
                     const std::invalid_argument &);

    // this must work
    truncate.setActualBinCount(550);
    TS_ASSERT_EQUALS(truncate.getMinimumExtraTimeValue(calculatedBinCount),
                     500.0 * 3.0);
  }

  void testGetCroppedWorkspace() {
    boost::shared_ptr<MockChopper> chopper = boost::make_shared<MockChopper>();
    EXPECT_CALL(*chopper, cycleTime()).Times(2).WillRepeatedly(Return(1500.0));

    MatrixWorkspace_sptr inputWorkspace =
        getProperWorkspaceWithXValues(1, 600, 3.0);

    TestablePoldiTruncateData truncate;
    truncate.setChopper(chopper);
    truncate.setTimeBinWidthFromWorkspace(inputWorkspace);

    MatrixWorkspace_sptr cropped = truncate.getCroppedWorkspace(inputWorkspace);

    // number of histograms does not change
    TS_ASSERT_EQUALS(cropped->getNumberHistograms(), 1);

    const auto &xData = cropped->x(0);

    TS_ASSERT_EQUALS(xData.size(), 500);

    // workspace which is too small
    MatrixWorkspace_sptr smallWorkspace =
        getProperWorkspaceWithXValues(1, 400, 3.0);
    truncate.setTimeBinWidthFromWorkspace(smallWorkspace);

    TS_ASSERT_THROWS(truncate.getCroppedWorkspace(smallWorkspace),
                     const std::invalid_argument &);
  }

  void testGetExtraCountsWorkspace() {
    boost::shared_ptr<MockChopper> chopper = boost::make_shared<MockChopper>();
    EXPECT_CALL(*chopper, cycleTime()).Times(2).WillRepeatedly(Return(1500.0));

    MatrixWorkspace_sptr inputWorkspace =
        getProperWorkspaceWithXValues(10, 600, 3.0);

    TestablePoldiTruncateData truncate;
    truncate.setChopper(chopper);
    truncate.setTimeBinWidthFromWorkspace(inputWorkspace);

    MatrixWorkspace_sptr cropped =
        truncate.getExtraCountsWorkspace(inputWorkspace);

    // number of histograms does not change
    TS_ASSERT_EQUALS(cropped->getNumberHistograms(), 1);

    const auto &xData = cropped->x(0);

    TS_ASSERT_EQUALS(xData.size(), 100);

    // workspace which is too small
    MatrixWorkspace_sptr smallWorkspace =
        getProperWorkspaceWithXValues(1, 400, 3.0);
    truncate.setTimeBinWidthFromWorkspace(smallWorkspace);

    TS_ASSERT_THROWS(truncate.getExtraCountsWorkspace(smallWorkspace),
                     const std::invalid_argument &);
  }

  void testGetWorkspaceBelowX() {
    TestablePoldiTruncateData truncate;
    MatrixWorkspace_sptr workspace = getProperWorkspaceWithXValues(1, 600, 3.0);

    MatrixWorkspace_sptr below = truncate.getWorkspaceBelowX(workspace, 1497.0);

    const auto &x = below->x(0);

    TS_ASSERT_EQUALS(x.size(), 500);
    TS_ASSERT_EQUALS(x.front(), 0.0);
    TS_ASSERT_EQUALS(x.back(), 1497.0);
  }

  void testGetWorkspaceAboveX() {
    TestablePoldiTruncateData truncate;
    MatrixWorkspace_sptr workspace = getProperWorkspaceWithXValues(1, 600, 3.0);

    MatrixWorkspace_sptr above = truncate.getWorkspaceAboveX(workspace, 1500.0);

    const auto &x = above->x(0);

    TS_ASSERT_EQUALS(x.size(), 100);
    TS_ASSERT_EQUALS(x.front(), 1500.0);
    TS_ASSERT_EQUALS(x.back(), 1797.0);
  }

  void testGetSummedSpectra() {
    TestablePoldiTruncateData truncate;
    MatrixWorkspace_sptr workspace = getProperWorkspaceWithXValues(10, 10, 3.0);

    MatrixWorkspace_sptr summed = truncate.getSummedSpectra(workspace);

    TS_ASSERT_EQUALS(summed->getNumberHistograms(), 1);

    // since all y-values are 2.0, the sum should be 10 * 2.0
    TS_ASSERT_EQUALS(summed->y(0).front(), 20.0);
    TS_ASSERT_EQUALS(summed->y(0).back(), 20.0);
  }

  void testGetCropAlgorithmForWorkspace() {
    MatrixWorkspace_sptr workspace = getProperWorkspaceWithXValues(10, 10, 3.0);

    TestablePoldiTruncateData truncate;
    TS_ASSERT_THROWS_NOTHING(truncate.getCropAlgorithmForWorkspace(workspace));

    Algorithm_sptr cropAlgorithm =
        truncate.getCropAlgorithmForWorkspace(workspace);
    TS_ASSERT_EQUALS(cropAlgorithm->name(), "CropWorkspace");

    MatrixWorkspace_sptr inputWorkspace =
        cropAlgorithm->getProperty("InputWorkspace");
    TS_ASSERT_EQUALS(inputWorkspace, workspace);
  }

  void testGetOutputWorkspace() {
    MatrixWorkspace_sptr workspace = getProperWorkspaceWithXValues(10, 10, 3.0);

    TestablePoldiTruncateData truncate;
    Algorithm_sptr cropAlgorithm =
        truncate.getCropAlgorithmForWorkspace(workspace);
    MatrixWorkspace_sptr outputWorkspace =
        truncate.getOutputWorkspace(cropAlgorithm);

    TS_ASSERT(outputWorkspace)
  }

private:
  MatrixWorkspace_sptr getProperWorkspaceWithXValues(size_t histograms,
                                                     size_t binCount,
                                                     double spacing) {
    Points xValues(binCount, LinearGenerator(0, spacing));

    MatrixWorkspace_sptr workspace =
        WorkspaceCreationHelper::create2DWorkspace123(histograms, binCount);

    for (size_t i = 0; i < histograms; ++i)
      workspace->setPoints(i, xValues);

    return workspace;
  }

  class TestablePoldiTruncateData : public PoldiTruncateData {
    friend class PoldiTruncateDataTest;

  public:
    TestablePoldiTruncateData() : PoldiTruncateData() {}
    ~TestablePoldiTruncateData() override {}
  };
};

#endif /* MANTID_SINQ_POLDITRUNCATEDATATEST_H_ */
