// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_ALFVIEWPRESENTER_H_
#define MANTIDQT_CUSTOMINTERFACES_ALFVIEWPRESENTER_H_

#include "BaseInstrumentModel.h"
#include "BaseInstrumentPresenter.h"
#include "BaseInstrumentView.h"
#include "PlotFitAnalysisPanePresenter.h"

#include "ALFView_view.h"
#include "DllConfig.h"
#include "MantidQtWidgets/Common/ObserverPattern.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_DIRECT_DLL ALFView_presenter : public BaseInstrumentPresenter {
  Q_OBJECT

public:
  ALFView_presenter(ALFView_view *view, BaseInstrumentModel *model,
                    PlotFitAnalysisPanePresenter *analysisPane);
  ~ALFView_presenter(){};

protected:
  void loadSideEffects() override;

private:
  void setUpInstrumentAnalysisSplitter() override;

  ALFView_view *m_view;
  BaseInstrumentModel *m_model;
  PlotFitAnalysisPanePresenter *m_analysisPane;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_ALFVIEWPRESENTER_H_ */
