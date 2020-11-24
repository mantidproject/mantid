// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FitScriptGeneratorView.h"
#include "FitScriptGeneratorPresenter.h"

namespace MantidQt {
namespace MantidWidgets {
namespace Fitting {

FitScriptGeneratorView::FitScriptGeneratorView() : m_presenter() {
  m_ui.setupUi(this);
  connectSignals();
}

void FitScriptGeneratorView::connectSignals() {
  connect(m_ui.pbRemove, SIGNAL(clicked()), this, SLOT(onRemoveClicked()));
}

void FitScriptGeneratorView::subscribePresenter(
    FitScriptGeneratorPresenter *presenter) {
  m_presenter = presenter;
}

void FitScriptGeneratorView::onRemoveClicked() {
  m_presenter->notifyPresenter(ViewEvent::RemoveClicked);
}

} // namespace Fitting
} // namespace MantidWidgets
} // namespace MantidQt
