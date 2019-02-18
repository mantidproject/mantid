// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_IBATCHJOBALGORITHM_H_
#define MANTID_CUSTOMINTERFACES_IBATCHJOBALGORITHM_H_

#include "Common/DllConfig.h"
#include "IBatchJobAlgorithm.h"
#include "MantidAPI/Workspace_fwd.h"

#include <map>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class Item;

class MANTIDQT_ISISREFLECTOMETRY_DLL IBatchJobAlgorithm {
public:
  virtual Item *item() = 0;
  virtual std::vector<std::string> outputWorkspaceNames() const = 0;
  virtual std::map<std::string, Mantid::API::Workspace_sptr>
  outputWorkspaceNameToWorkspace() const = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTID_CUSTOMINTERFACES_IBATCHJOBALGORITHM_H_
