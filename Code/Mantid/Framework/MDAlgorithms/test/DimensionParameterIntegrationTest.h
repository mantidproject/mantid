#ifndef TEST_DIMENSION_PARAMETER_INTEGRATION_H
#define TEST_DIMENSION_PARAMETER_INTEGRATION_H

#include <memory>
#include "MantidMDAlgorithms/DimensionParameterIntegration.h"
#include "MantidMDAlgorithms/DimensionParameterNoIntegration.h"
#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

class  TestDimensionParameterIntegration: public CxxTest::TestSuite
{

public:

  void testConstructionIntegration()
  {
    Mantid::MDAlgorithms::DimensionParameterIntegration integration(1,2);

    TSM_ASSERT_EQUALS("Integration lower bounds is not wired-up correctly.", 2, integration.getLowerLimit());
    TSM_ASSERT_EQUALS("Integration upper bounds is not wired-up correctly.", 1, integration.getUpperLimit());
    TSM_ASSERT("Integration flag should always return true for this type", true == integration.isIntegrated());
  }

  void testConstructionNoIntegration()
  {
    Mantid::MDAlgorithms::DimensionParameterNoIntegration integration;
    TSM_ASSERT_THROWS("Access to upper bounds should throw if there is logically no upper bounds.", integration.getUpperLimit(), std::logic_error );
    TSM_ASSERT_THROWS("Access to upper bounds should throw if there is logically no lower bounds.", integration.getLowerLimit(), std::logic_error );
    TSM_ASSERT("Integration flag should always return false for this type", false == integration.isIntegrated());
  }

};

#endif
