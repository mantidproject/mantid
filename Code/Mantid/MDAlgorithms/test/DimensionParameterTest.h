#ifndef TEST_DIMENSION_PARAMETER_H
#define TEST_DIMENSION_PARAMETER_H

#include <memory>
#include "DimensionParameter.h"
#include "DimensionParameterIntegration.h"
#include "DimensionParameterNoIntegration.h"
#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

class  TestDimensionParameter : public CxxTest::TestSuite
{

public:

  void testConstruction()
  {
    using namespace Mantid::MDAlgorithms;
    DimensionParameterIntegration* p_integration = new DimensionParameterIntegration(2, 4);
    DimensionParameter dimensionParameter(1, "Temperature Parameter", 5, 1, boost::shared_ptr<DimensionParameterIntegration>(p_integration));

    TSM_ASSERT_EQUALS("Parameter name not wired-up correctly.", "Temperature Parameter", dimensionParameter.getName());
    TSM_ASSERT_EQUALS("Parameter upper bound not wired-up correctly.", 5, dimensionParameter.getUpperBound());
    TSM_ASSERT_EQUALS("Parameter lower bound not wired-up correctly.", 1, dimensionParameter.getLowerBound());
    TSM_ASSERT_EQUALS("Parameter integration getter not wired-up correctly.", 4, dimensionParameter.getIntegration()->getLowerLimit());
    TSM_ASSERT_EQUALS("Parameter integration getter not wired-up correctly.", 2, dimensionParameter.getIntegration()->getUpperLimit());
    TSM_ASSERT_EQUALS("Parameter id getter not wired-up correctly.", 1, dimensionParameter.getId());
  }

  void testBadIntegrationConflictLowerBounds()
  { 
    using namespace Mantid::MDAlgorithms;
    DimensionParameterIntegration* p_integration = new DimensionParameterIntegration(4, 0); // lower limit for integration out of range.

    //Construction should throw.
    TSM_ASSERT_THROWS("Integration lower limit below that of dimension lower bound. Should throw.", DimensionParameter(1, "Temperature Parameter", 5, 1, boost::shared_ptr<DimensionParameterIntegration>(p_integration)), std::out_of_range );
  }

  void testBadIntegrationConflictUpperBounds()
  { 	
    using namespace Mantid::MDAlgorithms;
    DimensionParameterIntegration* p_integration = new DimensionParameterIntegration(6, 1); // lower limit for integration out of range.

    //Construction should throw.
    TSM_ASSERT_THROWS("Integration upper limit above that of dimension upper bound. Should throw.", DimensionParameter(1, "Temperature Parameter", 5, 1, boost::shared_ptr<DimensionParameterIntegration>(p_integration)), std::out_of_range );

  }

  void testUpperBoundsBelowLowerBoundsThrows()
  { 	
    using namespace Mantid::MDAlgorithms;
    DimensionParameterIntegration* p_integration = new DimensionParameterNoIntegration; 

    //Construction should not throw.
    TSM_ASSERT_THROWS("Dimension upper bounds must be above integration lower bounds.", DimensionParameter(1, "Temperature Parameter", 0, 1, boost::shared_ptr<DimensionParameterIntegration>(p_integration)), std::logic_error );

  }

  void testNoIntegration()
  {
    using namespace Mantid::MDAlgorithms;
    DimensionParameterIntegration* p_integration = new DimensionParameterNoIntegration; 

    //Construction should not throw.
    TSM_ASSERT_THROWS_NOTHING("Should not throw as no integration in effect.", DimensionParameter(1, "Temperature Parameter", 5, 1, boost::shared_ptr<DimensionParameterIntegration>(p_integration)));

  }

};

#endif