// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_PLOTFITANALYSISPANEPRESENTER_H_
#define MANTIDQT_CUSTOMINTERFACES_PLOTFITANALYSISPANEPRESENTER_H_

#include "PlotFitAnalysisPaneModel.h"
#include "PlotFitAnalysisPaneView.h"

#include "MantidQtWidgets/Common/ObserverPattern.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class PlotFitAnalysisPanePresenter : public QObject {
  Q_OBJECT

public:
  PlotFitAnalysisPanePresenter(PlotFitAnalysisPaneView *m_view,
                               PlotFitAnalysisPaneModel *m_model);
  ~PlotFitAnalysisPanePresenter() {
    delete m_model;
    delete m_fitObserver;
  };
  PlotFitAnalysisPaneView *getView() { return m_view; };
  std::string getCurrentWS() { return m_currentName; };
  void clearCurrentWS() { m_currentName = ""; };
  void doFit();
  void addSpectrum(const std::string &wsName);
  void addFunction(Mantid::API::IFunction_sptr func);

private:
  VoidObserver *m_fitObserver;
  PlotFitAnalysisPaneView *m_view;
  PlotFitAnalysisPaneModel *m_model;
  std::string m_currentName;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_PLOTFITANALYSISPANEPRESENTER_H_ */
