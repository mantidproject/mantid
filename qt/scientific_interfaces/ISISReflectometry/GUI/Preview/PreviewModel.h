// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "IPreviewModel.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <memory>
#include <string>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class MANTIDQT_ISISREFLECTOMETRY_DLL PreviewModel : public IPreviewModel {
public:
  virtual ~PreviewModel() = default;
  void loadWorkspace(std::string const &workspaceName) override;
  Mantid::API::MatrixWorkspace_sptr getInstViewWorkspace() const override;

private:
  Mantid::API::MatrixWorkspace_sptr m_instViewWorkspace;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
