// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/InstrumentView/BaseCustomInstrumentPresenter.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPanePresenter.h"

#include "ALFCustomInstrumentModel.h"
#include "ALFCustomInstrumentView.h"
#include "DllConfig.h"
#include "MantidQtWidgets/Common/ObserverPattern.h"

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_DIRECT_DLL ALFCustomInstrumentPresenter : public MantidWidgets::BaseCustomInstrumentPresenter {
  Q_OBJECT

public:
  ALFCustomInstrumentPresenter(IALFCustomInstrumentView *view, IALFCustomInstrumentModel *model,
                               MantidWidgets::IPlotFitAnalysisPanePresenter *analysisPane);
  ~ALFCustomInstrumentPresenter() {
    delete m_extractSingleTubeObserver;
    delete m_averageTubeObserver;
    m_analysisPane->destructor();
  };

  void addInstrument() override;

  void loadSideEffects() override;

  std::pair<instrumentSetUp, instrumentObserverOptions> setupALFInstrument();
  void setUpInstrumentAnalysisSplitter() override;

  void extractSingleTube();
  void averageTube();

private:
  IALFCustomInstrumentView *m_view;
  IALFCustomInstrumentModel *m_model;
  MantidWidgets::IPlotFitAnalysisPanePresenter *m_analysisPane;
  VoidObserver *m_extractSingleTubeObserver;
  VoidObserver *m_averageTubeObserver;
};
} // namespace CustomInterfaces
} // namespace MantidQt
