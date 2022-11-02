// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/InstrumentView/BaseCustomInstrumentPresenter.h"

#include "ALFCustomInstrumentModel.h"
#include "ALFCustomInstrumentView.h"
#include "DllConfig.h"
#include "MantidQtWidgets/Common/ObserverPattern.h"

#include <string>

namespace MantidQt {

namespace MantidWidgets {
class PlotFitAnalysisPanePresenter;
}

namespace CustomInterfaces {

class MANTIDQT_DIRECT_DLL ALFCustomInstrumentPresenter : public MantidWidgets::BaseCustomInstrumentPresenter {
  Q_OBJECT

public:
  ALFCustomInstrumentPresenter(IALFCustomInstrumentView *view, IALFCustomInstrumentModel *model);
  ~ALFCustomInstrumentPresenter() {
    delete m_extractSingleTubeObserver;
    delete m_averageTubeObserver;
  };

  void subscribeAnalysisPresenter(MantidQt::MantidWidgets::PlotFitAnalysisPanePresenter *presenter);

  void addInstrument() override;

  std::pair<instrumentSetUp, instrumentObserverOptions> setupALFInstrument();

  void extractSingleTube();
  void averageTube();

private:
  MantidQt::MantidWidgets::PlotFitAnalysisPanePresenter *m_analysisPresenter;
  IALFCustomInstrumentView *m_view;
  IALFCustomInstrumentModel *m_model;
  VoidObserver *m_extractSingleTubeObserver;
  VoidObserver *m_averageTubeObserver;
};
} // namespace CustomInterfaces
} // namespace MantidQt
