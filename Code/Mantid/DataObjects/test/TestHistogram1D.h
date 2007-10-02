#ifndef TESTHISTOGRAM1D_
#define TESTHISTOGRAM1D_
#include "Histogram1D.h" 
#include <cxxtest/TestSuite.h>
#include <vector> 
#include <algorithm> 
#include <boost/shared_ptr.hpp>

class testHistogram1D : public CxxTest::TestSuite
{
private: 
	int nel; // Number of elements in the array
  Mantid::Histogram1D h, h2; // Two histograms
  std::vector<double> x1,y1,e1; // vectors 
  typedef boost::shared_ptr<std::vector<double> > parray;
  parray pa, pb; // Shared_ptr to vectors
public:
  testHistogram1D()
	{
		nel=100;
		x1.resize(nel);
		std::fill(x1.begin(),x1.end(),rand());	
		y1.resize(nel);
		std::fill(y1.begin(),y1.end(),rand());
		e1.resize(nel);
		pa=parray(new std::vector<double>(nel));
		std::fill(pa->begin(),pa->end(),rand());
		pb=parray(new std::vector<double>(nel));
		std::fill(pa->begin(),pa->end(),rand());
	}
	void testsetgetXvector()
	{
		h.setX(x1);
		TS_ASSERT_EQUALS(x1,h.getX());
	}
	void testcopyX()
	{
		h2.setX(x1);
		h.copyX(h2);
		TS_ASSERT_EQUALS(h.getX(),x1);
	}
	void testsetgetDataYVector()
	{
		h.setData(y1);
		TS_ASSERT_EQUALS(h.getY(),y1);
	}
	void testsetgetDataYEVector()
	{
		h.setData(y1,e1);
		TS_ASSERT_EQUALS(h.getY(),y1);
		TS_ASSERT_EQUALS(h.getE(),e1);
	}
	void testsetgetXPointer()
	{
		h.setX(pa);
		TS_ASSERT_EQUALS(h.getX(),*pa);
	}
	void testsetgetDataYPointer()
	{
		h.setData(pa);
		TS_ASSERT_EQUALS(h.getY(),*pa);
	}
	void testsetgetDataYEPointer()
	{
		h.setData(pa,pb);
		TS_ASSERT_EQUALS(h.getY(),*pa);
		TS_ASSERT_EQUALS(h.getE(),*pb);
	}
	void testgetXindex()
	{
	 h.setX(x1);
	 TS_ASSERT_EQUALS(h.getX(4),x1[4]);
	}
	void testgetYindex()
	{
	h.setData(y1);
	TS_ASSERT_EQUALS(h.getY(4),y1[4]);
	}
	void testgetEindex()
	{
	h.setData(y1,e1);
	TS_ASSERT_EQUALS(h.getE(4),e1[4]);
	} 
	void testoperatorbracket()
	{
	h.setX(x1);
	h.setData(y1,e1);
	double* xye;
	xye=h[0];
	TS_ASSERT_EQUALS(*xye,x1[0]);
	TS_ASSERT_EQUALS(*(xye+1),y1[0]);
	TS_ASSERT_EQUALS(*(xye+2),e1[0]);
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
		TS_ASSERT_THROWS(h.getX(-1),const std::runtime_error&);
		TS_ASSERT_THROWS(h.getX(nel),const std::runtime_error&);
	}
	void testrangeexceptionY()
	{
		h.setData(y1);
		TS_ASSERT_THROWS(h.getY(-1),const std::runtime_error&);
		TS_ASSERT_THROWS(h.getY(nel),const std::runtime_error&);
	}
	void testrangeexceptionE()
	{
		h.setData(y1,e1);
		TS_ASSERT_THROWS(h.getE(-1),const std::runtime_error&);
		TS_ASSERT_THROWS(h.getE(nel),const std::runtime_error&);
	}
	void testrangeexceptionoperatorbracket()
	{
		h.setX(x1);
		h.setData(y1,e1);
		TS_ASSERT_THROWS(h[-1],const std::runtime_error&);
		TS_ASSERT_THROWS(h[nel],const std::runtime_error&);
	}
	void testsetdatadifferentsizesException()
	{
		e1.resize(nel+1);
		TS_ASSERT_THROWS(h.setData(y1,e1),const std::runtime_error&);
		pb.reset();
		pb=parray(new std::vector<double>(nel+1));
		TS_ASSERT_THROWS(h.setData(pa,pb),const std::runtime_error&);
	}
	
	
	
	
};
#endif /*TESTHISTOGRAM1D_*/
