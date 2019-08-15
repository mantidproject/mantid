// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_SAVENEXUSGEOMETRY_H_
#define MANTID_DATAHANDLING_SAVENEXUSGEOMETRY_H_

#include "MantidDataHandling/DllConfig.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace DataHandling {

/** SaveNexusGeometry : TODO: DESCRIPTION
*/
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