// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_SETSPECIALCOORDINATES_H_
#define MANTID_CRYSTAL_SETSPECIALCOORDINATES_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/System.h"
#include <string>
#include <vector>

namespace Mantid {
namespace Crystal {

/** SetSpecialCoordinates :
 *

  Set the special coordinates on an IMDWorspace or peaksworkspace. Also print
 out any existing special coordinates.
*/
class DLLExport SetSpecialCoordinates : public API::Algorithm {
public:
  SetSpecialCoordinates();

  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Set or overwrite any Q3D special coordinates.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"ConvertToMD", "ConvertToDiffractionMDWorkspace"};
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
  std::vector<std::string> m_specialCoordinatesNames;
  using SpecialCoordinatesNameMap =
      std::map<std::string, Mantid::Kernel::SpecialCoordinateSystem>;
  SpecialCoordinatesNameMap m_specialCoordinatesMap;
  static const std::string QLabOption();
  static const std::string QSampleOption();
  static const std::string HKLOption();
  bool writeCoordinatesToMDEventWorkspace(
      Mantid::API::Workspace_sptr inWS,
      Mantid::Kernel::SpecialCoordinateSystem coordinateSystem);
  bool writeCoordinatesToMDHistoWorkspace(
      Mantid::API::Workspace_sptr inWS,
      Mantid::Kernel::SpecialCoordinateSystem coordinateSystem);
  bool writeCoordinatesToPeaksWorkspace(
      Mantid::API::Workspace_sptr inWS,
      Mantid::Kernel::SpecialCoordinateSystem coordinateSystem);
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_SETSPECIALCOORDINATES_H_ */
