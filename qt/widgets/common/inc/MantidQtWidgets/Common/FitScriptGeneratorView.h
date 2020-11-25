// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "ui_FitScriptGenerator.h"

#include "MantidQtWidgets/Common/MantidWidget.h"

#include <QWidget>

namespace MantidQt {
namespace MantidWidgets {

class FitScriptGeneratorPresenter;

class EXPORT_OPT_MANTIDQT_COMMON FitScriptGeneratorView
    : public API::MantidWidget {
  Q_OBJECT

public:
  enum class Event { RemoveClicked, StartXChanged, EndXChanged } const;

  FitScriptGeneratorView(QWidget *parent = nullptr);
  ~FitScriptGeneratorView() override;

  void subscribePresenter(FitScriptGeneratorPresenter *presenter);

private slots:
  void onRemoveClicked();

private:
  void connectUiSignals();

  FitScriptGeneratorPresenter *m_presenter;
  Ui::FitScriptGenerator m_ui;
};

} // namespace MantidWidgets
} // namespace MantidQt
