// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTSETTINGSMODEL_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTSETTINGSMODEL_H_

#include "DllConfig.h"

#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INDIRECT_DLL IndirectSettingsModel {
public:
  IndirectSettingsModel();
  virtual ~IndirectSettingsModel() = default;

  virtual std::string getSettingsGroup() const;

  virtual void setFacility(std::string const &facility);
  virtual std::string getFacility() const;

private:
  std::string const m_settingsGroup;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_INDIRECTSETTINGSMODEL_H_ */
