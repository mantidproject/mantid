// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {

namespace API {
class Run;
}

namespace Kernel {
/// forward declaration
class PropertyMantager;
/// Typedef for a shared pointer to a PropertyManager
using PropertyManager_sptr = std::shared_ptr<PropertyManager>;
} // namespace Kernel

namespace Algorithms {

/** PDDetermineCharacterizations
 */
class MANTID_ALGORITHMS_DLL PDDetermineCharacterizations : public API::DistributedAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  std::map<std::string, std::string> validateInputs() override;

private:
  double getLogValue(API::Run &run, const std::string &propName);
  void getInformationFromTable(const double frequency, const double wavelength, const std::string &canName);
  void setDefaultsInPropManager();
  void overrideRunNumProperty(const std::string &inputName, const std::string &propName);
  void init() override;
  void exec() override;

  Kernel::PropertyManager_sptr m_propertyManager;
  API::ITableWorkspace_sptr m_characterizations;
};

} // namespace Algorithms
} // namespace Mantid
