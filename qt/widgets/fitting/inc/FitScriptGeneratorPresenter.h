// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "FitScriptGeneratorModel.h"
#include "FitScriptGeneratorView.h"

namespace MantidQt {
namespace MantidWidgets {
namespace Fitting {

using ViewEvent = FitScriptGeneratorView::Event;

class FitScriptGeneratorPresenter {
public:
  FitScriptGeneratorPresenter(FitScriptGeneratorView *view,
                              FitScriptGeneratorModel *model);
  ~FitScriptGeneratorPresenter();

  void notifyPresenter(ViewEvent const &event);

private:
  FitScriptGeneratorModel *m_model;
  FitScriptGeneratorView *m_view;
};

} // namespace Fitting
} // namespace MantidWidgets
} // namespace MantidQt
