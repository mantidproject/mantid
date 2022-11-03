// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ALFInstrumentModel.h"
#include "ALFInstrumentView.h"
#include "DllConfig.h"
#include "MantidQtWidgets/Common/ObserverPattern.h"

#include <string>
#include <utility>

namespace MantidQt {

namespace MantidWidgets {
class PlotFitAnalysisPanePresenter;
}

namespace CustomInterfaces {

class MANTIDQT_DIRECT_DLL ALFInstrumentPresenter : public QObject {
  Q_OBJECT

public:
  ALFInstrumentPresenter(IALFInstrumentView *view, IALFInstrumentModel *model);

  void subscribeAnalysisPresenter(MantidQt::MantidWidgets::PlotFitAnalysisPanePresenter *presenter);

  typedef std::pair<std::string, std::vector<std::function<bool(std::map<std::string, bool>)>>> instrumentSetUp;
  typedef std::vector<std::tuple<std::string, Observer *>> instrumentObserverOptions;

  QWidget *getLoadWidget();
  MantidWidgets::InstrumentWidget *getInstrumentView();
  virtual void initLayout(std::pair<instrumentSetUp, instrumentObserverOptions> &setUp);
  void addInstrument();

  void extractSingleTube();
  void averageTube();
  virtual void loadRunNumber();

protected:
  virtual void loadAndAnalysis(const std::string &run);
  virtual void initInstrument(std::pair<instrumentSetUp, instrumentObserverOptions> &setUp);
  std::pair<instrumentSetUp, instrumentObserverOptions> setupInstrument();

  MantidQt::MantidWidgets::PlotFitAnalysisPanePresenter *m_analysisPresenter;
  IALFInstrumentView *m_view;
  IALFInstrumentModel *m_model;
  int m_currentRun;
  std::string m_currentFile;
};
} // namespace CustomInterfaces
} // namespace MantidQt
