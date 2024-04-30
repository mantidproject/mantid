// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FittingPresenter.h"

namespace MantidQt::CustomInterfaces::Inelastic {

FittingPresenter::FittingPresenter(std::unique_ptr<FittingModel> model) : m_model(std::move(model)) {}

} // namespace MantidQt::CustomInterfaces::Inelastic