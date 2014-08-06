#ifndef MANTID_MDALGORITHMS_MULLERANSATZTEST_H_
#define MANTID_MDALGORITHMS_MULLERANSATZTEST_H_

#include "MantidMDAlgorithms/Quantification/Models/MullerAnsatz.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MDFittingTestHelpers.h"

#include <cxxtest/TestSuite.h>

class MullerAnsatzTest : public CxxTest::TestSuite
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

      setParameter("Amplitude", 0.7);
      setParameter("J_coupling", 2.1);
    }

    /// Returns the number of attributes associated with the function
     virtual size_t nAttributes()const{return 2;}
     /// Returns a list of attribute names
     virtual std::vector<std::string> getAttributeNames()const
     {
       auto names = std::vector<std::string>(3);
       names[0] = "IonName";
       names[1] = "ChainDirection";
       names[2] = "MagneticFFDirection";
       return names;
     }


    std::string name() const { return "FakeFGModelFitFunction"; }
    double functionMD(const Mantid::API::IMDIterator&) const
    {
      return 0.0;
    }
  };

public:

  void test_Initialized_Model_Has_two_Parameters()
  {
    using Mantid::MDAlgorithms::MullerAnsatz;
    MullerAnsatz Cu2p;

    TS_ASSERT_EQUALS(Cu2p.nParams(), 0);
    Cu2p.initialize();
    TS_ASSERT_EQUALS(Cu2p.nParams(), 2);
  }

  void test_MANS_Has_DefaultIon_As_Cu2()
  {
    using Mantid::MDAlgorithms::MullerAnsatz;
    using Mantid::MDAlgorithms::ForegroundModel;
    MullerAnsatz Cu2Default;
    Cu2Default.initialize();
    Cu2Default.setParameter("Amplitude",0.67);
    Cu2Default.setParameter("J_coupling", 2.1);
    double valueWithDefault = calculateTestModelWeight(Cu2Default);
    TS_ASSERT_DELTA(0.016787062635810316, valueWithDefault, 1e-10); // Check the absolute value is correct

    MullerAnsatz Cu2Res;
    Cu2Res.initialize();
    Cu2Res.setParameter("Amplitude",0.67);
    Cu2Res.setParameter("J_coupling", 2.1);

    Cu2Res.setAttributeValue("IonName", "Cu2");
    Cu2Res.setAttributeValue("ChainDirection", MullerAnsatz::Along_c);
    Cu2Res.setAttributeValue("MagneticFFDirection", MullerAnsatz::Isotropic);
    
    // Same test but set the ion to check they match
    double valueWithAttrSet = calculateTestModelWeight(Cu2Res);

    TS_ASSERT_DELTA(valueWithDefault, valueWithAttrSet, 1e-10);
    //TS_ASSERT_DELTA(valueWithDefault, valueWithAttrSet, 10);


   }

private:
  
  double calculateTestModelWeight(Mantid::MDAlgorithms::MullerAnsatz & model)
  {
    using Mantid::MDAlgorithms::ForegroundModel;
    FakeFGModelFitFunction fakeFitFunction(model); // Use fit function to access current fit values

    const double qx(7.7), qy(6.5), qz(4.3), deltaE(3.3);
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
    //const ForegroundModel & MulFunction = model; // scatteringIntensity is private concrete model
    TS_ASSERT_THROWS_NOTHING( weight = model.scatteringIntensity(experimentDescr, std::vector<double>(qOmega, qOmega + 4)) );
    return weight;
  }

};

#endif // MANTID_MDALGORITHMS_STRONTIUM122_H_
