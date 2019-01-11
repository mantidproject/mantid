// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef METADATA_JSON_MANAGER_H
#define METADATA_JSON_MANAGER_H

#include "MantidKernel/System.h"
#include <json/json.h>
#include <string>

namespace Mantid {
namespace VATES {
/** Metadata container and handler to handle json data which is passed between
filters and sources through
    VTK field data

@date 31/11/2014
*/

class DLLExport MetadataJsonManager {
public:
  MetadataJsonManager();
  ~MetadataJsonManager();
  std::string getSerializedJson();
  void readInSerializedJson(const std::string &serializedJson);
  void setInstrument(const std::string &instrument);
  std::string &getInstrument();
  void setSpecialCoordinates(int specialCoordinates);
  int getSpecialCoordinates();

private:
  Json::Value metadataContainer;
  std::string instrument;
  int specialCoordinates;
};
} // namespace VATES
} // namespace Mantid
#endif
