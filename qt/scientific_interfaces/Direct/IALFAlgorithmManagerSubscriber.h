// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace MantidQt::CustomInterfaces {

class MANTIDQT_DIRECT_DLL IALFAlgorithmManagerSubscriber {

public:
  virtual ~IALFAlgorithmManagerSubscriber() = default;

  virtual void notifyAlgorithmError(std::string const &message) = 0;

  virtual void notifyLoadAndNormaliseComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) = 0;
  virtual void notifyRebinToWorkspaceComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) = 0;
  virtual void notifyReplaceSpecialValuesComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) = 0;
  virtual void notifyConvertUnitsComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) = 0;
};

} // namespace MantidQt::CustomInterfaces
