// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <algorithm>
#include <cxxtest/TestSuite.h>
#include <memory>
#include <vector>

#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/Histogram1D.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/WarningSuppressions.h"

using namespace Mantid;
using namespace API;
using namespace Kernel;
using namespace DataObjects;
using namespace HistogramData;

class Histogram1DTest : public CxxTest::TestSuite {
private:
  int nel; // Number of elements in the array
  Histogram1D h{Histogram::XMode::Points, Histogram::YMode::Counts};
  Histogram1D h2{Histogram::XMode::Points, Histogram::YMode::Counts};
  MantidVec x1, y1, e1; // vectors
  std::shared_ptr<HistogramY> pa;
  std::shared_ptr<HistogramE> pb;

public:
  void setUp() override {
    nel = 100;
    x1.resize(nel);
    std::generate(x1.begin(), x1.end(), LinearGenerator(0.1, 0.01));
    y1.resize(nel);
    std::fill(y1.begin(), y1.end(), rand());
    e1.resize(nel);
    pa = std::make_shared<HistogramY>(nel);
    std::fill(pa->begin(), pa->end(), rand());
    pb = std::make_shared<HistogramE>(nel);
    std::fill(pb->begin(), pb->end(), rand());
    h.setHistogram(Histogram(Points(100, LinearGenerator(0.0, 1.0)), Counts(100, 0.0), CountVariances(100, 0.0)));
    h2.setHistogram(Histogram(Points(100, LinearGenerator(0.0, 1.0)), Counts(100, 0.0), CountVariances(100, 0.0)));
    h.setCounts(100);
    h.setCountStandardDeviations(100);
    h2.setCounts(100);
    h2.setCountStandardDeviations(100);
  }

  void test_copyDataFrom() {
    Histogram1D histogram{Histogram::XMode::Points, Histogram::YMode::Counts};
    histogram.setHistogram(Points(1), Counts(1));
    EventList eventList;
    eventList.setHistogram(BinEdges(2));
    std::unique_ptr<const ISpectrum> specHist = std::make_unique<Histogram1D>(histogram);
    std::unique_ptr<const ISpectrum> specEvent = std::make_unique<EventList>(eventList);
    std::unique_ptr<ISpectrum> target =
        std::make_unique<Histogram1D>(Histogram::XMode::Points, Histogram::YMode::Counts);

    TS_ASSERT_THROWS_NOTHING(target->copyDataFrom(*specHist));
    TS_ASSERT(target->points());
    TS_ASSERT_EQUALS(&target->points()[0], &histogram.points()[0]);

    TS_ASSERT_THROWS_NOTHING(target->copyDataFrom(*specEvent));
    TS_ASSERT(target->binEdges());
    TS_ASSERT_EQUALS(&target->binEdges()[0], &eventList.binEdges()[0]);
  }

  void test_copyDataFrom_does_not_copy_indices() {
    Histogram1D histogram{Histogram::XMode::Points, Histogram::YMode::Counts};
    histogram.setHistogram(Points(1), Counts(1));
    EventList eventList;
    eventList.setHistogram(BinEdges(2));
    std::unique_ptr<const ISpectrum> specHist = std::make_unique<Histogram1D>(histogram);
    std::unique_ptr<const ISpectrum> specEvent = std::make_unique<EventList>(eventList);
    std::unique_ptr<ISpectrum> target =
        std::make_unique<Histogram1D>(Histogram::XMode::Points, Histogram::YMode::Counts);
    target->setSpectrumNo(37);
    target->setDetectorID(42);

    TS_ASSERT_THROWS_NOTHING(target->copyDataFrom(*specHist));
    TS_ASSERT(target->points());
    TS_ASSERT_EQUALS(&target->points()[0], &histogram.points()[0]);
    TS_ASSERT_EQUALS(target->getSpectrumNo(), 37);
    TS_ASSERT_EQUALS(target->getDetectorIDs(), std::set<detid_t>{42});

    TS_ASSERT_THROWS_NOTHING(target->copyDataFrom(*specEvent));
    TS_ASSERT(target->binEdges());
    TS_ASSERT_EQUALS(&target->binEdges()[0], &eventList.binEdges()[0]);
    TS_ASSERT_EQUALS(target->getSpectrumNo(), 37);
    TS_ASSERT_EQUALS(target->getDetectorIDs(), std::set<detid_t>{42});
  }

  void testcheckAndSanitizeHistogramThrowsNullY() {
    Histogram1D h{Histogram::XMode::Points, Histogram::YMode::Counts};
    BinEdges edges{-0.04, 1.7};
    TS_ASSERT_THROWS(h.setHistogram(edges), const std::invalid_argument &);
  }

  void testcheckAndSanitizeHistogramThrowsNullE() {
    Histogram1D h{Histogram::XMode::Points, Histogram::YMode::Counts};
    BinEdges edges{-0.04, 1.7};
    Histogram histogram{edges};
    Counts counts{23};
    histogram.setCounts(counts);
    TS_ASSERT_THROWS(h.setHistogram(histogram), const std::invalid_argument &);
  }

  void testsetgetXvector() {
    h.setPoints(x1);
    TS_ASSERT_EQUALS(x1, h.dataX());
  }
  void testcopyX() {
    h2.setPoints(x1);
    h.dataX() = h2.dataX();
    TS_ASSERT_EQUALS(h.dataX(), x1);
  }
  void testsetgetDataYVector() {
    h.setCounts(y1);
    TS_ASSERT_EQUALS(h.dataY(), y1);
  }
  void testsetgetDataYEVector() {
    h.setCounts(y1);
    h.setCountStandardDeviations(e1);
    TS_ASSERT_EQUALS(h.dataY(), y1);
    TS_ASSERT_EQUALS(h.dataE(), e1);
  }
  void testmaskSpectrum() {
    h.clearData();
    TS_ASSERT_EQUALS(h.dataY()[5], 0.0);
    TS_ASSERT_EQUALS(h.dataE()[12], 0.0);
  }
  void testsetgetXPointer() {
    auto px = std::make_shared<HistogramX>(0);
    h.setX(px);
    TS_ASSERT_EQUALS(&(*h.ptrX()), &(*px));
  }
  void testsetgetDataYPointer() {
    h.setCounts(pa);
    TS_ASSERT_EQUALS(h.dataY(), pa->rawData());
  }
  void testsetgetDataYEPointer() {
    h.setCounts(pa);
    h.setCountStandardDeviations(pb);
    TS_ASSERT_EQUALS(h.dataY(), pa->rawData());
    TS_ASSERT_EQUALS(h.dataE(), pb->rawData());
  }
  void testgetXindex() {
    h.setPoints(x1);
    TS_ASSERT_EQUALS(h.dataX()[4], x1[4]);
  }
  void testgetYindex() {
    h.setCounts(y1);
    TS_ASSERT_EQUALS(h.dataY()[4], y1[4]);
  }
  void testgetEindex() {
    h.setCounts(y1);
    h.setCountStandardDeviations(e1);
    TS_ASSERT_EQUALS(h.dataE()[4], e1[4]);
  }
  void testrangeexceptionX() {
    h.setPoints(x1);
    TS_ASSERT_THROWS(h.dataX().at(nel), const std::out_of_range &);
  }
  void testrangeexceptionY() {
    h.setCounts(y1);
    TS_ASSERT_THROWS(h.dataY().at(nel), const std::out_of_range &);
  }
  void testrangeexceptionE() {
    h.setCounts(y1);
    h.setCountStandardDeviations(e1);
    TS_ASSERT_THROWS(h.dataE().at(nel), const std::out_of_range &);
  }

  void test_copy_constructor() {
    const Histogram1D source(Histogram::XMode::Points, Histogram::YMode::Counts);
    Histogram1D clone(source);
    TS_ASSERT_EQUALS(&clone.readX(), &source.readX());
    TS_ASSERT_EQUALS(&clone.readY(), &source.readY());
    TS_ASSERT_EQUALS(&clone.readE(), &source.readE());
  }

  void test_move_constructor() {
    Histogram1D source(Histogram::XMode::Points, Histogram::YMode::Counts);
    auto oldX = &source.readX();
    auto oldY = &source.readY();
    auto oldE = &source.readE();
    Histogram1D clone(std::move(source));
    TS_ASSERT(!source.ptrX());
    TS_ASSERT_EQUALS(&clone.readX(), oldX);
    TS_ASSERT_EQUALS(&clone.readY(), oldY);
    TS_ASSERT_EQUALS(&clone.readE(), oldE);
  }

  void test_constructor_from_ISpectrum() {
    Histogram1D resource(Histogram::XMode::Points, Histogram::YMode::Counts);
    resource.dataX() = {0.1};
    resource.dataY() = {0.2};
    resource.dataE() = {0.3};
    const Mantid::API::ISpectrum &source = resource;
    Histogram1D clone(source);
    // X is shared...
    TS_ASSERT_EQUALS(&clone.readX(), &source.readX());
    // Y and E are in general not shared, since they are not part of ISpectrum,
    // but in this special case ISpectrum references Histogram1D, so they
    // should.
    TS_ASSERT_EQUALS(&clone.readY(), &source.readY());
    TS_ASSERT_EQUALS(&clone.readE(), &source.readE());
    TS_ASSERT_EQUALS(clone.readX()[0], 0.1);
    TS_ASSERT_EQUALS(clone.readY()[0], 0.2);
    TS_ASSERT_EQUALS(clone.readE()[0], 0.3);
  }

  void test_copy_assignment() {
    const Histogram1D source(Histogram::XMode::Points, Histogram::YMode::Counts);
    Histogram1D clone(Histogram::XMode::Points, Histogram::YMode::Counts);
    clone = source;
    TS_ASSERT_EQUALS(&clone.readX(), &source.readX());
    TS_ASSERT_EQUALS(&clone.readY(), &source.readY());
    TS_ASSERT_EQUALS(&clone.readE(), &source.readE());
  }

  void test_move_assignment() {
    Histogram1D source(Histogram::XMode::Points, Histogram::YMode::Counts);
    auto oldX = &source.readX();
    auto oldY = &source.readY();
    auto oldE = &source.readE();
    Histogram1D clone(Histogram::XMode::Points, Histogram::YMode::Counts);
    clone = std::move(source);
    TS_ASSERT(!source.ptrX());
    TS_ASSERT_EQUALS(&clone.readX(), oldX);
    TS_ASSERT_EQUALS(&clone.readY(), oldY);
    TS_ASSERT_EQUALS(&clone.readE(), oldE);
  }

  void test_assign_ISpectrum() {
    Histogram1D resource(Histogram::XMode::Points, Histogram::YMode::Counts);
    resource.dataX() = {0.1};
    resource.dataY() = {0.2};
    resource.dataE() = {0.3};
    const Mantid::API::ISpectrum &source = resource;
    Histogram1D clone(Histogram::XMode::Points, Histogram::YMode::Counts);
    clone = source;
    // X is shared...
    TS_ASSERT_EQUALS(&clone.readX(), &source.readX());
    // Y and E are in general not shared, since they are not part of ISpectrum,
    // but in this special case ISpectrum references Histogram1D, so they
    // should.
    TS_ASSERT_EQUALS(&clone.readY(), &source.readY());
    TS_ASSERT_EQUALS(&clone.readE(), &source.readE());
    TS_ASSERT_EQUALS(clone.readX()[0], 0.1);
    TS_ASSERT_EQUALS(clone.readY()[0], 0.2);
    TS_ASSERT_EQUALS(clone.readE()[0], 0.3);
  }
};
