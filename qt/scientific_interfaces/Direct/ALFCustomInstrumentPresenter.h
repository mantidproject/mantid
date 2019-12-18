// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_ALFCUSTOMINSTRUMENTPRESENTER_H_
#define MANTIDQT_CUSTOMINTERFACES_ALFCUSTOMINSTRUMENTPRESENTER_H_

#include "MantidQtWidgets/InstrumentView/BaseCustomInstrumentPresenter.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPanePresenter.h"

#include "ALFCustomInstrumentModel.h"
#include "ALFCustomInstrumentView.h"
#include "DllConfig.h"
#include "MantidQtWidgets/Common/ObserverPattern.h"

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_DIRECT_DLL ALFCustomInstrumentPresenter
    : public MantidWidgets::BaseCustomInstrumentPresenter {
  Q_OBJECT

public:
  ALFCustomInstrumentPresenter(
      ALFCustomInstrumentView *view, ALFCustomInstrumentModel *model,
      MantidWidgets::PlotFitAnalysisPanePresenter *analysisPane);
  ~ALFCustomInstrumentPresenter() {
    delete m_extractSingleTubeObserver;
    delete m_averageTubeObserver;
    delete m_analysisPane;
    delete m_model;
  };

  void addInstrument() override;

protected:
  void loadSideEffects() override;

  std::pair<instrumentSetUp, instrumentObserverOptions> setupALFInstrument();

private:
  void setUpInstrumentAnalysisSplitter() override;

  void extractSingleTube();
  void averageTube();

  ALFCustomInstrumentView *m_view;
  ALFCustomInstrumentModel *m_model;
  MantidWidgets::PlotFitAnalysisPanePresenter *m_analysisPane;
  VoidObserver *m_extractSingleTubeObserver;
  VoidObserver *m_averageTubeObserver;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_ALFCUSTOMINSTRUMENTPRESENTER_H_ */
