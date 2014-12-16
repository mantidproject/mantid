#ifndef BACKGROUND_RGB_PROVIDER_TEST_H_
#define BACKGROUND_RGB_PROVIDER_TEST_H_

#include "MantidVatesSimpleGuiViewWidgets/BackgroundRgbProvider.h"
#include <cxxtest/TestSuite.h>


using namespace Mantid::Vates::SimpleGui;

class BackgroundRgbProviderTest : public CxxTest::TestSuite
{
  public:
    // As config service is not setup, the backgroundRgbProvider should return the default value
    void testGetTheDefaultValue()
    {
      // Arrange
      BackgroundRgbProvider backgroundRgbProvider;

      // Act
      std::vector<double> colors = backgroundRgbProvider.getRgb();

      // Assert
      TSM_ASSERT("Should have three default entries for r, g and b", colors.size()==3);
      TSM_ASSERT("Should have the default value of r", colors[0] == 84.0/255.0);
      TSM_ASSERT("Should have the default value of g", colors[1] == 89.0/255.0);
      TSM_ASSERT("Should have the default value of b", colors[2] == 109.0/255.0);
    }
};
#endif