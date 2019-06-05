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
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

class Item;

class MANTIDQT_ISISREFLECTOMETRY_DLL IBatchJobAlgorithm {
public:
  virtual Item *item() = 0;
  virtual void updateItem() = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTID_CUSTOMINTERFACES_IBATCHJOBALGORITHM_H_
