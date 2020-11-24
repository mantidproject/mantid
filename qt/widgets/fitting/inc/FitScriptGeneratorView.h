// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

#include <QWidget>

namespace MantidQt {
namespace MantidWidgets {
namespace Fitting {

class FitScriptGeneratorPresenter;

class FitScriptGeneratorView : public QWidget {
public:
  enum class Event { StartXChanged, EndXChanged } const;

  FitScriptGeneratorView();
  ~FitScriptGeneratorView() = default;

  void subscribePresenter(FitScriptGeneratorPresenter *presenter);

private:
  FitScriptGeneratorPresenter *m_presenter;
};

} // namespace Fitting
} // namespace MantidWidgets
} // namespace MantidQt
