// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FitScriptGeneratorPresenter.h"

#include <stdexcept>

namespace MantidQt {
namespace MantidWidgets {
namespace Fitting {

FitScriptGeneratorPresenter::FitScriptGeneratorPresenter(
    FitScriptGeneratorView *view, FitScriptGeneratorModel *model)
    : m_view(view), m_model(model) {
  m_view->subscribePresenter(this);
}

FitScriptGeneratorPresenter::~FitScriptGeneratorPresenter() {}

void FitScriptGeneratorPresenter::notifyPresenter(ViewEvent const &event) {
  switch (event) {
  case ViewEvent::StartXChanged:
    break;
  case ViewEvent::EndXChanged:
    break;
  }

  throw std::runtime_error("Failed to notify the FitScriptGeneratorPresenter.");
}

} // namespace Fitting
} // namespace MantidWidgets
} // namespace MantidQt
