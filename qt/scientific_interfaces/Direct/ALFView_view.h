// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_ALFVIEW_VIEW_H_
#define MANTIDQT_CUSTOMINTERFACES_ALFVIEW_VIEW_H_

#include "BaseInstrumentView.h"
#include "DllConfig.h"
#include "MantidQtWidgets/Common/MWRunFiles.h"
#include "MantidQtWidgets/Common/ObserverPattern.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
#include "PlotFitAnalysisPaneView.h"

#include <QObject>
#include <QSplitter>
#include <QString>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class ALFView_view : public BaseInstrumentView {
  Q_OBJECT

public:
  explicit ALFView_view(const std::string &instrument,
                        QWidget *parent = nullptr);
  void observeExtractSingleTube(Observer *listner);
  void observeAverageTube(Observer *listner);

  void
  setUpInstrument(const std::string &fileName,
                  std::vector<std::function<bool(std::map<std::string, bool>)>>
                      &binders) override;

  void addObserver(std::tuple<std::string, Observer *> &listener) override;
  void addSpectrum(std::string wsName);
  void setupAnalysisPane(PlotFitAnalysisPaneView *analysis);

public slots:
  void extractSingleTube();
  void averageTube();

private:
  Observable *m_extractSingleTubeObservable;
  Observable *m_averageTubeObservable;
  QAction *m_extractAction;
  QAction *m_averageAction;
  PlotFitAnalysisPaneView *m_analysisPane;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_ALFVIEW_VIEW_H_ */
