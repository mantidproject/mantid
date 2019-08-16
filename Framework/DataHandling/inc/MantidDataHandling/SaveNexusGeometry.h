// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

/* SaveNexusGeometry : A thin Algorithm wrapper over
 * NexusGeometry::saveInstrument allowing user to save the geometry from
 * instrument attached to a workspace.
 *
 * @author Takudzwa Makoni, RAL (UKRI), ISIS
 * @date 16/08/2019
 */
#ifndef MANTID_DATAHANDLING_SAVENEXUSGEOMETRY_H_
#define MANTID_DATAHANDLING_SAVENEXUSGEOMETRY_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"

namespace Mantid {

namespace DataHandling {

class MANTID_DATAHANDLING_DLL SaveNexusGeometry : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_SAVENEXUSGEOMETRY_H_ */