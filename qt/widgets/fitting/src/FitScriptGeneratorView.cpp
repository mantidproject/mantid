// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FitScriptGeneratorView.h"

namespace MantidQt {
namespace MantidWidgets {
namespace Fitting {

FitScriptGeneratorView::FitScriptGeneratorView() {}

void FitScriptGeneratorView::subscribePresenter(
    FitScriptGeneratorPresenter *presenter) {
  m_presenter = presenter;
}

} // namespace Fitting
} // namespace MantidWidgets
} // namespace MantidQt
