#ifndef UNITTEST_H_
#define UNITTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Unit.h"

using namespace Mantid::Kernel;

class UnitTest : public CxxTest::TestSuite
{
  class UnitTester : public Unit
  {
  public:
    UnitTester() : Unit() {}
    virtual ~UnitTester() {}
    
    // Empty overrides of virtual methods
    const int unitCode() const {return 0;}
    const std::string caption() const {return "";}
    const std::string label() const {return "";}
    void toTOF(std::vector<double>&,std::vector<double>&,const double&,const double&,const double&,const int&,const double&,const double&) const {}
    void fromTOF(std::vector<double>&,std::vector<double>&,const double&,const double&,const double&,const int&,const double&,const double&) const {}
  };
  
public:
  
  //----------------------------------------------------------------------
  // Base Unit class tests
  //----------------------------------------------------------------------
  
	void testUnit_GetSetDescription()
	{
	  UnitTester t;
    TS_ASSERT_EQUALS( t.description(), "" )
    t.setDescription("testing");
    TS_ASSERT_EQUALS( t.description(), "testing" )
	}

  //----------------------------------------------------------------------
  // TOF tests
  //----------------------------------------------------------------------

	void testTOF_unitCode()
	{
		TS_ASSERT_EQUALS( tof.unitCode(), 1 )
	}

	void testTOF_caption()
	{
		TS_ASSERT_EQUALS( tof.caption(), "Time-of-flight" )
	}

	void testTOF_label()
	{
	  TS_ASSERT_EQUALS( tof.label(), "microsecond" )
	}
	
	void testTOF_cast()
	{
	  TS_ASSERT_THROWS_NOTHING( dynamic_cast<Unit&>(tof) )
	}

	void testTOF_toTOF()
	{
	  std::vector<double> x(20,9.9), y(20,8.8);
	  std::vector<double> xx = x;
    std::vector<double> yy = y;
	  TS_ASSERT_THROWS_NOTHING( tof.toTOF(x,y,1.0,1.0,1.0,1,1.0,1.0) )
	  // Check vectors are unchanged
	  TS_ASSERT( xx == x )
	  TS_ASSERT( yy == y )
	}

	void testTOF_fromTOF()
	{
    std::vector<double> x(20,9.9), y(20,8.8);
    std::vector<double> xx = x;
    std::vector<double> yy = y;
    TS_ASSERT_THROWS_NOTHING( tof.fromTOF(x,y,1.0,1.0,1.0,1,1.0,1.0) )
    // Check vectors are unchanged
    TS_ASSERT( xx == x )
    TS_ASSERT( yy == y )
	}

  //----------------------------------------------------------------------
  // Wavelength tests
  //----------------------------------------------------------------------

	void testWavelength_unitCode()
	{
    TS_ASSERT_EQUALS( lambda.unitCode(), 2 )
	}

	void testWavelength_caption()
	{
    TS_ASSERT_EQUALS( lambda.caption(), "Wavelength" )
	}

	void testWavelength_label()
	{
    TS_ASSERT_EQUALS( lambda.label(), "Angstrom" )
	}

	void testWavelength_cast()
  {
    TS_ASSERT_THROWS_NOTHING( dynamic_cast<Unit&>(lambda) )
  }

	void testWavelength_toTOF()
	{
    std::vector<double> x(1,1.0), y(1,1.0);
    std::vector<double> yy = y;
    TS_ASSERT_THROWS_NOTHING( lambda.toTOF(x,y,1.0,1.0,1.0,1,1.0,1.0) )
    TS_ASSERT_DELTA( x[0], 505.5568, 0.0001 )
    TS_ASSERT( yy == y )
	}

	void testWavelength_fromTOF()
	{
    std::vector<double> x(1,1.0), y(1,1.0);
    std::vector<double> yy = y;
    TS_ASSERT_THROWS_NOTHING( lambda.fromTOF(x,y,1.0,1.0,1.0,1,1.0,1.0) )
    TS_ASSERT_DELTA( x[0], 0.001978017, 0.000000001 )
    TS_ASSERT( yy == y )
	}

private:
  Units::TOF tof;
  Units::Wavelength lambda;
	
};

#endif /*UNITTEST_H_*/
