// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "DllOption.h"
#include "MantidQtWidgets/Common/ObserverPattern.h"
#include "MantidQtWidgets/InstrumentView/BaseCustomInstrumentModel.h"
#include "MantidQtWidgets/InstrumentView/BaseCustomInstrumentView.h"

#include <string>

namespace MantidQt {
namespace MantidWidgets {

class PlotFitAnalysisPanePresenter;

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW BaseCustomInstrumentPresenter : public QObject {
  Q_OBJECT

public:
  BaseCustomInstrumentPresenter(IBaseCustomInstrumentView *view, IBaseCustomInstrumentModel *model);

  void subscribeAnalysisPresenter(MantidQt::MantidWidgets::PlotFitAnalysisPanePresenter *presenter);

  typedef std::pair<std::string, std::vector<std::function<bool(std::map<std::string, bool>)>>> instrumentSetUp;
  typedef std::vector<std::tuple<std::string, Observer *>> instrumentObserverOptions;

  QWidget *getLoadWidget();
  MantidWidgets::InstrumentWidget *getInstrumentView();
  virtual void initLayout(std::pair<instrumentSetUp, instrumentObserverOptions> *setUp = nullptr);
  virtual void addInstrument();

  void extractSingleTube();
  void averageTube();
  virtual void loadRunNumber();

protected:
  virtual void loadAndAnalysis(const std::string &run);
  virtual void initInstrument(std::pair<instrumentSetUp, instrumentObserverOptions> *setUp);
  virtual std::pair<instrumentSetUp, instrumentObserverOptions> *setupInstrument() { return nullptr; };

  MantidQt::MantidWidgets::PlotFitAnalysisPanePresenter *m_analysisPresenter;
  IBaseCustomInstrumentView *m_view;
  IBaseCustomInstrumentModel *m_model;
  int m_currentRun;
  std::string m_currentFile;
};
} // namespace MantidWidgets
} // namespace MantidQt
