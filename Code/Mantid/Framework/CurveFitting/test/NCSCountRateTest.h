#ifndef MANTID_CURVEFITTING_NCSCOUNTRATETEST_H_
#define MANTID_CURVEFITTING_NCSCOUNTRATETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/NCSCountRate.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/FunctionDomain1D.h"

#include "MantidGeometry/Instrument/ComponentHelper.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <boost/make_shared.hpp>

using Mantid::CurveFitting::NCSCountRate;

class NCSCountRateTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NCSCountRateTest *createSuite() { return new NCSCountRateTest(); }
  static void destroySuite( NCSCountRateTest *suite ) { delete suite; }

  void test_Default_Function_Has_Expected_Parameters()
  {
    auto rate = createFunction();

    TS_ASSERT_EQUALS(1, rate->nParams());
    TS_ASSERT_THROWS_NOTHING(rate->getParameter("FSECoeff"));
    checkDefaultAttrsExist(*rate);
  }

  void test_Setting_Incorrect_Masses_String_Gives_Error()
  {
    using Mantid::API::IFunction;
    auto rate = createFunction();

    TS_ASSERT_THROWS(rate->setAttribute("Masses", IFunction::Attribute("")), std::invalid_argument);
    TS_ASSERT_THROWS(rate->setAttribute("Masses", IFunction::Attribute("1.008  a")), std::invalid_argument);
  }

  void test_Setting_Incorrect_Hermite_Coefficient_String_Gives_Error()
  {
    using Mantid::API::IFunction;
    auto rate = createFunction();

    TS_ASSERT_THROWS(rate->setAttribute("HermiteCoeffs", IFunction::Attribute("")), std::invalid_argument);
    TS_ASSERT_THROWS(rate->setAttribute("HermiteCoeffs", IFunction::Attribute("1.008 1 ")), std::invalid_argument); // has a double
    TS_ASSERT_THROWS(rate->setAttribute("HermiteCoeffs", IFunction::Attribute("0 a 1 ")), std::invalid_argument); // has a non-numeric character
  }


  void test_Setting_Masses_And_Hermite_Coeffs_Declares_Correct_Number_Of_Additional_Params()
  {
    using Mantid::API::IFunction;
    auto rate = createFunction();

    TS_ASSERT_THROWS_NOTHING(rate->setAttribute("Masses", IFunction::Attribute("1.008 16 33")));
    TS_ASSERT_THROWS_NOTHING(rate->setAttribute("HermiteCoeffs", IFunction::Attribute("1 0 1")));

    static const int npars = 8;
    const char * expectedNamesArr[npars] = {"Sigma_0", "C_0", "C_4", "Sigma_1", "Intens_1", "Sigma_2", "Intens_2", "FSECoeff"};

    TS_ASSERT_EQUALS(npars, rate->nParams());
    std::set<std::string> expectedNamesSet(expectedNamesArr,expectedNamesArr + npars);
    std::vector<std::string> actualNames = rate->getParameterNames();
    for(size_t i =0; i < actualNames.size(); ++i)
    {
      const std::string & name = actualNames[i];
      size_t keyCount = expectedNamesSet.count(name);
      TSM_ASSERT_EQUALS("Expected " + name + " to be found as a parameter but it was not.", 1, keyCount);
    }

    checkDefaultAttrsExist(*rate);
  }

  void test_SetWorkspace_Throws_If_Instrument_Has_No_Source_Or_Sample()
  {
    using Mantid::API::IFunction;
    auto rate = createFunction();
    auto testWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(1,10,0.0,1.0); // Empty instrument

    TS_ASSERT_THROWS(rate->setWorkspace(testWS), std::invalid_argument);
  }


  void test_SetWorkspace_Throws_If_Instrument_Does_Not_Have_Defined_Set_Of_Parameters()
  {
    using Mantid::API::IFunction;
    auto rate = createFunction();
    auto testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1,10,false);

    TS_ASSERT_THROWS(rate->setWorkspace(testWS), std::invalid_argument);
  }

  void test_SetWorkspace_Succeeds_If_Instrument_Has_Required_Set_Of_Paramters()
  {
    using Mantid::API::IFunction;
    auto rate = createFunction();
    auto testWS = createTestWorkspace();

    TS_ASSERT_THROWS_NOTHING(rate->setWorkspace(testWS));
  }

  void xtest_Function_Gives_Expected_Value_For_Given_Input()
  {
    using namespace Mantid::API;
    auto rate = createFunction();
    auto testWS = createTestWorkspace();

    const auto & xdata = testWS->readX(0);
    const size_t ndata = xdata.size();
    FunctionDomain1DView domain(xdata.data(),ndata);
    FunctionValues results(domain);
    rate->function(domain, results);

    for(size_t i = 0; i < ndata; ++i)
    {
      TS_ASSERT_DELTA(results[i], 0.0, 1e-12);
    }
  }

private:

  Mantid::API::IFunction_sptr createFunction()
  {
    auto rate = boost::make_shared<NCSCountRate>();
    rate->initialize();
    return rate;
  }

  Mantid::API::MatrixWorkspace_sptr createTestWorkspace()
  {
    using namespace Mantid::Geometry;
    const int nbins(100);
    auto ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1,nbins,false);
    // Adjust bins to be more realistic values
    for(int i = 0; i < nbins; ++i)
    {
      ws->dataX(0)[i] = 120.0 + 0.5*i;
    }

    // Add the required instrument parameters
    auto inst = ws->getInstrument();
    auto & pmap = ws->instrumentParameters();
    pmap.addDouble(inst->getComponentID(), "sigma_l1", 0.021);
    pmap.addDouble(inst->getComponentID(), "sigma_l2", 0.023);
    pmap.addDouble(inst->getComponentID(), "sigma_theta", 0.0023);
    pmap.addDouble(inst->getComponentID(), "efixed", 4908);
    pmap.addDouble(inst->getComponentID(), "hwhm_energy_lorentz", 24);
    pmap.addDouble(inst->getComponentID(), "sigma_energy_gauss", 73);

    std::vector<Mantid::Geometry::IComponent_const_sptr> dets;
    inst->getChildren(dets,true);

    for(auto it = dets.begin(); it != dets.end(); ++it)
    {
      pmap.addDouble((*it)->getComponentID(),"t0", -0.32);
    }

    Mantid::Kernel::V3D pos0;
    pos0.spherical(11.005,66.5993,10.0);
    auto det0 = ws->getDetector(0);
    ComponentHelper::moveComponent(*det0, pmap, pos0, ComponentHelper::Absolute);
    return ws;

  }

  void checkDefaultAttrsExist(const Mantid::API::IFunction & rate)
  {
    static const size_t nattrs = 4;
    TS_ASSERT_EQUALS(nattrs, rate.nAttributes());

    // Test names as they are used in scripts
    if(rate.nAttributes() > 0)
    {
      const char * attrAarr[nattrs] = {"WorkspaceIndex","Masses", "HermiteCoeffs", "BackgroundPoly"};
      std::set<std::string> expectedAttrs(attrAarr, attrAarr + nattrs);
      std::vector<std::string> actualNames = rate.getAttributeNames();

      for(size_t i = 0; i < nattrs; ++i)
      {
        const std::string & name = actualNames[i];
        size_t keyCount = expectedAttrs.count(name);
        TSM_ASSERT_EQUALS("Expected " + name + " to be found as attribute but it was not.", 1, keyCount);
      }
    }
  }

};


#endif /* MANTID_CURVEFITTING_NCSCOUNTRATETEST_H_ */
