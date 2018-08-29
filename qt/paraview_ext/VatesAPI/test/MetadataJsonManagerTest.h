#ifndef METADATA_JSON_MANAGER_TEST
#define METADATA_JSON_MANAGER_TEST

#ifdef _MSC_VER
// Disabling Json warnings regarding non-export of Json::Reader and Json::Writer
#pragma warning(disable : 4275)
#pragma warning(disable : 4251)
#endif

#include "MantidVatesAPI/MetadataJsonManager.h"
#include <cxxtest/TestSuite.h>
#include <json/reader.h>
#include <string>

using Mantid::VATES::MetadataJsonManager;

class MetadataJsonManagerTest : public CxxTest::TestSuite {
public:
  void testDefaultValuesAreReturnedWhenContainerIsNotSet() {
    // Arrange
    MetadataJsonManager manager;

    // Act
    std::string instrument = manager.getInstrument();
    // Assert
    std::string expectedInstrument("_EMPTY_");

    TSM_ASSERT("The instrument string is empty, since it does not exist.",
               expectedInstrument == instrument);
  }

  void testSetValuesCanBeReadOut() {
    // Arrange
    MetadataJsonManager manager;

    std::string instrument = "OSIRIS";
    // Act
    manager.setInstrument(instrument);
    // Assert
    TSM_ASSERT_EQUALS("The instrument is read in and out.", instrument,
                      manager.getInstrument());
  }

  void testJsonStringIsReadInAndPopualtesContainer() {
    // Arrange
    MetadataJsonManager manager;
    std::string jsonString = "{\"instrument\": \"OSIRIS\"}";

    // Act
    manager.readInSerializedJson(jsonString);

    // Assert

    TSM_ASSERT("The instrument of the serialized Json string is detected.",
               manager.getInstrument() == "OSIRIS");
  }

  void testJsonStringWhichDoesNotHaveFieldsProducesDefaultValues() {
    // Arrange
    MetadataJsonManager manager;
    std::string jsonString = "{\"myInstrument\": \"OSIRIS\"}";

    // Act
    manager.readInSerializedJson(jsonString);

    // Assert
    std::string expectedInstrument = "_EMPTY_";
    TSM_ASSERT("The json object does not find the instrument field and returns "
               "default.",
               manager.getInstrument() == expectedInstrument);
  }

  void testCorrectJsonStringIsProduced() {
    // Arrange
    MetadataJsonManager manager;
    manager.setInstrument("OSIRIS");

    // Act
    std::string jsonString = manager.getSerializedJson();
    Json::Reader reader;
    Json::Value container;
    reader.parse(jsonString, container, false);

    // Assert
    TSM_ASSERT("Json string is being produced", !jsonString.empty());
    TSM_ASSERT_EQUALS("Json string contains inserted instrument.", "OSIRIS",
                      container["instrument"].asString());
  }
};
#endif