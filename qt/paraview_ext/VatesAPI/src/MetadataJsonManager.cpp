#include "MantidVatesAPI/MetadataJsonManager.h"
#include <json/json.h>
#include <json/reader.h>
#include <json/writer.h>

namespace Mantid {
namespace VATES {
// Note that we need to have a non-empty default string
MetadataJsonManager::MetadataJsonManager()
    : instrument("_EMPTY_"), specialCoordinates(-1) {}

MetadataJsonManager::~MetadataJsonManager() {}

/**
 * Get the serialized JSON container as a string
 * @return The Json container in string format.
 */
std::string MetadataJsonManager::getSerializedJson() {
  Json::FastWriter writer;
  metadataContainer.clear();

  metadataContainer["instrument"] = instrument;
  metadataContainer["specialCoordinates"] = specialCoordinates;

  return writer.write(metadataContainer);
}

/**
 * Read in the serialized JSON data and opulate the JSON container
 * @param serializedJson The serialized JSON string.
 */
void MetadataJsonManager::readInSerializedJson(
    const std::string &serializedJson) {
  Json::Reader reader;
  metadataContainer.clear();

  bool parseSuccess = reader.parse(serializedJson, metadataContainer, false);

  if (parseSuccess) {
    // Set the instrument
    if (metadataContainer.isObject() &&
        metadataContainer.isMember("instrument")) {
      instrument = metadataContainer["instrument"].asString();
    } else {
      instrument = "_EMPTY_";
    }

    // Set the instrument
    if (metadataContainer.isObject() &&
        metadataContainer.isMember("specialCoordinates")) {
      specialCoordinates = metadataContainer["specialCoordinates"].asInt();
    } else {
      specialCoordinates = -1;
    }
  }
}

/**
 * Set the instrument.
 * @param instrument The instrument associated with the workspace.
 */
void MetadataJsonManager::setInstrument(const std::string &instrument) {
  this->instrument = instrument;
}

/**
 * Get the instrument.
 * @returns The stored instrument or the an empty string.
 */
std::string &MetadataJsonManager::getInstrument() { return instrument; }

/**
 * Set the special coordinates.
 * @param specialCoordinates The special coordinates.
 */
void MetadataJsonManager::setSpecialCoordinates(int specialCoordinates) {
  this->specialCoordinates = specialCoordinates;
}

/**
 * Get the special coordinates
 * @returns The special coordinates.
 */
int MetadataJsonManager::getSpecialCoordinates() { return specialCoordinates; }
} // namespace VATES
} // namespace Mantid
