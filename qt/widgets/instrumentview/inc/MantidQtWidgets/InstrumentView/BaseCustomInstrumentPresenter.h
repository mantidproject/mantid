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
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPanePresenter.h"

#include <string>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW BaseCustomInstrumentPresenter : public QObject {
  Q_OBJECT

public:
  BaseCustomInstrumentPresenter(IBaseCustomInstrumentView *view, IBaseCustomInstrumentModel *model,
                                IPlotFitAnalysisPanePresenter *analysisView);
  ~BaseCustomInstrumentPresenter() { delete m_loadRunObserver; };

  typedef std::pair<std::string, std::vector<std::function<bool(std::map<std::string, bool>)>>> instrumentSetUp;
  typedef std::vector<std::tuple<std::string, Observer *>> instrumentObserverOptions;

  virtual void initLayout(std::pair<instrumentSetUp, instrumentObserverOptions> *setUp = nullptr);
  virtual void addInstrument();

protected slots:
  virtual void loadRunNumber();

protected:
  virtual void loadSideEffects(){};
  virtual void loadAndAnalysis(const std::string &run);
  virtual void initInstrument(std::pair<instrumentSetUp, instrumentObserverOptions> *setUp);
  virtual void setUpInstrumentAnalysisSplitter();
  virtual std::pair<instrumentSetUp, instrumentObserverOptions> *setupInstrument() { return nullptr; };

  IBaseCustomInstrumentView *m_view;
  IBaseCustomInstrumentModel *m_model;
  int m_currentRun;
  std::string m_currentFile;
  VoidObserver *m_loadRunObserver;
  IPlotFitAnalysisPanePresenter *m_analysisPanePresenter;
};
} // namespace MantidWidgets
} // namespace MantidQt
