// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ALFInstrumentModel.h"
#include "DllConfig.h"

#include <optional>
#include <string>
#include <utility>

#include <QObject>
#include <QWidget>

namespace MantidQt {

namespace MantidWidgets {
class InstrumentWidget;
class PlotFitAnalysisPanePresenter;
} // namespace MantidWidgets

namespace CustomInterfaces {

class IALFInstrumentView;

class MANTIDQT_DIRECT_DLL ALFInstrumentPresenter : public QObject {
  Q_OBJECT

public:
  ALFInstrumentPresenter(IALFInstrumentView *view, std::unique_ptr<IALFInstrumentModel> model);

  QWidget *getLoadWidget();
  MantidWidgets::InstrumentWidget *getInstrumentView();

  void subscribeAnalysisPresenter(MantidQt::MantidWidgets::PlotFitAnalysisPanePresenter *presenter);

  void loadRunNumber();

  void extractSingleTube();
  void averageTube();

  bool showAverageTubeOption() const;

private:
  std::optional<std::string> loadAndTransform(const std::string &run);

  MantidQt::MantidWidgets::PlotFitAnalysisPanePresenter *m_analysisPresenter;

  IALFInstrumentView *m_view;
  std::unique_ptr<IALFInstrumentModel> m_model;
};
} // namespace CustomInterfaces
} // namespace MantidQt
