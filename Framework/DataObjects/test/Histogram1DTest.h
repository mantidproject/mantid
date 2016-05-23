#ifndef TESTHISTOGRAM1D_
#define TESTHISTOGRAM1D_

#include <vector>
#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/Histogram1D.h"

using Mantid::DataObjects::Histogram1D;
using Mantid::MantidVec;
using Mantid::Kernel::make_cow;
using namespace Mantid::HistogramData;

class Histogram1DTest : public CxxTest::TestSuite {
private:
  int nel; // Number of elements in the array
  Histogram1D h{Histogram::XMode::Points};
  Histogram1D h2{Histogram::XMode::Points};
  MantidVec x1, y1, e1; // vectors
  boost::shared_ptr<HistogramY> pa;
  boost::shared_ptr<HistogramE> pb;
public:
  void setUp() override {
    nel = 100;
    x1.resize(nel);
    std::fill(x1.begin(), x1.end(), rand());
    y1.resize(nel);
    std::fill(y1.begin(), y1.end(), rand());
    e1.resize(nel);
    pa = boost::make_shared<HistogramY>(nel);
    std::fill(pa->begin(), pa->end(), rand());
    pb = boost::make_shared<HistogramE>(nel);
    std::fill(pb->begin(), pb->end(), rand());
    h.setHistogram(Histogram(Points(100)));
    h2.setHistogram(Histogram(Points(100)));
    h.setData(make_cow<HistogramY>(100), make_cow<HistogramE>(100));
    h2.setData(make_cow<HistogramY>(100), make_cow<HistogramE>(100));
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
    h.setData(y1);
    TS_ASSERT_EQUALS(h.dataY(), y1);
  }
  void testsetgetDataYEVector() {
    h.setData(y1, e1);
    TS_ASSERT_EQUALS(h.dataY(), y1);
    TS_ASSERT_EQUALS(h.dataE(), e1);
  }
  void testmaskSpectrum() {
    h.clearData();
    TS_ASSERT_EQUALS(h.dataY()[5], 0.0);
    TS_ASSERT_EQUALS(h.dataE()[12], 0.0);
  }
  void testsetgetXPointer() {
    auto px = boost::make_shared<HistogramX>(0);
    h.setX(px);
    TS_ASSERT_EQUALS(&(*h.ptrX()), &(*px));
  }
  void testsetgetDataYPointer() {
    h.setData(pa);
    TS_ASSERT_EQUALS(h.dataY(), pa->rawData());
  }
  void testsetgetDataYEPointer() {
    h.setData(pa, pb);
    TS_ASSERT_EQUALS(h.dataY(), pa->rawData());
    TS_ASSERT_EQUALS(h.dataE(), pb->rawData());
  }
  void testgetXindex() {
    h.setPoints(x1);
    TS_ASSERT_EQUALS(h.dataX()[4], x1[4]);
  }
  void testgetYindex() {
    h.setData(y1);
    TS_ASSERT_EQUALS(h.dataY()[4], y1[4]);
  }
  void testgetEindex() {
    h.setData(y1, e1);
    TS_ASSERT_EQUALS(h.dataE()[4], e1[4]);
  }
  void testoperatorbracket() {
    //	  h.setPoints(x1);
    //	  h.setData(y1,e1);
    //	  double* xye;
    //	  xye=h[0];
    //	  TS_ASSERT_EQUALS(*xye,x1[0]);
    //	  TS_ASSERT_EQUALS(*(xye+1),y1[0]);
    //	  TS_ASSERT_EQUALS(*(xye+2),e1[0]);
  }

  void testrangeexceptionX() {
    h.setPoints(x1);
    TS_ASSERT_THROWS(h.dataX().at(nel), std::out_of_range);
  }
  void testrangeexceptionY() {
    h.setData(y1);
    TS_ASSERT_THROWS(h.dataY().at(nel), std::out_of_range);
  }
  void testrangeexceptionE() {
    h.setData(y1, e1);
    TS_ASSERT_THROWS(h.dataE().at(nel), std::out_of_range);
  }
};
#endif /*TESTHISTOGRAM1D_*/
