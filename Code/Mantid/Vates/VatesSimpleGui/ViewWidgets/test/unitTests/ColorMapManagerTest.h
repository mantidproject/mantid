#ifndef BACKGROUND_RGB_PROVIDER_TEST_H_
#define BACKGROUND_RGB_PROVIDER_TEST_H_

#include "MantidVatesSimpleGuiViewWidgets/ColorMapManager.h"
#include <cxxtest/TestSuite.h>


using namespace Mantid::Vates::SimpleGui;

class ColorMapManagerTest : public CxxTest::TestSuite
{
  public:

    void testWriteAndReadIndexForValidElements()
    {
      // Arrange
      ColorMapManager manager;

      // Act
      manager.readInColorMap("test1");
      manager.readInColorMap("test2");
      manager.readInColorMap("test3");

      // Assert
      TSM_ASSERT("Should have an index of 0 as it was entered first.", manager.getColorMapIndex("test1") == 0);
      TSM_ASSERT("Should have an index of 1 as it was entered second.", manager.getColorMapIndex("test2") == 1);
      TSM_ASSERT("Should have an index of 2 as it was entered third.", manager.getColorMapIndex("test3") == 2);
    }

    void testGetsFirstIndexForInvalidColorMapRequest()
    {
      // Arrange
      ColorMapManager manager;

      // Act
      manager.readInColorMap("test1");
      manager.readInColorMap("test2");
      manager.readInColorMap("test3");

      // Assert
      TSM_ASSERT("Should have an index of 0 if the color map does not exist", manager.getColorMapIndex("wrongMap") == 0);
    }

    void testGetsFirstIndexForManagerWithoutRecordings()
    {
      // Arrange
      ColorMapManager manager;

      // Act

      // Assert
      TSM_ASSERT("Should have an index of 0 if there are no color maps recorded.", manager.getColorMapIndex("wrongMap") == 0);
    }

    void testDetectsValidAndInvalidEntries()
    {
      //Arrange
      ColorMapManager manager;

      // Act 
      manager.readInColorMap("test1");

      // Assert
      TSM_ASSERT("Should find a recorded color map", manager.isRecordedColorMap("test1"));
      TSM_ASSERT("Should not find an unrecorded color map", !manager.isRecordedColorMap("wrongMap"));
    }
};
#endif