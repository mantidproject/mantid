#ifndef TESTHISTOGRAM1D_
#define TESTHISTOGRAM1D_

#include <vector> 
#include <algorithm> 
#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/Histogram1D.h" 

using Mantid::DataObjects::Histogram1D;

class Histogram1DTest : public CxxTest::TestSuite
{
private: 
  int nel; // Number of elements in the array
  Histogram1D h, h2; // Two histograms
  MantidVec x1,y1,e1; // vectors 
  typedef boost::shared_ptr<MantidVec > parray;
  parray pa, pb; // Shared_ptr to vectors
public:
  Histogram1DTest()
  {
    nel=100;
    x1.resize(nel);
    std::fill(x1.begin(),x1.end(),rand());	
    y1.resize(nel);
    std::fill(y1.begin(),y1.end(),rand());
    e1.resize(nel);
    pa=parray(new MantidVec(nel));
    std::fill(pa->begin(),pa->end(),rand());
    pb=parray(new MantidVec(nel));
    std::fill(pa->begin(),pa->end(),rand());
  }
  void testsetgetXvector()
  {
    h.setX(x1);
    TS_ASSERT_EQUALS(x1,h.dataX());
  }
  void testcopyX()
  {
    h2.setX(x1);
    h.dataX()=h2.dataX();
    TS_ASSERT_EQUALS(h.dataX(),x1);
  }
  void testsetgetDataYVector()
  {
    h.setData(y1);
    TS_ASSERT_EQUALS(h.dataY(),y1);
  }
  void testsetgetDataYEVector()
  {
    h.setData(y1,e1);
    TS_ASSERT_EQUALS(h.dataY(),y1);
    TS_ASSERT_EQUALS(h.dataE(),e1);
  }
  void testsetgetXPointer()
  {
    h.setX(pa);
    TS_ASSERT_EQUALS(h.dataX(),*pa);
  }
  void testsetgetDataYPointer()
  {
    h.setData(pa);
    TS_ASSERT_EQUALS(h.dataY(),*pa);
  }
  void testsetgetDataYEPointer()
  {
    h.setData(pa,pb);
    TS_ASSERT_EQUALS(h.dataY(),*pa);
    TS_ASSERT_EQUALS(h.dataE(),*pb);
  }
  void testgetXindex()
  {
    h.setX(x1);
    TS_ASSERT_EQUALS(h.dataX()[4],x1[4]);
  }
  void testgetYindex()
  {
    h.setData(y1);
    TS_ASSERT_EQUALS(h.dataY()[4],y1[4]);
  }
  void testgetEindex()
  {
    h.setData(y1,e1);
    TS_ASSERT_EQUALS(h.dataE()[4],e1[4]);
  } 
  void testoperatorbracket()
  {
    //	  h.setX(x1);
    //	  h.setData(y1,e1);
    //	  double* xye;
    //	  xye=h[0];
    //	  TS_ASSERT_EQUALS(*xye,x1[0]);
    //	  TS_ASSERT_EQUALS(*(xye+1),y1[0]);
    //	  TS_ASSERT_EQUALS(*(xye+2),e1[0]);
  }
  void testnxbin()
  {
    h.setX(x1);
    TS_ASSERT_EQUALS(h.nxbin(),x1.size());
  }
  void testnybin()
  {
    h.setData(y1);
    TS_ASSERT_EQUALS(h.nybin(),y1.size());
  }
  void testrangeexceptionX()
  {
    h.setX(x1);
    //	  TS_ASSERT_THROWS(h.dataX().at(-1),const std::exception&);
    TS_ASSERT_THROWS(h.dataX().at(nel),const std::exception&);
  }
  void testrangeexceptionY()
  {
    h.setData(y1);
    //	  TS_ASSERT_THROWS(h.dataY().at(-1),const std::exception&);
    TS_ASSERT_THROWS(h.dataY().at(nel),const std::exception&);
  }
  void testrangeexceptionE()
  {
    h.setData(y1,e1);
    //	    TS_ASSERT_THROWS(h.dataE().at(-1),const std::exception&);
    TS_ASSERT_THROWS(h.dataE().at(nel),const std::exception&);
  }
  void testsetdatadifferentsizesException()
  {
    //	    e1.resize(nel+1);
    // TS_ASSERT_THROWS(h.setData(y1,e1),const std::invalid_argument&);
    //pb.reset();
    //pb=MantidVecPtr::(new std::vector<double>(nel+1));
    //TS_ASSERT_THROWS(h.setData(pa,pb),const std::invalid_argument&);
  }

};
#endif /*TESTHISTOGRAM1D_*/
