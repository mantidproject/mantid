// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ALFAnalysisPresenter.h"
#include "ALFInstrumentPresenter.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"

#include "DllConfig.h"

#include <QPushButton>
#include <QWidget>

#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_DIRECT_DLL ALFView : public API::UserSubWindow {
  Q_OBJECT

public:
  ALFView(QWidget *parent = nullptr);
  static std::string name() { return "ALF View"; }
  static QString categoryInfo() { return "Direct"; }

protected:
  void initLayout() override;

private slots:
  void openHelp();

private:
  QWidget *createHelpWidget();

  QPushButton *m_help;

  std::unique_ptr<ALFInstrumentPresenter> m_instrumentPresenter;
  std::unique_ptr<ALFAnalysisPresenter> m_analysisPresenter;
};

} // namespace CustomInterfaces
} // namespace MantidQt
