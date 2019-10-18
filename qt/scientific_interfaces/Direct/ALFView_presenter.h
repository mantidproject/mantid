// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_ALFVIEWPRESENTER_H_
#define MANTIDQT_CUSTOMINTERFACES_ALFVIEWPRESENTER_H_

#include "BaseInstrumentPresenter.h"
#include "PlotFitAnalysisPanePresenter.h"

#include "ALFView_view.h"
#include "ALFView_model.h"
#include "DllConfig.h"
#include "MantidQtWidgets/Common/ObserverPattern.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_DIRECT_DLL ALFView_presenter : public BaseInstrumentPresenter {
  Q_OBJECT

public:
  ALFView_presenter(ALFView_view *view, ALFView_model *model,
                    PlotFitAnalysisPanePresenter *analysisPane);
  ~ALFView_presenter() {
    delete m_extractSingleTubeObserver;
    delete m_averageTubeObserver;
    delete m_analysisPane;
    delete m_model;};

protected:
  void loadSideEffects() override;

private:

  void setUpInstrumentAnalysisSplitter() override;

  typedef std::pair<
      std::string,
      std::vector<std::function<bool(std::map<std::string, bool>)>>>
      instrumentSetUp;
  typedef std::vector<std::tuple<std::string, Observer *>>
      instrumentObserverOptions;

  std::pair<instrumentSetUp, instrumentObserverOptions> initInstrument();
  void extractSingleTube();
  void averageTube();

  ALFView_view *m_view;
  ALFView_model *m_model;
  PlotFitAnalysisPanePresenter *m_analysisPane;
  VoidObserver *m_extractSingleTubeObserver;
  VoidObserver *m_averageTubeObserver;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_ALFVIEWPRESENTER_H_ */
