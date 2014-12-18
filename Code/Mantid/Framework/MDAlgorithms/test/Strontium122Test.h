#ifndef MANTID_MDALGORITHMS_STRONTIUM122TEST_H_
#define MANTID_MDALGORITHMS_STRONTIUM122TEST_H_

#include "MantidMDAlgorithms/Quantification/Models/Strontium122.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MDFittingTestHelpers.h"

#include <cxxtest/TestSuite.h>

class Strontium122Test : public CxxTest::TestSuite
{
private:
  class FakeFGModelFitFunction :
    public virtual Mantid::API::IFunctionMD, public virtual Mantid::API::ParamFunction
  {
  public:
    FakeFGModelFitFunction(Mantid::MDAlgorithms::ForegroundModel & fgModel)
    {
      fgModel.setFunctionUnderMinimization(*this);
      const size_t nparams = fgModel.nParams();
      for(size_t i = 0; i < nparams; ++i)
      {
        this->declareParameter(fgModel.parameterName(i), fgModel.getInitialParameterValue(i),
                               fgModel.parameterDescription(i));
      }

      setParameter("Seff", 0.7);
      setParameter("J1a", 38.7);
      setParameter("J1b", -5.0);
      setParameter("J2", 27.3);
      setParameter("SJc", 10.0);
      setParameter("GammaSlope", 0.08);
    }

    /// Returns the number of attributes associated with the function
     virtual size_t nAttributes()const{return 2;}
     /// Returns a list of attribute names
     virtual std::vector<std::string> getAttributeNames()const
     {
       auto names = std::vector<std::string>(2);
       names[0] = "MultEps";
       names[1] = "TwinType";
       return names;
     }


    std::string name() const { return "FakeFGModelFitFunction"; }
    double functionMD(const Mantid::API::IMDIterator&) const
    {
      return 0.0;
    }
  };

public:

  void test_Initialized_Model_Has_Six_Parameters()
  {
    using Mantid::MDAlgorithms::Strontium122;
    Strontium122 sr122;

    TS_ASSERT_EQUALS(sr122.nParams(), 0);
    sr122.initialize();
    TS_ASSERT_EQUALS(sr122.nParams(), 6);
  }

  void test_Sr122_Has_DefaultIon_As_Fe2()
  {
    using Mantid::MDAlgorithms::Strontium122;
    using Mantid::MDAlgorithms::ForegroundModel;
    Strontium122 sr122Default;
    sr122Default.initialize();
    sr122Default.setAttributeValue("MultEps", 0);
    sr122Default.setAttributeValue("TwinType", 0);
    double valueWithDefault = calculateTestModelWeight(sr122Default);
    TS_ASSERT_DELTA(0.0000062768, valueWithDefault, 1e-10); // Check the absolute value is correct

    Strontium122 sr122;
    sr122.initialize();
    sr122.setAttributeValue("MultEps", 0);
    sr122.setAttributeValue("TwinType", 0);
    sr122.setAttributeValue("FormFactorIon", "Fe2");
    
    // Same test but set the ion to check they match
    double valueWithAttrSet = calculateTestModelWeight(sr122);

    TS_ASSERT_DELTA(valueWithDefault, valueWithAttrSet, 1e-10);

    // FakeFGModelFitFunction fakeFitFunction(sr122); // Use fit function to access current fit values

    // const double qx(7.7), qy(6.5), qz(4.3), deltaE(300);
    // const double qOmega[4] = {qx, qy, qz, deltaE};
    // Mantid::API::ExperimentInfo experimentDescr;
    // auto lattice = new Mantid::Geometry::OrientedLattice(5.51,12.298,5.57);
    // Mantid::Kernel::V3D uVec(9.800000e-03,9.996000e-01,9.700000e-03),
    //   vVec(-3.460000e-02,-4.580000e-02,9.992000e-01);
    // lattice->setUFromVectors(uVec, vVec);

    // experimentDescr.mutableSample().setOrientedLattice(lattice);
    // experimentDescr.mutableRun().addProperty("temperature_log", 6.0);

    // double weight(-1.0);
    // const ForegroundModel & sr122Function = sr122; // scatteringIntensity is private concrete model
    // TS_ASSERT_THROWS_NOTHING( weight = sr122Function.scatteringIntensity(experimentDescr, std::vector<double>(qOmega, qOmega + 4)) );
    // TS_ASSERT_DELTA(0.0000062768, weight, 1e-10);
  }

private:
  
  double calculateTestModelWeight(Mantid::MDAlgorithms::Strontium122 & model)
  {
    using Mantid::MDAlgorithms::ForegroundModel;
    FakeFGModelFitFunction fakeFitFunction(model); // Use fit function to access current fit values

    const double qx(7.7), qy(6.5), qz(4.3), deltaE(300);
    const double qOmega[4] = {qx, qy, qz, deltaE};
    Mantid::API::ExperimentInfo experimentDescr;
    auto lattice = new Mantid::Geometry::OrientedLattice(5.51,12.298,5.57);
    Mantid::Kernel::V3D uVec(9.800000e-03,9.996000e-01,9.700000e-03),
      vVec(-3.460000e-02,-4.580000e-02,9.992000e-01);
    lattice->setUFromVectors(uVec, vVec);

    experimentDescr.mutableSample().setOrientedLattice(lattice);
    delete lattice;
    experimentDescr.mutableRun().addProperty("temperature_log", 6.0);

    double weight(-1.0);
    const ForegroundModel & sr122Function = model; // scatteringIntensity is private concrete model
    TS_ASSERT_THROWS_NOTHING( weight = sr122Function.scatteringIntensity(experimentDescr, std::vector<double>(qOmega, qOmega + 4)) );
    return weight;
  }

};

#endif // MANTID_MDALGORITHMS_STRONTIUM122_H_
