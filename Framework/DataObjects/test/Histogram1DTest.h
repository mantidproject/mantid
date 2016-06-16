#ifndef TESTHISTOGRAM1D_
#define TESTHISTOGRAM1D_

#include <vector>
#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/Histogram1D.h"

using Mantid::DataObjects::Histogram1D;
using Mantid::MantidVec;

class Histogram1DTest : public CxxTest::TestSuite {
private:
  int nel;              // Number of elements in the array
  Histogram1D h, h2;    // Two histograms
  MantidVec x1, y1, e1; // vectors
  typedef boost::shared_ptr<MantidVec> parray;
  parray pa, pb; // Shared_ptr to vectors
public:
  void setUp() override {
    nel = 100;
    x1.resize(nel);
    std::fill(x1.begin(), x1.end(), rand());
    y1.resize(nel);
    std::fill(y1.begin(), y1.end(), rand());
    e1.resize(nel);
    pa = parray(new MantidVec(nel));
    std::fill(pa->begin(), pa->end(), rand());
    pb = parray(new MantidVec(nel));
    std::fill(pa->begin(), pa->end(), rand());
  }

  void testsetgetXvector() {
    h.setX(x1);
    TS_ASSERT_EQUALS(x1, h.dataX());
  }
  void testcopyX() {
    h2.setX(x1);
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
    h.setX(pa);
    TS_ASSERT_EQUALS(h.dataX(), *pa);
  }
  void testsetgetDataYPointer() {
    h.setData(pa);
    TS_ASSERT_EQUALS(h.dataY(), *pa);
  }
  void testsetgetDataYEPointer() {
    h.setData(pa, pb);
    TS_ASSERT_EQUALS(h.dataY(), *pa);
    TS_ASSERT_EQUALS(h.dataE(), *pb);
  }
  void testgetXindex() {
    h.setX(x1);
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
    //	  h.setX(x1);
    //	  h.setData(y1,e1);
    //	  double* xye;
    //	  xye=h[0];
    //	  TS_ASSERT_EQUALS(*xye,x1[0]);
    //	  TS_ASSERT_EQUALS(*(xye+1),y1[0]);
    //	  TS_ASSERT_EQUALS(*(xye+2),e1[0]);
  }

  void testrangeexceptionX() {
    h.setX(x1);
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

  void test_copy_constructor() {
    const Histogram1D source;
    Histogram1D clone(source);
    TS_ASSERT_EQUALS(&clone.readX(), &source.readX());
    TS_ASSERT_EQUALS(&clone.readY(), &source.readY());
    TS_ASSERT_EQUALS(&clone.readE(), &source.readE());
  }

  void test_move_constructor() {
    Histogram1D source;
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
    Histogram1D resource;
    resource.dataX() = {0.1};
    resource.dataY() = {0.2};
    resource.dataE() = {0.3};
    const Mantid::API::ISpectrum &source = resource;
    Histogram1D clone(source);
    // X is shared...
    TS_ASSERT_EQUALS(&clone.readX(), &source.readX());
    // .. but not Y and E, since they are not part of ISpectrum.
    TS_ASSERT_DIFFERS(&clone.readY(), &source.readY());
    TS_ASSERT_DIFFERS(&clone.readE(), &source.readE());
    TS_ASSERT_EQUALS(clone.readX()[0], 0.1);
    TS_ASSERT_EQUALS(clone.readY()[0], 0.2);
    TS_ASSERT_EQUALS(clone.readE()[0], 0.3);
  }

  void test_copy_assignment() {
    const Histogram1D source;
    Histogram1D clone;
    clone = source;
    TS_ASSERT_EQUALS(&clone.readX(), &source.readX());
    TS_ASSERT_EQUALS(&clone.readY(), &source.readY());
    TS_ASSERT_EQUALS(&clone.readE(), &source.readE());
  }

  void test_move_assignment() {
    Histogram1D source;
    auto oldX = &source.readX();
    auto oldY = &source.readY();
    auto oldE = &source.readE();
    Histogram1D clone;
    clone = std::move(source);
    TS_ASSERT(!source.ptrX());
    TS_ASSERT_EQUALS(&clone.readX(), oldX);
    TS_ASSERT_EQUALS(&clone.readY(), oldY);
    TS_ASSERT_EQUALS(&clone.readE(), oldE);
  }

  void test_assign_ISpectrum() {
    Histogram1D resource;
    resource.dataX() = {0.1};
    resource.dataY() = {0.2};
    resource.dataE() = {0.3};
    const Mantid::API::ISpectrum &source = resource;
    Histogram1D clone;
    clone = source;
    // X is shared...
    TS_ASSERT_EQUALS(&clone.readX(), &source.readX());
    // .. but not Y and E, since they are not part of ISpectrum.
    TS_ASSERT_DIFFERS(&clone.readY(), &source.readY());
    TS_ASSERT_DIFFERS(&clone.readE(), &source.readE());
    TS_ASSERT_EQUALS(clone.readX()[0], 0.1);
    TS_ASSERT_EQUALS(clone.readY()[0], 0.2);
    TS_ASSERT_EQUALS(clone.readE()[0], 0.3);
  }
};
#endif /*TESTHISTOGRAM1D_*/
