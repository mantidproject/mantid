#ifndef UNITTEST_H_
#define UNITTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Unit.h"
#include "MantidKernel/PhysicalConstants.h"

using namespace Mantid::Kernel;

class UnitTest : public CxxTest::TestSuite
{
  class UnitTester : public Unit
  {
public:
    UnitTester() : Unit()
    {
      addConversion("a", 1.1);
      addConversion("b", 2.2, 0.5);
    }
    virtual ~UnitTester() {}

    // Empty overrides of virtual methods
    const std::string unitID() const {return "aUnit";}
    const std::string caption() const {return "";}
    const std::string label() const{return "";}
    void toTOF(std::vector<double>&, std::vector<double>&, const double&, const double&, const double&,const int&, const double&, const double&) const {}
    void fromTOF(std::vector<double>&, std::vector<double>&, const double&, const double&, const double&, const int&, const double&, const double&) const {}
  };

public:

  //----------------------------------------------------------------------
  // Base Unit class tests
  //----------------------------------------------------------------------

  void testUnit_quickConversion()
  {
    UnitTester t;
    double factor;
    double power;
    TS_ASSERT( t.quickConversion("a",factor,power) );
    TS_ASSERT_EQUALS( factor, 1.1 );
    TS_ASSERT_EQUALS( power, 1.0 );
    TS_ASSERT( t.quickConversion("b",factor,power) );
    TS_ASSERT_EQUALS( factor, 2.2 );
    TS_ASSERT_EQUALS( power, 0.5 );
    TS_ASSERT( ! t.quickConversion("notThere",factor,power) );

    // Test the quickConversion method that takes a Unit
    Units::TOF tof;
    TS_ASSERT( ! t.quickConversion(tof,factor,power) );
  }
    
  //----------------------------------------------------------------------
  // TOF tests
  //----------------------------------------------------------------------

  void testTOF_unitID()
  {
    TS_ASSERT_EQUALS( tof.unitID(), "TOF" );
  }

  void testTOF_caption()
  {
    TS_ASSERT_EQUALS( tof.caption(), "Time-of-flight" );
  }

  void testTOF_label()
  {
    TS_ASSERT_EQUALS( tof.label(), "microsecond" )
  }

  void testTOF_cast()
  {
    Unit *u = NULL;
    TS_ASSERT_THROWS_NOTHING( u = dynamic_cast<Unit*>(&tof) );
    TS_ASSERT_EQUALS(u->unitID(), "TOF");
  }

  void testTOF_toTOF()
  {
    std::vector<double> x(20, 9.9), y(20, 8.8);
    std::vector<double> xx = x;
    std::vector<double> yy = y;
    TS_ASSERT_THROWS_NOTHING( tof.toTOF(x,y,1.0,1.0,1.0,1,1.0,1.0) )
    // Check vectors are unchanged
    TS_ASSERT( xx == x )
    TS_ASSERT( yy == y )
  }

  void testTOF_fromTOF()
  {
    std::vector<double> x(20, 9.9), y(20, 8.8);
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

  void testWavelength_unitID()
  {
    TS_ASSERT_EQUALS( lambda.unitID(), "Wavelength" )
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
    Unit *u = NULL;
    TS_ASSERT_THROWS_NOTHING( u = dynamic_cast<Unit*>(&lambda) );
    TS_ASSERT_EQUALS(u->unitID(), "Wavelength");
  }

  void testWavelength_toTOF()
  {
    std::vector<double> x(1, 1.5), y(1, 1.5);
    std::vector<double> yy = y;
    TS_ASSERT_THROWS_NOTHING( lambda.toTOF(x,y,1.0,1.0,1.0,1,1.0,1.0) )
    TS_ASSERT_DELTA( x[0], 2665.4390, 0.0001 ) //  758.3352
    TS_ASSERT( yy == y )
  }

  void testWavelength_fromTOF()
  {
    std::vector<double> x(1, 1000.5), y(1, 1.5);
    std::vector<double> yy = y;
    TS_ASSERT_THROWS_NOTHING( lambda.fromTOF(x,y,1.0,1.0,1.0,1,1.0,1.0) )
    TS_ASSERT_DELTA( x[0], -5.0865, 0.0001 ) // 1.979006
    TS_ASSERT( yy == y )
  }

  void testWavelength_quickConversions()
  {
    // Test it gives the same answer as going 'the long way'
    double factor, power;
    TS_ASSERT( lambda.quickConversion(energy,factor,power) )
    double input = 1.1;
    double result = factor * std::pow(input,power);
    std::vector<double> x(1,input);
    lambda.toTOF(x,x,99.0,99.0,99.0,99,99.0,99.0);
    energy.fromTOF(x,x,99.0,99.0,99.0,99,99.0,99.0);
    TS_ASSERT_DELTA( x[0], result, 1.0e-10 )

    TS_ASSERT( lambda.quickConversion(energyk,factor,power) )
    double result2 = factor * std::pow(input,power);
    TS_ASSERT_EQUALS( result2/result, Mantid::PhysicalConstants::meVtoWavenumber )
    std::vector<double> x2(1,input);
    lambda.toTOF(x2,x2,99.0,99.0,99.0,99,99.0,99.0);
    energyk.fromTOF(x2,x2,99.0,99.0,99.0,99,99.0,99.0);
    TS_ASSERT_DELTA( x2[0], result2, 1.0e-10 )
  }

  //----------------------------------------------------------------------
  // Energy tests
  //----------------------------------------------------------------------

  void testEnergy_unitID()
  {
    TS_ASSERT_EQUALS( energy.unitID(), "Energy" )
  }

  void testEnergy_caption()
  {
    TS_ASSERT_EQUALS( energy.caption(), "Energy" )
  }

  void testEnergy_label()
  {
    TS_ASSERT_EQUALS( energy.label(), "meV" )
  }

  void testEnergy_cast()
  {
    Unit *u = NULL;
    TS_ASSERT_THROWS_NOTHING( u = dynamic_cast<Unit*>(&energy) );
    TS_ASSERT_EQUALS(u->unitID(), "Energy");
  }

  void testEnergy_toTOF()
  {
    std::vector<double> x(1, 4.0), y(1, 1.0);
    std::vector<double> yy = y;
    TS_ASSERT_THROWS_NOTHING( energy.toTOF(x,y,1.0,1.0,1.0,1,1.0,1.0) )
    TS_ASSERT_DELTA( x[0], 2286.271, 0.001 )
    TS_ASSERT( yy == y )
  }

  void testEnergy_fromTOF()
  {
    std::vector<double> x(1, 4.0), y(1, 1.0);
    std::vector<double> yy = y;
    TS_ASSERT_THROWS_NOTHING( energy.fromTOF(x,y,1.0,1.0,1.0,1,1.0,1.0) )
    TS_ASSERT_DELTA( x[0], 1306759.0, 1.0 )
    TS_ASSERT( yy == y )
  }

  void testEnergy_quickConversions()
  {
    // Test it gives the same answer as going 'the long way'
    double factor, power;
    TS_ASSERT( energy.quickConversion(energyk,factor,power) )
    double input = 100.1;
    double result = factor * std::pow(input,power);
    TS_ASSERT_EQUALS ( result/input, Mantid::PhysicalConstants::meVtoWavenumber )
    std::vector<double> x(1,input);
    energy.toTOF(x,x,99.0,99.0,99.0,99,99.0,99.0);
    energyk.fromTOF(x,x,99.0,99.0,99.0,99,99.0,99.0);
    TS_ASSERT_DELTA( x[0], result, 1.0e-12 )

    TS_ASSERT( energy.quickConversion(lambda,factor,power) )
    result = factor * std::pow(input,power);
    std::vector<double> x2(1,input);
    energy.toTOF(x2,x2,99.0,99.0,99.0,99,99.0,99.0);
    lambda.fromTOF(x2,x2,99.0,99.0,99.0,99,99.0,99.0);
    TS_ASSERT_DELTA( x2[0], result, 1.0e-15 )
  }

  //----------------------------------------------------------------------
  // Energy_inWavenumber tests
  //----------------------------------------------------------------------

  void testEnergy_inWavenumber_unitID()
  {
    TS_ASSERT_EQUALS( energyk.unitID(), "Energy_inWavenumber" )
  }

  void testEnergy_inWavenumber_caption()
  {
    TS_ASSERT_EQUALS( energyk.caption(), "Energy" )
  }

  void testEnergy_inWavenumber_label()
  {
    TS_ASSERT_EQUALS( energyk.label(), "1/cm" )
  }

  void testEnergy_inWavenumber_cast()
  {
    Unit *u = NULL;
    TS_ASSERT_THROWS_NOTHING( u = dynamic_cast<Unit*>(&energyk) );
    TS_ASSERT_EQUALS(u->unitID(), "Energy_inWavenumber");
  }

  void testEnergy_inWavenumber_toTOF()
  {
    std::vector<double> x(1, 4.0), y(1, 1.0);
    std::vector<double> yy = y;
    TS_ASSERT_THROWS_NOTHING( energyk.toTOF(x,y,1.0,1.0,1.0,1,1.0,1.0) )
    TS_ASSERT_DELTA( x[0], 6492.989, 0.001 )
    TS_ASSERT( yy == y )
  }

  void testEnergy_inWavenumber_fromTOF()
  {
    std::vector<double> x(1, 4.0), y(1, 1.0);
    std::vector<double> yy = y;
    TS_ASSERT_THROWS_NOTHING( energyk.fromTOF(x,y,1.0,1.0,1.0,1,1.0,1.0) )
    TS_ASSERT_DELTA( x[0], 10539725, 1.0 )
    TS_ASSERT( yy == y )
  }

  void testEnergy_inWavenumber_quickConversions()
  {
    // Test it gives the same answer as going 'the long way'
    double factor, power;
    TS_ASSERT( energyk.quickConversion(energy,factor,power) )
    double input = 100.1;
    double result = factor * std::pow(input,power);
    TS_ASSERT_EQUALS ( input/result, Mantid::PhysicalConstants::meVtoWavenumber )
    std::vector<double> x(1,input);
    energyk.toTOF(x,x,99.0,99.0,99.0,99,99.0,99.0);
    energy.fromTOF(x,x,99.0,99.0,99.0,99,99.0,99.0);
    TS_ASSERT_DELTA( x[0], result, 1.0e-14 )

    TS_ASSERT( energyk.quickConversion(lambda,factor,power) )
    result = factor * std::pow(input,power);
    std::vector<double> x2(1,input);
    energyk.toTOF(x2,x2,99.0,99.0,99.0,99,99.0,99.0);
    lambda.fromTOF(x2,x2,99.0,99.0,99.0,99,99.0,99.0);
    TS_ASSERT_DELTA( x2[0], result, 1.0e-15 )
  }

  //----------------------------------------------------------------------
  // d-Spacing tests
  //----------------------------------------------------------------------

  void testdSpacing_unitID()
  {
    TS_ASSERT_EQUALS( d.unitID(), "dSpacing" )
  }

  void testdSpacing_caption()
  {
    TS_ASSERT_EQUALS( d.caption(), "d-Spacing" )
  }

  void testdSpacing_label()
  {
    TS_ASSERT_EQUALS( d.label(), "Angstrom" )
  }

  void testdSpacing_cast()
  {
    Unit *u = NULL;
    TS_ASSERT_THROWS_NOTHING( u = dynamic_cast<Unit*>(&d) );
    TS_ASSERT_EQUALS(u->unitID(), "dSpacing");
   }

  void testdSpacing_toTOF()
  {
    std::vector<double> x(1, 1.0), y(1, 1.0);
    std::vector<double> yy = y;
    TS_ASSERT_THROWS_NOTHING( d.toTOF(x,y,1.0,1.0,1.0,1,1.0,1.0) )
    TS_ASSERT_DELTA( x[0], 484.7537, 0.0001 )
    TS_ASSERT( yy == y )
  }

  void testdSpacing_fromTOF()
  {
    std::vector<double> x(1, 1001.1), y(1, 1.0);
    std::vector<double> yy = y;
    TS_ASSERT_THROWS_NOTHING( d.fromTOF(x,y,1.0,1.0,1.0,1,1.0,1.0) )
    TS_ASSERT_DELTA( x[0], 2.065172, 0.000001 )
    TS_ASSERT( yy == y )
  }

  void testdSpacing_quickConversions()
  {
    // Test it gives the same answer as going 'the long way'
    // To MomentumTransfer
    double factor, power;
    TS_ASSERT( d.quickConversion(q,factor,power) )
    double input = 1.1;
    double result = factor * std::pow(input,power);
    std::vector<double> x(1,input);
    d.toTOF(x,x,99.0,99.0,1.0,0,99.0,99.0);
    q.fromTOF(x,x,99.0,99.0,1.0,0,99.0,99.0);
    TS_ASSERT_DELTA( x[0], result, 1.0e-12 )

    // To QSquared
    TS_ASSERT( d.quickConversion(q2,factor,power) )
    input = 1.1;
    result = factor * std::pow(input,power);
    x[0] = input;
    d.toTOF(x,x,99.0,99.0,1.0,0,99.0,99.0);
    q2.fromTOF(x,x,99.0,99.0,1.0,0,99.0,99.0);
    TS_ASSERT_DELTA( x[0], result, 1.0e-12 )
  }

  //----------------------------------------------------------------------
  // Momentum Transfer tests
  //----------------------------------------------------------------------

  void testQTransfer_unitID()
  {
    TS_ASSERT_EQUALS( q.unitID(), "MomentumTransfer" )
  }

  void testQTransfer_caption()
  {
    TS_ASSERT_EQUALS( q.caption(), "q" )
  }

  void testQTransfer_label()
  {
    TS_ASSERT_EQUALS( q.label(), "1/Angstrom" )
  }

  void testQTransfer_cast()
  {
    Unit *u = NULL;
    TS_ASSERT_THROWS_NOTHING( u = dynamic_cast<Unit*>(&q) );
    TS_ASSERT_EQUALS(u->unitID(), "MomentumTransfer");
  }

  void testQTransfer_toTOF()
  {
    std::vector<double> x(1, 1.1), y(1, 1.0);
    std::vector<double> yy = y;
    TS_ASSERT_THROWS_NOTHING( q.toTOF(x,y,1.0,1.0,1.0,1,1.0,1.0) )
    TS_ASSERT_DELTA( x[0], 2768.9067, 0.0001 )
    TS_ASSERT( yy == y )
  }

  void testQTransfer_fromTOF()
  {
    std::vector<double> x(1, 1.1), y(1, 1.0);
    std::vector<double> yy = y;
    TS_ASSERT_THROWS_NOTHING( q.fromTOF(x,y,1.0,1.0,1.0,1,1.0,1.0) )
    TS_ASSERT_DELTA( x[0], 2768.9067, 0.0001 )
    TS_ASSERT( yy == y )
  }

  void testQTransfer_quickConversions()
  {
    // Test it gives the same answer as going 'the long way'
    // To QSquared
    double factor, power;
    TS_ASSERT( q.quickConversion(q2,factor,power) )
    double input = 1.1;
    double result = factor * std::pow(input,power);
    std::vector<double> x(1,input);
    q.toTOF(x,x,99.0,99.0,99.0,99,99.0,99.0);
    q2.fromTOF(x,x,99.0,99.0,99.0,99,99.0,99.0);
    TS_ASSERT_DELTA( x[0], result, 1.0e-30 )

    // To dSpacing
    TS_ASSERT( q.quickConversion(d,factor,power) )
    input = 1.1;
    result = factor * std::pow(input,power);
    x[0] = input;
    q.toTOF(x,x,99.0,99.0,1.0,99,99.0,99.0);
    d.fromTOF(x,x,99.0,99.0,1.0,99,99.0,99.0);
    TS_ASSERT_DELTA( x[0], result, 1.0e-12 )
  }

  //----------------------------------------------------------------------
  // Momentum Squared tests
  //----------------------------------------------------------------------

  void testQ2_unitID()
  {
    TS_ASSERT_EQUALS( q2.unitID(), "QSquared" )
  }

  void testQ2_caption()
  {
    TS_ASSERT_EQUALS( q2.caption(), "Q2" )
  }

  void testQ2_label()
  {
    TS_ASSERT_EQUALS( q2.label(), "Angstrom^-2" )
  }

  void testQ2_cast()
  {
    Unit *u = NULL;
    TS_ASSERT_THROWS_NOTHING( u = dynamic_cast<Unit*>(&q2) );
    TS_ASSERT_EQUALS(u->unitID(), "QSquared");
  }

  void testQ2_toTOF()
  {
    std::vector<double> x(1, 4.0), y(1, 1.0);
    std::vector<double> yy = y;
    TS_ASSERT_THROWS_NOTHING( q2.toTOF(x,y,1.0,1.0,1.0,1,1.0,1.0) )
    TS_ASSERT_DELTA( x[0], 1522.899, 0.001 )
    TS_ASSERT( yy == y )
  }

  void testQ2_fromTOF()
  {
    std::vector<double> x(1, 200.0), y(1, 1.0);
    std::vector<double> yy = y;
    TS_ASSERT_THROWS_NOTHING( q2.fromTOF(x,y,1.0,1.0,1.0,1,1.0,1.0) )
    TS_ASSERT_DELTA( x[0], 231.9220, 0.0001 )
    TS_ASSERT( yy == y )
  }

  void testQ2_quickConversions()
  {
    // Test it gives the same answer as going 'the long way'
    // To MomentumTransfer
    double factor, power;
    TS_ASSERT( q2.quickConversion(q,factor,power) )
    double input = 1.1;
    double result = factor * std::pow(input,power);
    std::vector<double> x(1,input);
    q2.toTOF(x,x,99.0,99.0,99.0,99,99.0,99.0);
    q.fromTOF(x,x,99.0,99.0,99.0,99,99.0,99.0);
    TS_ASSERT_DELTA( x[0], result, 1.0e-30 )

    // To dSpacing
    TS_ASSERT( q2.quickConversion(d,factor,power) )
    input = 1.1;
    result = factor * std::pow(input,power);
    x[0] = input;
    q2.toTOF(x,x,99.0,99.0,1.0,99,99.0,99.0);
    d.fromTOF(x,x,99.0,99.0,1.0,99,99.0,99.0);
    TS_ASSERT_DELTA( x[0], result, 1.0e-15 )
  }

  //----------------------------------------------------------------------
  // Energy transfer tests
  //----------------------------------------------------------------------

  void testDeltaE_unitID()
  {
    TS_ASSERT_EQUALS( dE.unitID(), "DeltaE" )
  }

  void testDeltaE_caption()
  {
    TS_ASSERT_EQUALS( dE.caption(), "Energy transfer" )
  }

  void testDeltaE_label()
  {
    TS_ASSERT_EQUALS( dE.label(), "meV" )
  }

  void testDeltaE_cast()
  {
    Unit *u = NULL;
    TS_ASSERT_THROWS_NOTHING( u = dynamic_cast<Unit*>(&dE) );
    TS_ASSERT_EQUALS(u->unitID(), "DeltaE");
  }

  void testDeltaE_toTOF()
  {
    std::vector<double> x(1, 1.1), y(1, 1.0);
    std::vector<double> yy = y;
    TS_ASSERT_THROWS_NOTHING( dE.toTOF(x,y,1.5,2.5,0.0,1,4.0,0.0) )
    TS_ASSERT_DELTA( x[0], 5071.066, 0.001 )
    TS_ASSERT( yy == y )

    x[0] = 1.1;
    TS_ASSERT_THROWS_NOTHING( dE.toTOF(x,y,1.5,2.5,0.0,2,4.0,0.0) )
    TS_ASSERT_DELTA( x[0], 4376.406, 0.001 )
    TS_ASSERT( yy == y )

    // emode = 0
    TS_ASSERT_THROWS( dE.toTOF(x,y,1.5,2.5,0.0,0,4.0,0.0), std::invalid_argument )
  }

  void testDeltaE_fromTOF()
  {
    std::vector<double> x(1, 2001.0), y(1, 1.0);
    std::vector<double> yy = y;
    TS_ASSERT_THROWS_NOTHING( dE.fromTOF(x,y,1.5,2.5,0.0,1,4.0,0.0) )
    TS_ASSERT_DELTA( x[0], -394.5692, 0.0001 )
    TS_ASSERT( yy == y )

    x[0] = 3001.0;
    TS_ASSERT_THROWS_NOTHING( dE.fromTOF(x,y,1.5,2.5,0.0,2,4.0,0.0) )
    TS_ASSERT_DELTA( x[0], 569.8397, 0.0001 )
    TS_ASSERT( yy == y )

    // emode = 0
    TS_ASSERT_THROWS( dE.fromTOF(x,y,1.5,2.5,0.0,0,4.0,0.0), std::invalid_argument )
  }

  //----------------------------------------------------------------------
  // Energy transfer tests
  //----------------------------------------------------------------------

  void testDeltaEk_unitID()
  {
    TS_ASSERT_EQUALS( dEk.unitID(), "DeltaE_inWavenumber" )
  }

  void testDeltaEk_caption()
  {
    TS_ASSERT_EQUALS( dEk.caption(), "Energy transfer" )
  }

  void testDeltaEk_label()
  {
    TS_ASSERT_EQUALS( dEk.label(), "1/cm" )
  }

  void testDeltaEk_cast()
  {
    Unit *u = NULL;
    TS_ASSERT_THROWS_NOTHING( u = dynamic_cast<Unit*>(&dEk) );
    TS_ASSERT_EQUALS(u->unitID(), "DeltaE_inWavenumber");
  }

  void testDeltaEk_toTOF()
  {
    std::vector<double> x(1, 1.1), y(1, 1.0);
    std::vector<double> yy = y;
    TS_ASSERT_THROWS_NOTHING( dEk.toTOF(x,y,1.5,2.5,0.0,1,4.0,0.0) )
    TS_ASSERT_DELTA( x[0], 11246.74, 0.01 )
    TS_ASSERT( yy == y )

    x[0] = 1.1;
    TS_ASSERT_THROWS_NOTHING( dEk.toTOF(x,y,1.5,2.5,0.0,2,4.0,0.0) )
    TS_ASSERT_DELTA( x[0], 7170.555, 0.001 )
    TS_ASSERT( yy == y )

    // emode = 0
    TS_ASSERT_THROWS( dEk.toTOF(x,y,1.5,2.5,0.0,0,4.0,0.0), std::invalid_argument )
  }

  void testDeltaEk_fromTOF()
  {
    std::vector<double> x(1, 2001.0), y(1, 1.0);
    std::vector<double> yy = y;
    TS_ASSERT_THROWS_NOTHING( dEk.fromTOF(x,y,1.5,2.5,0.0,1,4.0,0.0) )
    TS_ASSERT_DELTA( x[0], -3182.416, 0.001 )
    TS_ASSERT( yy == y )

    x[0] = 3001.0;
    TS_ASSERT_THROWS_NOTHING( dEk.fromTOF(x,y,1.5,2.5,0.0,2,4.0,0.0) )
    TS_ASSERT_DELTA( x[0], 4596.068, 0.001 )
    TS_ASSERT( yy == y )

    // emode = 0
    TS_ASSERT_THROWS( dEk.fromTOF(x,y,1.5,2.5,0.0,0,4.0,0.0), std::invalid_argument )
  }

private:
  Units::TOF tof;
  Units::Wavelength lambda;
  Units::Energy energy;
  Units::Energy_inWavenumber energyk;
  Units::dSpacing d;
  Units::MomentumTransfer q;
  Units::QSquared q2;
  Units::DeltaE dE;
  Units::DeltaE_inWavenumber dEk;
};

#endif /*UNITTEST_H_*/
