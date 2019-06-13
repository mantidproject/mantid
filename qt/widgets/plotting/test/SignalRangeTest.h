// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_API_SIGNALRANGETEST_H_
#define MANTIDQT_API_SIGNALRANGETEST_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Plotting/Qwt/SignalRange.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/IMDIterator.h"

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class SignalRangeTest : public CxxTest::TestSuite {
private:
  class MockMDWorkspace : public Mantid::API::IMDWorkspace {
  public:
    const std::string id() const override { return "MockMDWorkspace"; }
    size_t getMemorySize() const override { return 0; }
    MOCK_CONST_METHOD0(getNPoints, uint64_t());
    MOCK_CONST_METHOD0(getNEvents, uint64_t());
    MOCK_CONST_METHOD2(createIterators,
                       std::vector<std::unique_ptr<Mantid::API::IMDIterator>>(
                           size_t, Mantid::Geometry::MDImplicitFunction *));
    MOCK_CONST_METHOD2(getSignalAtCoord,
                       Mantid::signal_t(const Mantid::coord_t *,
                                        const Mantid::API::MDNormalization &));
    MOCK_CONST_METHOD2(getSignalWithMaskAtCoord,
                       Mantid::signal_t(const Mantid::coord_t *,
                                        const Mantid::API::MDNormalization &));
    MOCK_CONST_METHOD3(getLinePlot, Mantid::API::IMDWorkspace::LinePlot(
                                        const Mantid::Kernel::VMD &,
                                        const Mantid::Kernel::VMD &,
                                        Mantid::API::MDNormalization));
    MOCK_CONST_METHOD1(
        createIterator,
        Mantid::API::IMDIterator *(Mantid::Geometry::MDImplicitFunction *));
    MOCK_CONST_METHOD2(getSignalAtVMD,
                       Mantid::signal_t(const Mantid::Kernel::VMD &,
                                        const Mantid::API::MDNormalization &));

    MOCK_METHOD1(mockSetMDMasking, void(Mantid::Geometry::MDImplicitFunction* func));
    void setMDMasking(std::unique_ptr<Mantid::Geometry::MDImplicitFunction> func){
      mockSetMDMasking(func.get());
    }
    MOCK_METHOD0(clearMDMasking, void());
    MOCK_CONST_METHOD0(getSpecialCoordinateSystem,
                       Mantid::Kernel::SpecialCoordinateSystem());
    MOCK_CONST_METHOD0(isMDHistoWorkspace, bool());
    MOCK_CONST_METHOD0(hasOrientedLattice, bool());

  private:
    MockMDWorkspace *doClone() const override {
      throw std::runtime_error(
          "Cloning of MockMDWorkspace is not implemented.");
    }
    MockMDWorkspace *doCloneEmpty() const override {
      throw std::runtime_error(
          "Cloning of MockMDWorkspace is not implemented.");
    }
  };

  class MockMDIterator : public Mantid::API::IMDIterator {
  public:
    MOCK_CONST_METHOD0(getDataSize, size_t());
    MOCK_METHOD0(next, bool());
    MOCK_CONST_METHOD0(valid, bool());
    MOCK_METHOD1(jumpTo, void(size_t));
    MOCK_METHOD1(next, bool(size_t));
    MOCK_CONST_METHOD0(getNormalizedSignal, Mantid::signal_t());
    MOCK_CONST_METHOD0(getNormalizedError, Mantid::signal_t());
    MOCK_CONST_METHOD0(getNormalizedSignalWithMask, Mantid::signal_t());
    MOCK_CONST_METHOD0(getSignal, Mantid::signal_t());
    MOCK_CONST_METHOD0(getError, Mantid::signal_t());
    MOCK_CONST_METHOD1(getVertexesArray,
                       std::unique_ptr<Mantid::coord_t[]>(size_t &));
    MOCK_CONST_METHOD3(getVertexesArray,
                       std::unique_ptr<Mantid::coord_t[]>(size_t &,
                                                          const size_t,
                                                          const bool *));
    MOCK_CONST_METHOD0(getCenter, Mantid::Kernel::VMD());
    MOCK_CONST_METHOD0(getNumEvents, size_t());
    MOCK_CONST_METHOD1(getInnerRunIndex, uint16_t(size_t));
    MOCK_CONST_METHOD1(getInnerDetectorID, int32_t(size_t));
    MOCK_CONST_METHOD2(getInnerPosition, Mantid::coord_t(size_t, size_t));
    MOCK_CONST_METHOD1(getInnerSignal, Mantid::signal_t(size_t));
    MOCK_CONST_METHOD1(getInnerError, Mantid::signal_t(size_t));
    MOCK_CONST_METHOD0(getIsMasked, bool());
    MOCK_CONST_METHOD0(findNeighbourIndexesFaceTouching, std::vector<size_t>());
    MOCK_CONST_METHOD0(findNeighbourIndexes, std::vector<size_t>());
    MOCK_CONST_METHOD0(getLinearIndex, size_t());
    MOCK_CONST_METHOD1(isWithinBounds, bool(size_t));
  };
  GNU_DIAG_ON_SUGGEST_OVERRIDE
  class NormalizableMockIterator : public MockMDIterator {
  public:
    Mantid::signal_t getNormalizedSignal() const override {
      return this->getSignal() / static_cast<double>(this->getNumEvents());
    }
  };

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SignalRangeTest *createSuite() { return new SignalRangeTest(); }
  static void destroySuite(SignalRangeTest *suite) { delete suite; }

  void
  test_IMDWorkspace_Without_Function_Or_Normalization_Gives_Expected_Full_Range() {
    using namespace ::testing;

    int nthreads = PARALLEL_GET_MAX_THREADS;
    std::vector<std::unique_ptr<Mantid::API::IMDIterator>> iterators;
    iterators.reserve(nthreads);
    for (int i = 0; i < nthreads; ++i) {
      auto iterator = std::make_unique<MockMDIterator>();
      EXPECT_CALL(*iterator, valid()).WillRepeatedly(Return(true));
      EXPECT_CALL(*iterator, next())
          .WillOnce(Return(true))
          .WillRepeatedly(Return(false));
      EXPECT_CALL(*iterator, getNormalizedSignal())
          .WillOnce(Return(-1.5))
          .WillRepeatedly(Return(10.0));
      iterators.emplace_back(std::move(iterator));
    }

    MockMDWorkspace data;
    EXPECT_CALL(data, createIterators(nthreads, NULL))
        .Times(Exactly(1))
        .WillOnce(Return(ByMove(std::move(iterators))));

    MantidQt::API::SignalRange sr(data);
    QwtDoubleInterval range = sr.interval();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&data));

    TS_ASSERT_DELTA(-1.5, range.minValue(), 1e-10);
    TS_ASSERT_DELTA(10.0, range.maxValue(), 1e-10);
  }

  void test_IMDWorkspace_Uses_Specified_Normalization() {
    using namespace ::testing;

    int nthreads = PARALLEL_GET_MAX_THREADS;
    std::vector<std::unique_ptr<Mantid::API::IMDIterator>> iterators;
    iterators.reserve(nthreads);
    for (int i = 0; i < nthreads; ++i) {
      auto iterator = std::make_unique<NormalizableMockIterator>();
      EXPECT_CALL(*iterator, getNumEvents())
          .Times(Exactly(2))
          .WillRepeatedly(Return(2));
      EXPECT_CALL(*iterator, valid()).WillRepeatedly(Return(true));
      EXPECT_CALL(*iterator, next())
          .WillOnce(Return(true))
          .WillRepeatedly(Return(false));
      EXPECT_CALL(*iterator, getSignal())
          .WillOnce(Return(1.5))
          .WillRepeatedly(Return(10.0));
      iterators.emplace_back(std::move(iterator));
    }

    MockMDWorkspace data;
    EXPECT_CALL(data, createIterators(nthreads, NULL))
        .Times(Exactly(1))
        .WillOnce(Return(ByMove(std::move(iterators))));

    MantidQt::API::SignalRange sr(data, Mantid::API::NumEventsNormalization);
    QwtDoubleInterval range = sr.interval();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&data));

    TS_ASSERT_DELTA(0.75, range.minValue(), 1e-10);
    TS_ASSERT_DELTA(5.0, range.maxValue(), 1e-10);
  }

  void test_IMDWorkspace_With_Function_() {
    using namespace ::testing;

    int nthreads = PARALLEL_GET_MAX_THREADS;
    std::vector<std::unique_ptr<Mantid::API::IMDIterator>> iterators;
    iterators.reserve(nthreads);
    for (int i = 0; i < nthreads; ++i) {
      auto iterator = std::make_unique<NormalizableMockIterator>();
      EXPECT_CALL(*iterator, getNumEvents())
          .Times(Exactly(2))
          .WillRepeatedly(Return(2));
      EXPECT_CALL(*iterator, valid()).WillRepeatedly(Return(true));
      EXPECT_CALL(*iterator, next())
          .WillOnce(Return(true))
          .WillRepeatedly(Return(false));
      EXPECT_CALL(*iterator, getSignal())
          .WillOnce(Return(1.5))
          .WillRepeatedly(Return(10.0));
      iterators.emplace_back(std::move(iterator));
    }

    MockMDWorkspace data;
    Mantid::Geometry::MDImplicitFunction function;
    Mantid::coord_t normal[3] = {1234, 456, 678};
    Mantid::coord_t point[3] = {1, 2, 3};
    function.addPlane(Mantid::Geometry::MDPlane(3, normal, point));

    EXPECT_CALL(data, createIterators(nthreads, &function))
        .Times(Exactly(1))
        .WillOnce(Return(ByMove(std::move(iterators))));

    MantidQt::API::SignalRange sr(data, function, Mantid::API::NoNormalization);
    QwtDoubleInterval range = sr.interval();

    TS_ASSERT(Mock::VerifyAndClearExpectations(&data));

    TS_ASSERT_DELTA(0.75, range.minValue(), 1e-10);
    TS_ASSERT_DELTA(5.0, range.maxValue(), 1e-10);
  }
};

#endif /* MANTIDQT_API_SIGNALRANGETEST */
