// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "FitScriptGeneratorView.h"

namespace MantidQt {
namespace MantidWidgets {

using ViewEvent = FitScriptGeneratorView::Event;

class FitScriptGeneratorModel;

class EXPORT_OPT_MANTIDQT_COMMON FitScriptGeneratorPresenter {
public:
  FitScriptGeneratorPresenter(FitScriptGeneratorView *view,
                              FitScriptGeneratorModel *model);
  ~FitScriptGeneratorPresenter();

  void notifyPresenter(ViewEvent const &event);

  void openFitScriptGenerator();

private:
  FitScriptGeneratorModel *m_model;
  FitScriptGeneratorView *m_view;
};

} // namespace MantidWidgets
} // namespace MantidQt
