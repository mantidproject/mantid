// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_SAVENEXUSGEOMETRY_H_
#define MANTID_DATAHANDLING_SAVENEXUSGEOMETRY_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"

namespace Mantid {

namespace DataHandling {

/* SaveNexusGeometry : A thin Algorithm wrapper over
 * NexusGeometry::saveInstrument allowing user to save the geometry from
 * instrument attached to a workspace.
 */
class MANTID_DATAHANDLING_DLL SaveNexusGeometry : public API::Algorithm {
public:
  const std::string name() const override { return "SaveNexusGeometry"; }
  int version() const override { return 1; }
  const std::string category() const override {
    return "DataHandling\\Instrument";
  }
  const std::string summary() const override {
    return "Reads the instrument from a workspace, and saves it to a Nexus "
           "file with the full path file "
           "destination and root name.";
  }

private:
  void init() override;
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_SAVENEXUSGEOMETRY_H_ */