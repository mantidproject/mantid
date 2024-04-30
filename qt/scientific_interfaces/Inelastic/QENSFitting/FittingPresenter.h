// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "FittingModel.h"

#include "DllConfig.h"

#include <memory>

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

class MANTIDQT_INELASTIC_DLL FittingPresenter {
public:
  FittingPresenter(std::unique_ptr<FittingModel> model);

private:
  std::unique_ptr<FittingModel> m_model;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt