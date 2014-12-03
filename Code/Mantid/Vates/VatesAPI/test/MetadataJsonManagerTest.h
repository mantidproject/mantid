#ifndef METADATA_JSON_MANAGER_TEST
#define METADATA_JSON_MANAGER_TEST

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/MetadataJsonManager.h"
#include <string>
#include "jsoncpp/json/reader.h"

using Mantid::VATES::MetadataJsonManager;

class MetadataJsonManagerTest: public CxxTest::TestSuite
{
  public:
    
    void testDefaultValuesAreReturnedWhenContainerIsNotSet()
    {
      // Arrange
      MetadataJsonManager manager;

      // Act
      std::string instrument = manager.getInstrument();
      double minValue = manager.getMinValue();
      double maxValue = manager.getMaxValue();

      // Assert
      double expectedMinValue = 0.0;
      double expectedMaxValue = 1.0;
      std::string expectedInstrument("_EMPTY_");
      
      TSM_ASSERT("The instrument string is empty, since it does not exist.", expectedInstrument == instrument);
      TSM_ASSERT_EQUALS("The min default value is 0.0.", expectedMinValue, minValue);
      TSM_ASSERT_EQUALS("The max default value is 1.0.", expectedMaxValue, maxValue);
    }

    void testSetValuesCanBeReadOut()
    {
      // Arrange
      MetadataJsonManager manager;

      std::string instrument = "OSIRIS";
      double minValue = 123.0;
      double maxValue = 124234.3;

      // Act
      manager.setInstrument(instrument);
      manager.setMinValue(minValue);
      manager.setMaxValue(maxValue);
 
      // Assert
      TSM_ASSERT_EQUALS("The instrument is read in and out.", instrument, manager.getInstrument());
      TSM_ASSERT_EQUALS("The min value is read in and out.", minValue, manager.getMinValue());
      TSM_ASSERT_EQUALS("The max value is read in and out.", maxValue, manager.getMaxValue());
    }

    void testJsonStringIsReadInAndPopualtesContainer()
    {
      // Arrange
      MetadataJsonManager manager;
      std::string jsonString = "{\"instrument\": \"OSIRIS\", \"minValue\":1.0, \"maxValue\": 2.0}";

      // Act 
      manager.readInSerializedJson(jsonString);

      // Assert

      TSM_ASSERT("The instrument of the serialized Json string is detected.", manager.getInstrument() == "OSIRIS");
      TSM_ASSERT_EQUALS("The min value of the serialized Json string is detected.", 1.0, manager.getMinValue());
      TSM_ASSERT_EQUALS("The max value of the serialized Json string is detected.", 2.0, manager.getMaxValue());
    }

    void testJsonStringWhichDoesNotHaveFieldsProducesDefaultValues()
    {
      // Arrange
      MetadataJsonManager manager;
      std::string jsonString = "{\"myInstrument\": \"OSIRIS\", \"myMinValue\":1.0, \"myMaxValue\": 2.0}";

      // Act 
      manager.readInSerializedJson(jsonString);

      // Assert
      std::string expectedInstrument ="_EMPTY_";
      TSM_ASSERT("The json object does not find the instrument field and returns default.", manager.getInstrument() == expectedInstrument);
      TSM_ASSERT_EQUALS("The json object does not find the max value field and returns default.", 0.0, manager.getMinValue());
      TSM_ASSERT_EQUALS("The json object does not find the min value field and returns default.", 1.0, manager.getMaxValue());
    }

    void testCorrectJsonStringIsProduced()
    {
      // Arrange
      MetadataJsonManager manager;
      manager.setInstrument("OSIRIS");
      manager.setMaxValue(3.0);
      manager.setMinValue(2.0);

      // Act
      std::string jsonString = manager.getSerializedJson();
      Json::Reader reader;
      Json::Value container;
      reader.parse(jsonString, container, false);

      // Assert
      TSM_ASSERT("Json string is being produced", !jsonString.empty());
      TSM_ASSERT_EQUALS("Json string contains inserted instrument.", "OSIRIS", container["instrument"].asString());
      TSM_ASSERT_EQUALS("Json string contains inserted min value.", 2.0, container["minValue"].asDouble());
      TSM_ASSERT_EQUALS("Json string containns inserted max value.", 3.0, container["maxValue"].asDouble());
    }
};
#endif