#include "MantidVatesAPI/MetadataJsonManager.h"
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/writer.h>
#include <jsoncpp/json/reader.h>

namespace Mantid
{
  namespace VATES
  {
    // Note that we need to have a non-empty default string
    MetadataJsonManager::MetadataJsonManager() : instrument("_EMPTY_"), minValue(0.0), maxValue(1.0), specialCoordinates(-1)
    {
      
    }

    MetadataJsonManager::~MetadataJsonManager()
    {
    }

    /**
     * Get the serialized JSON container as a string
     * @return The Json container in string format.
     */
    std::string MetadataJsonManager::getSerializedJson()
    {
      Json::FastWriter writer;
      metadataContainer.clear();

      metadataContainer["instrument"] = instrument;
      metadataContainer["minValue"] = minValue;
      metadataContainer["maxValue"] = maxValue;
      metadataContainer["specialCoordinates"] = specialCoordinates;

      return writer.write(metadataContainer);
    }

    /**
     * Read in the serialized JSON data and opulate the JSON container
     * @param serializedJson The serialized JSON string.
     */
    void MetadataJsonManager::readInSerializedJson(std::string serializedJson)
    {
      Json::Reader reader;
      metadataContainer.clear();

      bool parseSuccess = reader.parse(serializedJson, metadataContainer, false);

      if (parseSuccess)
      {
        // Set the max value
        if (metadataContainer.isObject() && metadataContainer.isMember("maxValue"))
        {
          maxValue = metadataContainer["maxValue"].asDouble();
        }
        else 
        {
          maxValue = 1.0;
        }

        // Set the min value
        if (metadataContainer.isObject() && metadataContainer.isMember("minValue"))
        {
          minValue = metadataContainer["minValue"].asDouble();
        }
        else 
        {
          minValue = 0.0;
        }

        // Set the instrument
        if (metadataContainer.isObject() && metadataContainer.isMember("instrument"))
        {
          instrument = metadataContainer["instrument"].asString();
        }
        else 
        {
          instrument = "_EMPTY_";
        }

        // Set the instrument
        if (metadataContainer.isObject() && metadataContainer.isMember("specialCoordinates"))
        {
          specialCoordinates = metadataContainer["specialCoordinates"].asInt();
        }
        else 
        {
          specialCoordinates = -1;
        }
      }
    }

    /**
     * Set the max value of the workspace's data range.
     * @param maxValue The max value.
     */
    void MetadataJsonManager::setMaxValue(double maxValue)
    {
      this->maxValue = maxValue;
    }

    /**
     * Get the max value of teh workspace''s data range.
     * @return The max value or 0.0.
     */
    double MetadataJsonManager::getMaxValue()
    {
      return maxValue;
    }

    /**
     * Set the min value of the workspace's data range.
     * @param minValue The min value.
     */
    void MetadataJsonManager::setMinValue(double minValue)
    {
      this->minValue = minValue;
    }

    /**
     * Get the min value of teh workspace's data range.
     * @returns The min value or 0.0;
     *
     */
    double MetadataJsonManager::getMinValue()
    {
      return minValue;
    }

    /**
     * Set the instrument.
     * @param instrument The instrument associated with the workspace.
     */
    void MetadataJsonManager::setInstrument(std::string instrument)
    {
      this->instrument = instrument;
    }

    /**
     * Get the instrument.
     * @returns The stored instrument or the an empty string.
     */
    std::string& MetadataJsonManager::getInstrument()
    {
      return instrument;
    }

    /**
     * Set the special coordinates.
     * @param specialCoordinates. The special coordinates.
     */
    void MetadataJsonManager::setSpecialCoordinates(int specialCoordinates)
    {
      this->specialCoordinates = specialCoordinates;
    }

    /**
     * Get the special coordinates
     * @returns The special coordinates.
     */
    int MetadataJsonManager::getSpecialCoordinates()
    {
      return specialCoordinates;
    }
  }
}