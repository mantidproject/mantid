// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "ui_FitScriptGenerator.h"

#include <QWidget>

namespace MantidQt {
namespace MantidWidgets {

class FitScriptGeneratorPresenter;

class FitScriptGeneratorView : public QWidget {
  Q_OBJECT

public:
  enum class Event { RemoveClicked, StartXChanged, EndXChanged } const;

  FitScriptGeneratorView();
  ~FitScriptGeneratorView() = default;

  void subscribePresenter(FitScriptGeneratorPresenter *presenter);

private slots:
  void onRemoveClicked();

private:
  void connectSignals();

  FitScriptGeneratorPresenter *m_presenter;
  Ui::FitScriptGenerator m_ui;
};

} // namespace MantidWidgets
} // namespace MantidQt
