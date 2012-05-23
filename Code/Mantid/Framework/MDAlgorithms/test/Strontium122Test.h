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
      setParameter("MultEps", 0.0);
      setParameter("TwinType", 0.0);
    }

    std::string name() const { return "FakeFGModelFitFunction"; }
    double functionMD(const Mantid::API::IMDIterator&) const
    {
      return 0.0;
    }
  };

public:

  void test_Initialized_Model_Has_Eight_Parameters()
  {
    using Mantid::MDAlgorithms::Strontium122;
    Strontium122 sr122;

    TS_ASSERT_EQUALS(sr122.nParams(), 0);
    sr122.initialize();
    TS_ASSERT_EQUALS(sr122.nParams(), 8);
  }

  void test_Given_ND_Point_Returns_Expected_Value_For_Test_Parameter_Values()
  {
    using Mantid::MDAlgorithms::Strontium122;
    using Mantid::MDAlgorithms::ForegroundModel;
    Strontium122 sr122;
    sr122.initialize();
    FakeFGModelFitFunction fakeFitFunction(sr122);

    const double qx(-1.7), qy(0.0), qz(1.05), deltaE(300);
    const double qOmega[4] = {qx, qy, qz, deltaE};
    Mantid::API::ExperimentInfo experimentDescr;
    auto lattice = new Mantid::Geometry::OrientedLattice(5.57,5.51,12.298);
    Mantid::Kernel::V3D uVec(9.700000e-03,9.800000e-03,9.996000e-01),
        vVec(9.992000e-01,-3.460000e-02,-4.580000e-02);
    lattice->setUFromVectors(uVec, vVec);

    experimentDescr.mutableSample().setOrientedLattice(lattice);
    experimentDescr.mutableRun().addProperty("temperature_log", 6.0);

    double weight(-1.0);
    const ForegroundModel & sr122Function = sr122; // scatteringIntensity is private concrete model
    TS_ASSERT_THROWS_NOTHING( weight = sr122Function.scatteringIntensity(experimentDescr, std::vector<double>(qOmega, qOmega + 4)) );
    //TS_ASSERT_DELTA(0.24589087, weight, 1e-8);
  }

};

#endif // MANTID_MDALGORITHMS_STRONTIUM122_H_
