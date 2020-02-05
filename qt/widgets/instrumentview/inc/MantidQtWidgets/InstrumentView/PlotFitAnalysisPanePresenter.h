// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_INSTRUMENTVIEW_PLOTFITANALYSISPANEPRESENTER_H_
#define MANTIDQT_INSTRUMENTVIEW_PLOTFITANALYSISPANEPRESENTER_H_

#include "DllOption.h"
#include "MantidQtWidgets/Common/ObserverPattern.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPaneModel.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPaneView.h"
#include <string>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW PlotFitAnalysisPanePresenter
    : public QObject {
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
} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTIDQT_INSTRUMENTVIEW_PLOTFITANALYSISPANEPRESENTER_H_ */
