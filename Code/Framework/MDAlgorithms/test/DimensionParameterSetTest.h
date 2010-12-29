#ifndef TEST_DIMENSION_PARAMETER_SET_H
#define TEST_DIMENSION_PARAMETER_SET_H

#include <memory>
#include "MantidMDAlgorithms/DimensionParameterNoIntegration.h"
#include "MantidMDAlgorithms/DimensionParameter.h"
#include "MantidMDAlgorithms/DimensionParameterSet.h"
#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

class  TestDimensionParameterSet : public CxxTest::TestSuite
{

public:

  void testAlignments()
  {
    using namespace Mantid::MDAlgorithms;

    boost::shared_ptr<DimensionParameterIntegration> p_integration = boost::shared_ptr<DimensionParameterIntegration>(new DimensionParameterNoIntegration);
    DimensionParameter* dimensionT = new DimensionParameter(1, "Temperature", 5, 1, p_integration);
    DimensionParameter* dimensionP = new DimensionParameter(2, "Pressure", 5, 1, p_integration);
    DimensionParameter* dimensionQx = new DimensionParameter(3, "Qx", 5, 1, p_integration);
    DimensionParameter* dimensionQy = new DimensionParameter(4, "Qy", 5, 1, p_integration);

    DimensionParameterSet set;
    set.addDimensionParameter(dimensionT);
    set.addDimensionParameter(dimensionP);
    set.addDimensionParameter(dimensionQx);
    set.addDimensionParameter(dimensionQy);

    set.setXDimension(dimensionT->getId());
    set.setYDimension(dimensionP->getId());
    set.setZDimension(dimensionQx->getId());
    set.settDimension(dimensionQy->getId());

    TSM_ASSERT_EQUALS("Dimensions x has not been aligned as instructed.", dimensionT->getId(), set.getXDimension()->getId());
    TSM_ASSERT_EQUALS("Dimensions y has not been aligned as instructed.", dimensionP->getId(), set.getYDimension()->getId());
    TSM_ASSERT_EQUALS("Dimensions z has not been aligned as instructed.", dimensionQx->getId(), set.getZDimension()->getId());
    TSM_ASSERT_EQUALS("Dimensions t has not been aligned as instructed.", dimensionQy->getId(), set.gettDimension()->getId());
  }

  void testAddSameDimensionThrows()
  {
    using namespace Mantid::MDAlgorithms;
    boost::shared_ptr<DimensionParameterIntegration> p_integration = boost::shared_ptr<DimensionParameterIntegration>(new DimensionParameterNoIntegration);
    DimensionParameter* dimensionT1 = new DimensionParameter(1, "Temperature", 5, 1, p_integration);
    DimensionParameter* dimensionT2 = new DimensionParameter(1, "Temperature", 5, 1, p_integration);

    DimensionParameterSet set;
    set.addDimensionParameter(dimensionT1);
    TSM_ASSERT_THROWS("Dimension Ids should be unique within a dimension set.", set.addDimensionParameter(dimensionT2), std::logic_error);
  }

  void testSetBadIdThrows()
  {
    using namespace Mantid::MDAlgorithms;
    boost::shared_ptr<DimensionParameterIntegration> p_integration = boost::shared_ptr<DimensionParameterIntegration>(new DimensionParameterNoIntegration);
    DimensionParameter* dimensionT = new DimensionParameter(1, "Temperature", 5, 1, p_integration);

    DimensionParameterSet set;
    set.addDimensionParameter(dimensionT);

    TSM_ASSERT_THROWS("Setting a fictional dimension id should throw.", set.setXDimension(100), std::runtime_error);
  }

};

#endif
