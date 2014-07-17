#ifndef UNITTEST_H_
#define UNITTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Unit.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitLabelTypes.h"
#include <boost/lexical_cast.hpp>
#include <cfloat>
#include <limits>

using namespace Mantid::Kernel;
using namespace Mantid::Kernel::Units;

// function checks if conversion within limits works reversibly
std::string convert_units_check_range(const Unit &aUnit,std::vector<double> &samples,std::vector<double> &results, double epsilon1=0)
{
  std::string error_mess("");

  samples.resize(4);
  results.resize(4);
  double tof_min = aUnit.conversionTOFMin();
  double tof_max = aUnit.conversionTOFMax();
  samples[0] = tof_min;
  samples[1] = tof_max;

  double initValMin = aUnit.singleFromTOF(tof_min); 
  double initValMax = aUnit.singleFromTOF(tof_max);
  samples[2] = initValMin;
  samples[3] = initValMax ;

  results[0]= aUnit.singleToTOF(initValMin); // tof1
  results[1]= aUnit.singleToTOF(initValMax); // tof2
  results[2]= aUnit.singleFromTOF(results[0]); // unit 1
  results[3]= aUnit.singleFromTOF(results[1]); // unit 2


  auto range = aUnit.conversionRange();
  double tof1=aUnit.singleToTOF(range.first);
  double tof2=aUnit.singleToTOF(range.second);
  bool t_increases(true);
  if (tof1>tof2)
    t_increases=false;

  if (tof1==tof2)
  {
    error_mess = "conversion: "+aUnit.unitID()+ " Time range is  zero (tof_left==tof_rignt)";
    return error_mess;
  }
  if (tof1<tof_min || tof2 < tof_min)
  {
      error_mess = "conversion: "+aUnit.unitID()+ " min time range is smaller then minimal conversion time";
      return error_mess;
  }
  if (tof1>tof_max*(1+epsilon1) || tof2 > tof_max*(1+epsilon1))
  {
      error_mess = "conversion: "+aUnit.unitID()+ "max time range is bigger then maximal conversion time";
      return error_mess;
  }


  const size_t nSteps(100);

  double step = (range.second-range.first)/nSteps;
  if (step == std::numeric_limits<double>::infinity())
  {
    step =(DBL_MAX/nSteps)*2;
  }
 
  double t1 = aUnit.singleToTOF(range.first);
  for(size_t i=1;i<=nSteps;i++)
  {
    double unitVal=range.first+double(i)*step;
    double tofVal = aUnit.singleToTOF(unitVal);
    if (t_increases)
    {
      if (tofVal*(1+epsilon1)< t1)
      {
        error_mess="conversion: "+aUnit.unitID()+" subsequent tof decreases for increasing function at step: "+boost::lexical_cast<std::string>(i);
        return error_mess;
      }
    }
    else
    {
      if (tofVal>t1*(1+epsilon1))
      {
        error_mess="conversion: "+aUnit.unitID()+" subsequent tof increases for decreasing function at step: "+boost::lexical_cast<std::string>(i);
        return error_mess;
      }

      t1=tofVal;
    }

  }

  return error_mess;
}

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
    const UnitLabel label () const {return UnitLabel("");}
    void init() {}
    virtual double singleToTOF(const double ) const { return 0; }
    virtual double singleFromTOF(const double ) const { return 0; }
    virtual double conversionTOFMax()const{return std::numeric_limits<double>::quiet_NaN();}
    virtual double conversionTOFMin()const{return std::numeric_limits<double>::quiet_NaN();}

    virtual Unit * clone() const { return new UnitTester();}
  };

public:

  //----------------------------------------------------------------------
  // Label tests
  //----------------------------------------------------------------------

  void testLabel_constructor()
  {
    Label lbl("Temperature", "K");
    TS_ASSERT_EQUALS(lbl.caption(), "Temperature");
    TS_ASSERT_EQUALS(lbl.label().ascii(), "K");
  }

  void testLabel_unitID()
  {
    TS_ASSERT_EQUALS( label.unitID(), "Label" );
  }

  void testLabel_caption()
  {
    TS_ASSERT_EQUALS( label.caption(), "Quantity" );
  }

  void testLabel_label()
  {
    TS_ASSERT_EQUALS( label.label().ascii(), "" );
  }

  void testLabel_cast()
  {
    Unit *u = NULL;
    TS_ASSERT_THROWS_NOTHING( u = dynamic_cast<Unit*>(&label) );
    TS_ASSERT_EQUALS(u->unitID(), "Label");
  }

  void testLabel_setLabel()
  {
    label.setLabel("Temperature", "K");
    TS_ASSERT_EQUALS(label.caption(), "Temperature");
    TS_ASSERT_EQUALS(label.label().ascii(), "K");
  }
  void testLabel_limits()
  {    
    double volatile lim_min=label.conversionTOFMin();
    TS_ASSERT(lim_min!=label.conversionTOFMin());
    double volatile lim_max=label.conversionTOFMax();
    TS_ASSERT(lim_max!=label.conversionTOFMax());
  }

  /**
   * Tests the two equality operators == and !=
   */
  void testEqualityOperators()
  {
    //Get some units to test equality with
    auto *e1 = Energy().clone();
    auto *e2 = Energy().clone();
    auto *wl = Wavelength().clone();

    //Test equality operator
    TS_ASSERT(*e1 == *e2);

    //Test inequality oeprator
    TS_ASSERT(*e1 != *wl);

    delete e1;
    delete e2;
    delete wl;
  }

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

  void test_clone()
  {
    auto unit = Empty().clone();
    TS_ASSERT( dynamic_cast<Empty*>( unit ) );
    delete unit;
    unit = Label().clone();
    TS_ASSERT( dynamic_cast<Label*>( unit ) );
    delete unit;
    unit = Wavelength().clone();
    TS_ASSERT( dynamic_cast<Wavelength*>( unit ) );
    delete unit;
    unit = Energy().clone();
    TS_ASSERT( dynamic_cast<Energy*>( unit ) );
    delete unit;
    unit = Energy_inWavenumber().clone();
    TS_ASSERT( dynamic_cast<Energy_inWavenumber*>( unit ) );
    delete unit;
    unit = dSpacing().clone();
    TS_ASSERT( dynamic_cast<dSpacing*>( unit ) );
    delete unit;
    unit = MomentumTransfer().clone();
    TS_ASSERT( dynamic_cast<MomentumTransfer*>( unit ) );
    delete unit;
    unit = QSquared().clone();
    TS_ASSERT( dynamic_cast<QSquared*>( unit ) );
    delete unit;
    unit = DeltaE().clone();
    TS_ASSERT( dynamic_cast<DeltaE*>( unit ) );
    delete unit;
    unit = DeltaE_inWavenumber().clone();
    TS_ASSERT( dynamic_cast<DeltaE_inWavenumber*>( unit ) );
    delete unit;
    unit = Momentum().clone();
    TS_ASSERT( dynamic_cast<Momentum*>( unit ) );
    delete unit;
  }
    
  //----------------------------------------------------------------------
  // TOF tests
  //----------------------------------------------------------------------

  void testTOF_unitID()
  {
    TS_ASSERT_EQUALS( tof.unitID(), "TOF" );
  }

  void test_copy_constructor_on_concrete_type()
  {
    Units::TOF first;
    first.initialize(1.0,1.0,1.0,2,1.0,1.0);
    Units::TOF second(first);
    TS_ASSERT_EQUALS(first.isInitialized(), second.isInitialized());
    TS_ASSERT_EQUALS(first.unitID(), second.unitID())
    TS_ASSERT_EQUALS(first.caption(), second.caption())
    TS_ASSERT_EQUALS(first.label().ascii(), second.label().ascii())
    TS_ASSERT_EQUALS(first.label().utf8(), second.label().utf8())
  }

  void test_copy_assignment_operator_on_concrete_type()
  {
    Units::TOF first;
    first.initialize(1.0,1.0,1.0,2,1.0,1.0);
    Units::TOF second;
    second = first;
    TS_ASSERT_EQUALS(first.isInitialized(), second.isInitialized());
    TS_ASSERT_EQUALS(first.unitID(), second.unitID())
    TS_ASSERT_EQUALS(first.caption(), second.caption())
    TS_ASSERT_EQUALS(first.label().ascii(), second.label().ascii())
    TS_ASSERT_EQUALS(first.label().utf8(), second.label().utf8())
  }


  void testTOF_caption()
  {
    TS_ASSERT_EQUALS( tof.caption(), "Time-of-flight" );
  }

  void testTOF_label()
  {
    TS_ASSERT_EQUALS( tof.label().ascii(), "microsecond" )
    TS_ASSERT_EQUALS( tof.label().utf8(), L"\u03bcs" )
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
  void testTOFrange()
  {    
    std::vector<double> sample,rezult;
    std::string err_mess=convert_units_check_range(tof,sample,rezult);
    TSM_ASSERT(" ERROR:"+err_mess,err_mess.size()==0);

    for(size_t i=0;i<sample.size();i++)
    {
      TS_ASSERT_DELTA(sample[i],rezult[i],FLT_EPSILON);
    }
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
    TS_ASSERT_EQUALS( lambda.label().ascii(), "Angstrom" )
    TS_ASSERT_EQUALS( lambda.label().utf8(), L"\u212b" )
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

    TS_ASSERT_DELTA( lambda.convertSingleToTOF(1.5, 1.0,1.0,1.0,1,1.0,1.0), 2665.4390, 0.0001 );
  }

  void testWavelength_fromTOF()
  {
    std::vector<double> x(1, 1000.5), y(1, 1.5);
    std::vector<double> yy = y;
    TS_ASSERT_THROWS_NOTHING( lambda.fromTOF(x,y,1.0,1.0,1.0,1,1.0,1.0) )
    TS_ASSERT_DELTA( x[0], -5.0865, 0.0001 ) // 1.979006
    TS_ASSERT( yy == y )

    TS_ASSERT_DELTA( lambda.convertSingleFromTOF(1000.5, 1.0,1.0,1.0,1,1.0,1.0), -5.0865, 0.0001);
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

  void testWavelengthrange()
  {    
    std::vector<double> sample,rezult;
    std::string err_mess=convert_units_check_range(lambda,sample,rezult);
    TSM_ASSERT(" ERROR:"+err_mess,err_mess.size()==0);

    for(size_t i=0;i<sample.size();i++)
    {
      TSM_ASSERT_DELTA(" Failed for conversion N: "+boost::lexical_cast<std::string>(i),sample[i],rezult[i],FLT_EPSILON);
    }
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
    TS_ASSERT_EQUALS( energy.label().ascii(), "meV" )
    TS_ASSERT_EQUALS( energy.label().utf8(), L"meV" )
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
  void testEnergyRange()
  {    
    std::vector<double> sample,rezult;
    std::string err_mess=convert_units_check_range(energy,sample,rezult);
    TSM_ASSERT(" ERROR:"+err_mess,err_mess.size()==0);
    for(size_t i=0;i<sample.size();i++)
    {
      if (std::fabs(sample[i])<10*FLT_EPSILON)
      {
        TSM_ASSERT_DELTA("Energy limits Failed for conversion N: "+boost::lexical_cast<std::string>(i),sample[i],rezult[i],10*FLT_EPSILON);
      }
      else
      {
        TSM_ASSERT_DELTA("Energy limits Failed for conversion N: "+boost::lexical_cast<std::string>(i),rezult[i]/sample[i],1.,10*FLT_EPSILON);
      }
        
    }
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
    TS_ASSERT_EQUALS( energyk.label().ascii(), "cm^-1" )
    TS_ASSERT_EQUALS( energyk.label().utf8(), L"cm\u207b\u00b9" )
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
    TS_ASSERT_EQUALS( d.label().ascii(), "Angstrom" )
    TS_ASSERT_EQUALS( d.label().utf8(), L"\u212b" )
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
  void testdSpacingRange()
  {    
    std::vector<double> sample,rezult;

    std::string err_mess=convert_units_check_range(d,sample,rezult);
    TSM_ASSERT(" ERROR:"+err_mess,err_mess.size()==0);

    for(size_t i=0;i<sample.size();i++)
    {
      if (std::fabs(sample[i])<10*FLT_EPSILON)
      {
        TSM_ASSERT_DELTA("d-spacing limits Failed for conversion N: "+boost::lexical_cast<std::string>(i),sample[i],rezult[i],10*FLT_EPSILON);
      }
      else
      {
        TSM_ASSERT_DELTA("d-spacing limits Failed for conversion N: "+boost::lexical_cast<std::string>(i),rezult[i]/sample[i],1.,10*FLT_EPSILON);
      }
        
    }
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
    TS_ASSERT_EQUALS( q.label().ascii(), "Angstrom^-1" )
    TS_ASSERT_EQUALS( q.label().utf8(), L"\u212b\u207b\u00b9" )
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
  void testMomentumTransferRange()
  {    
    std::vector<double> sample,rezult;

    std::string err_mess=convert_units_check_range(q,sample,rezult);
    TSM_ASSERT(" ERROR:"+err_mess,err_mess.size()==0);

    for(size_t i=0;i<sample.size();i++)
    {
      if (std::fabs(sample[i])<10*FLT_EPSILON)
      {
        TSM_ASSERT_DELTA("Momentum transfer limits Failed for conversion N: "+boost::lexical_cast<std::string>(i),sample[i],rezult[i],10*FLT_EPSILON);
      }
      else
      {
        TSM_ASSERT_DELTA("Momentum transfer limits Failed for conversion N: "+boost::lexical_cast<std::string>(i),rezult[i]/sample[i],1.,10*FLT_EPSILON);
      }
        
    }
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
    TS_ASSERT_EQUALS( q2.label().ascii(), "Angstrom^-2" )
    TS_ASSERT_EQUALS( q2.label().utf8(), L"\u212b\u207b\u00b2" )
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
  void testQ2Range()
  {    
    std::vector<double> sample,rezult;

    q2.initialize(1.1,1.1,99.0,0,99.0,0);
    std::string err_mess=convert_units_check_range(q2,sample,rezult,-DBL_EPSILON);
    TSM_ASSERT(" ERROR:"+err_mess,err_mess.size()==0);

    for(size_t i=0;i<sample.size();i++)
    {
      if (std::fabs(sample[i])<10*FLT_EPSILON)
      {
        TSM_ASSERT_DELTA("Momentum transfer limits Failed for conversion N: "+boost::lexical_cast<std::string>(i),sample[i],rezult[i],10*FLT_EPSILON);
      }
      else
      {
        TSM_ASSERT_DELTA("Momentum transfer limits Failed for conversion N: "+boost::lexical_cast<std::string>(i),rezult[i]/sample[i],1.,10*FLT_EPSILON);
      }
        
    }
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
    TS_ASSERT_EQUALS( dE.label().ascii(), "meV" )
    TS_ASSERT_EQUALS( dE.label().utf8(), L"meV" )
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
  void testDERange()
  {    
    std::vector<double> sample,rezult;
    //Direct
    dE.initialize(2001.0,1.0, 1.5, 1, 10., 0.0);

    std::string err_mess=convert_units_check_range(dE,sample,rezult,DBL_EPSILON);
    TSM_ASSERT(" ERROR:"+err_mess,err_mess.size()==0);

    TSM_ASSERT_DELTA("Direct energy transfer limits Failed for conversion t_min: ",sample[0],rezult[0],10*FLT_EPSILON);
    TSM_ASSERT_DELTA("Direct energy transfer limits Failed for conversion t_max: ",sample[1]/rezult[1],1.,0.05);
    TSM_ASSERT_DELTA("Direct energy transfer limits Failed for conversion e_min: ",sample[2],rezult[2],10*FLT_EPSILON);
    TSM_ASSERT_DELTA("Direct energy transfer limits Failed for conversion e_max: ",sample[3],rezult[3],10*FLT_EPSILON);
   
    //Indirect
    dE.initialize(2001.0,1.0, 1.5, 2, 10., 0.0);

    err_mess=convert_units_check_range(dE,sample,rezult);
    TSM_ASSERT(" ERROR:"+err_mess,err_mess.size()==0);

    TSM_ASSERT_DELTA("Indirect energy transfer limits Failed for conversion t_min: ",sample[0],rezult[0],10*FLT_EPSILON);
    TSM_ASSERT_DELTA("Indirect energy transfer limits Failed for conversion t_max: ",sample[1]/rezult[1],1.,0.05);
    TSM_ASSERT_DELTA("Indirect energy transfer limits Failed for conversion e_min: ",sample[2],rezult[2],10*FLT_EPSILON);
    TSM_ASSERT_DELTA("Indirect energy transfer limits Failed for conversion e_max: ",sample[3],rezult[3],10*FLT_EPSILON);

  
  }




  //----------------------------------------------------------------------
  // Energy transfer in wavenumber tests
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
    TS_ASSERT_EQUALS( dEk.label().ascii(), "cm^-1" )
    TS_ASSERT_EQUALS( dEk.label().utf8(), L"cm\u207b\u00b9" )
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
    TS_ASSERT_DELTA( x[0], 4622.5452, 0.01 )
    TS_ASSERT( yy == y )

    x[0] = 1.1;
    TS_ASSERT_THROWS_NOTHING( dEk.toTOF(x,y,1.5,2.5,0.0,2,4.0,0.0) )
    TS_ASSERT_DELTA( x[0], 4544.0378, 0.001 )
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
  void testDE_kRange()
  {    
    std::vector<double> sample,rezult;
    //Direct
    dEk.initialize(2001.0,1.0, 1.5, 1, 10., 0.0);

    std::string err_mess=convert_units_check_range(dEk,sample,rezult);
    TSM_ASSERT(" ERROR:"+err_mess,err_mess.size()==0);

    TSM_ASSERT_DELTA("Direct energy transfer limits Failed for conversion t_min: ",sample[0],rezult[0],10*FLT_EPSILON);
    TSM_ASSERT_DELTA("Direct energy transfer limits Failed for conversion t_max: ",sample[1]/rezult[1],1.,0.05);
    TSM_ASSERT_DELTA("Direct energy transfer limits Failed for conversion e_min: ",sample[2],rezult[2],10*FLT_EPSILON);
    TSM_ASSERT_DELTA("Direct energy transfer limits Failed for conversion e_max: ",sample[3],rezult[3],10*FLT_EPSILON);
   
    //Indirect
    dEk.initialize(2001.0,1.0, 1.5, 2, 10., 0.0);

    err_mess=convert_units_check_range(dEk,sample,rezult);
    TSM_ASSERT(" ERROR:"+err_mess,err_mess.size()==0);

    TSM_ASSERT_DELTA("Indirect energy transfer limits Failed for conversion t_min: ",sample[0],rezult[0],10*FLT_EPSILON);
    TSM_ASSERT_DELTA("Indirect energy transfer limits Failed for conversion t_max: ",sample[1]/rezult[1],1.,0.05);
    TSM_ASSERT_DELTA("Indirect energy transfer limits Failed for conversion e_min: ",sample[2],rezult[2],10*FLT_EPSILON);
    TSM_ASSERT_DELTA("Indirect energy transfer limits Failed for conversion e_max: ",sample[3],rezult[3],10*FLT_EPSILON);

  
  }

  //----------------------------------------------------------------------
  // Momentum tests
  //----------------------------------------------------------------------

  void testMomentum_unitID()
  {
    TS_ASSERT_EQUALS( k_i.unitID(), "Momentum" )
  }

  void testMomentum_caption()
  {
    TS_ASSERT_EQUALS( k_i.caption(), "Momentum" )
  }

  void testMomentum_label()
  {
    TS_ASSERT_EQUALS( k_i.label().ascii(), "Angstrom^-1" )
    TS_ASSERT_EQUALS( k_i.label().utf8(), L"\u212b\u207b\u00b9" )
  }

  void testMomentum_cast()
  {
    Unit *u = NULL;
    TS_ASSERT_THROWS_NOTHING( u = dynamic_cast<Unit*>(&k_i) );
    TS_ASSERT_EQUALS(u->unitID(), "Momentum");
  }

  void testMomentum_toTOF()
  {
    std::vector<double> x(1, 2*M_PI/1.5), y(1, 1.5);
    std::vector<double> yy = y;
    TS_ASSERT_THROWS_NOTHING( k_i.toTOF(x,y,1.0,1.0,1.0,1,1.0,1.0) )
    //TS_ASSERT_DELTA( x[0], 2665.4390, 0.0001 ) // -- wavelength to TOF;
    TS_ASSERT_DELTA( x[0], 2665.4390, 0.0001 ) //
    TS_ASSERT( yy == y )
  }

  void testMomentum_fromTOF()
  {
    std::vector<double> x(1, 1000.5), y(1, 1.5);
    std::vector<double> yy = y;
    TS_ASSERT_THROWS_NOTHING( k_i.fromTOF(x,y,1.0,1.0,1.0,1,1.0,1.0) )
//    TS_ASSERT_DELTA( x[0], -5.0865, 0.0001 ) // wavelength from TOF
    TS_ASSERT_DELTA( x[0], 2*M_PI/(-5.0865), 0.0001 ) // 1.979006
    TS_ASSERT( yy == y )
  }

  void testMomentum_quickConversions()
  {
    // Test it gives the same answer as going 'the long way'
    double factor, power;
    TS_ASSERT( k_i.quickConversion(energy,factor,power) )
    double input = 1.1;
    double result = factor * std::pow(input,power);
    std::vector<double> x(1,input);
    k_i.toTOF(x,x,99.0,99.0,99.0,99,99.0,99.0);
    energy.fromTOF(x,x,99.0,99.0,99.0,99,99.0,99.0);
    TS_ASSERT_DELTA( x[0], result, 1.0e-10 )

    TS_ASSERT( k_i.quickConversion(energyk,factor,power) )
    double result2 = factor * std::pow(input,power);
    TS_ASSERT_EQUALS( result2/result, Mantid::PhysicalConstants::meVtoWavenumber )
    std::vector<double> x2(1,input);
    k_i.toTOF(x2,x2,99.0,99.0,99.0,99,99.0,99.0);
    energyk.fromTOF(x2,x2,99.0,99.0,99.0,99,99.0,99.0);
    TS_ASSERT_DELTA( x2[0], result2, 1.0e-10 );

    TS_ASSERT( k_i.quickConversion(lambda,factor,power) );
    double factor1, power1;
    TS_ASSERT( lambda.quickConversion(k_i,factor1,power1) );

    TS_ASSERT_DELTA(0,power-power1,0.0001);
    TS_ASSERT_DELTA(0,factor-factor1,0.0001);
  }
  void testK_iRange()
  {    
    std::vector<double> sample,rezult;
    k_i.initialize(1.1,1.1,99.0,0,99.0,99);

    std::string err_mess=convert_units_check_range(k_i,sample,rezult,DBL_EPSILON);
    TSM_ASSERT(" ERROR:"+err_mess,err_mess.size()==0);

    for(size_t i=0;i<sample.size();i++)
    {
      if (std::fabs(sample[i])<10*FLT_EPSILON)
      {
        TSM_ASSERT_DELTA("Elastic Momentum transfer limits Failed for conversion N: "+boost::lexical_cast<std::string>(i),sample[i],rezult[i],10*FLT_EPSILON);
      }
      else
      {
        TSM_ASSERT_DELTA("Elastic Momentum transfer limits Failed for conversion N: "+boost::lexical_cast<std::string>(i),rezult[i]/sample[i],1.,10*FLT_EPSILON);
      }        
    }

    k_i.initialize(10000,11,99.0,2,99.0,99);

    err_mess=convert_units_check_range(k_i,sample,rezult,DBL_EPSILON);
    TSM_ASSERT(" ERROR:"+err_mess,err_mess.size()==0);

    for(size_t i=0;i<sample.size();i++)
    {
      if (std::fabs(sample[i])<10*FLT_EPSILON)
      {
        TSM_ASSERT_DELTA("Indirect Momentum transfer limits Failed for conversion N: "+boost::lexical_cast<std::string>(i),sample[i],rezult[i],10*FLT_EPSILON);
      }
      else
      {
        TSM_ASSERT_DELTA("Indirect Momentum transfer limits Failed for conversion N: "+boost::lexical_cast<std::string>(i),rezult[i]/sample[i],1.,10*FLT_EPSILON);
      }        
    }

    k_i.initialize(1,1.1,99.0,1,99.0,99);

    err_mess=convert_units_check_range(k_i,sample,rezult,DBL_EPSILON);
    TSM_ASSERT(" ERROR:"+err_mess,err_mess.size()==0);

    for(size_t i=0;i<sample.size();i++)
    {
      if (std::fabs(sample[i])<10*FLT_EPSILON)
      {
        TSM_ASSERT_DELTA("Direct Momentum transfer limits Failed for conversion N: "+boost::lexical_cast<std::string>(i),sample[i],rezult[i],10*FLT_EPSILON);
      }
      else
      {
        TSM_ASSERT_DELTA("Direct Momentum transfer limits Failed for conversion N: "+boost::lexical_cast<std::string>(i),rezult[i]/sample[i],1.,10*FLT_EPSILON);
      }        
    }

  }



   //----------------------------------------------------------------------
  // Spin Echo Length tests
  //----------------------------------------------------------------------

  void testSpinEchoLength_unitID()
  {
    TS_ASSERT_EQUALS( delta.unitID(), "SpinEchoLength" )
  }

  void testSpinEchoLength_caption()
  {
    TS_ASSERT_EQUALS( delta.caption(), "Spin Echo Length" )
  }

  void testSpinEchoLength_label()
  {
    TS_ASSERT_EQUALS( delta.label().ascii(), "nm" )
    TS_ASSERT_EQUALS( delta.label().utf8(), L"nm" )
  }

  void testSpinEchoLength_cast()
  {
    Unit *u = NULL;
    TS_ASSERT_THROWS_NOTHING( u = dynamic_cast<Unit*>(&delta) );
    TS_ASSERT_EQUALS(u->unitID(), "SpinEchoLength");
  }

  void testSpinEchoLength_toTOF()
  {
    std::vector<double> x(1, 4.5), y(1, 1.5);
    std::vector<double> yy = y;
    TS_ASSERT_THROWS_NOTHING( delta.toTOF(x,y,1.0,1.0,1.0,0,2.0,1.0) )
    TS_ASSERT_DELTA( x[0], 758.3352, 0.0001 ) 
    TS_ASSERT( yy == y )

    TS_ASSERT_DELTA( delta.convertSingleToTOF(4.5, 1.0,1.0,1.0,0,2.0,1.0),  758.3352, 0.0001 );
  }

  void testSpinEchoLength_fromTOF()
  {
    std::vector<double> x(1, 1000.5), y(1, 1.5);
    std::vector<double> yy = y;
    TS_ASSERT_THROWS_NOTHING( delta.fromTOF(x,y,1.0,1.0,1.0,0,2.0,1.0) )
    TS_ASSERT_DELTA( x[0], 7.8329, 0.0001 ) 
    TS_ASSERT( yy == y )

    TS_ASSERT_DELTA( delta.convertSingleFromTOF(1000.5, 1.0,1.0,1.0,0,2.0,1.0), 7.8329, 0.0001);
  }

  void testSpinEchoLength_invalidfromTOF()
  {
    std::vector<double> x(1, 1000.5), y(1, 1.5);
    //emode must = 0
    TS_ASSERT_THROWS_ANYTHING( delta.fromTOF(x,y,1.0,1.0,1.0,1,2.0,1.0) )
  }

  void testSpinEchoLength_quickConversions()
  {
    // Test that the quick conversions from wavelength have not leaked through
    double factor, power;
    TS_ASSERT( !delta.quickConversion(energy,factor,power) )
    TS_ASSERT( !delta.quickConversion(energyk,factor,power) )
  }
  void testSpinEchoRange()
  {    
    std::vector<double> sample,rezult;
    delta.initialize(10,1.1,99.0,0,99.0,99);

    std::string err_mess=convert_units_check_range(delta,sample,rezult);
    TSM_ASSERT(" ERROR:"+err_mess,err_mess.size()==0);

    for(size_t i=0;i<sample.size();i++)
    {
      if (std::fabs(sample[i])<10*FLT_EPSILON)
      {
        TSM_ASSERT_DELTA("Spin Echo limits Failed for conversion N: "+boost::lexical_cast<std::string>(i),sample[i],rezult[i],10*FLT_EPSILON);
      }
      else
      {
        TSM_ASSERT_DELTA("Spin Echo limits Failed for conversion N: "+boost::lexical_cast<std::string>(i),rezult[i]/sample[i],1.,10*FLT_EPSILON);
      }        
    } 

  }

  
   //----------------------------------------------------------------------
  // Spin Echo Time tests
  //----------------------------------------------------------------------

  void testSpinEchoTime_unitID()
  {
    TS_ASSERT_EQUALS( tau.unitID(), "SpinEchoTime" )
  }

  void testSpinEchoTime_caption()
  {
    TS_ASSERT_EQUALS( tau.caption(), "Spin Echo Time" )
  }

  void testSpinEchoTime_label()
  {
    TS_ASSERT_EQUALS( tau.label().ascii(), "ns" )
    TS_ASSERT_EQUALS( tau.label().utf8(), L"ns" )
  }

  void testSpinEchoTime_cast()
  {
    Unit *u = NULL;
    TS_ASSERT_THROWS_NOTHING( u = dynamic_cast<Unit*>(&tau) );
    TS_ASSERT_EQUALS(u->unitID(), "SpinEchoTime");
  }

  void testSpinEchoTime_toTOF()
  {
    std::vector<double> x(1, 4.5), y(1, 1.5);
    std::vector<double> yy = y;
    TS_ASSERT_THROWS_NOTHING( tau.toTOF(x,y,1.0,1.0,1.0,0,2.0,1.0) )
    TS_ASSERT_DELTA( x[0], 662.4668, 0.0001 ) 
    TS_ASSERT( yy == y )

    TS_ASSERT_DELTA( tau.convertSingleToTOF(4.5, 1.0,1.0,1.0,0,2.0,1.0),  662.4668, 0.0001 );
  }

  void testSpinEchoTime_fromTOF()
  {
    std::vector<double> x(1, 1000.5), y(1, 1.5);
    std::vector<double> yy = y;
    TS_ASSERT_THROWS_NOTHING( tau.fromTOF(x,y,1.0,1.0,1.0,0,2.0,1.0) )
    TS_ASSERT_DELTA( x[0], 15.5014, 0.0001 ) 
    TS_ASSERT( yy == y )

    TS_ASSERT_DELTA( tau.convertSingleFromTOF(1000.5, 1.0,1.0,1.0,0,2.0,1.0), 15.5014, 0.0001);
  }

  void testSpinEchoTime_invalidfromTOF()
  {
    std::vector<double> x(1, 1000.5), y(1, 1.5);
    //emode must = 0
    TS_ASSERT_THROWS_ANYTHING( tau.fromTOF(x,y,1.0,1.0,1.0,1,2.0,1.0) )
  }

  void testSpinEchoTime_quickConversions()
  {
    // Test that the quick conversions from wavelength have not leaked through
    double factor, power;
    TS_ASSERT( !tau.quickConversion(energy,factor,power) )
    TS_ASSERT( !tau.quickConversion(energyk,factor,power) )
  }
  void testSpinEchoTimeRange()
  {    
    std::vector<double> sample,rezult;
    tau.initialize(100,11,1.0,0,1.0,1);

    std::string err_mess=convert_units_check_range(tau,sample,rezult,DBL_EPSILON);
    TSM_ASSERT(" ERROR:"+err_mess,err_mess.size()==0);

    for(size_t i=0;i<sample.size();i++)
    {
      if (std::fabs(sample[i])<10*FLT_EPSILON)
      {
        TSM_ASSERT_DELTA("Spin Echo limits Failed for conversion N: "+boost::lexical_cast<std::string>(i),sample[i],rezult[i],10*FLT_EPSILON);
      }
      else
      {
        TSM_ASSERT_DELTA("Spin Echo limits Failed for conversion N: "+boost::lexical_cast<std::string>(i),rezult[i]/sample[i],1.,10*FLT_EPSILON);
      }        
    } 

  }


private:
  Units::Label label;
  Units::TOF tof;
  Units::Wavelength lambda;
  Units::Energy energy;
  Units::Energy_inWavenumber energyk;
  Units::dSpacing d;
  Units::MomentumTransfer q;
  Units::QSquared q2;
  Units::DeltaE dE;
  Units::DeltaE_inWavenumber dEk;
  Units::Momentum k_i;
  Units::SpinEchoLength delta;
  Units::SpinEchoTime tau;
};

#endif /*UNITTEST_H_*/
