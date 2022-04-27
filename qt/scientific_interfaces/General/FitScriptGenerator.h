// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/FitScriptGeneratorModel.h"
#include "MantidQtWidgets/Common/FitScriptGeneratorPresenter.h"
#include "MantidQtWidgets/Common/FitScriptGeneratorView.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"

#include <memory>

#include <QString>
#include <QVariant>
#include <QWidget>

namespace MantidQt {
namespace CustomInterfaces {

class FitScriptGenerator : public MantidQt::API::UserSubWindow {
  Q_OBJECT

public:
  FitScriptGenerator(QWidget *parent = nullptr);
  ~FitScriptGenerator();

  static std::string name() { return "Fit Script Generator"; }
  static QString categoryInfo() { return "General"; }

  void initLayout() override;

private:
  MantidQt::MantidWidgets::FitScriptGeneratorView *m_view;
  std::unique_ptr<MantidQt::MantidWidgets::FitScriptGeneratorModel> m_model;
  std::unique_ptr<MantidQt::MantidWidgets::FitScriptGeneratorPresenter> m_presenter;
};

} // namespace CustomInterfaces
} // namespace MantidQt
