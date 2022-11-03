// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "MantidQtWidgets/InstrumentView/BaseCustomInstrumentPresenter.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPanePresenter.h"

#include "DllConfig.h"

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

private:
  std::unique_ptr<MantidWidgets::BaseCustomInstrumentModel> m_instrumentModel;
  std::unique_ptr<MantidWidgets::BaseCustomInstrumentPresenter> m_instrumentPresenter;
  std::unique_ptr<MantidWidgets::PlotFitAnalysisPanePresenter> m_analysisPresenter;
};
} // namespace CustomInterfaces
} // namespace MantidQt
