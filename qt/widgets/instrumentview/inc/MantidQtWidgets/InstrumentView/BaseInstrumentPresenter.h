// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_BASEINSTRUMENTPRESENTER_H_
#define MANTIDQT_CUSTOMINTERFACES_BASEINSTRUMENTPRESENTER_H_
#include "DllOption.h"

#include "MantidQtWidgets/InstrumentView/BaseInstrumentModel.h"
#include "MantidQtWidgets/InstrumentView/BaseInstrumentView.h"

#include "MantidQtWidgets/Common/ObserverPattern.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"

#include <string>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW BaseInstrumentPresenter
    : public QObject {
  Q_OBJECT

public:
  BaseInstrumentPresenter(BaseInstrumentView *view, BaseInstrumentModel *model,
                          QWidget *analysisView);
  ~BaseInstrumentPresenter() { delete m_loadRunObserver; };

  typedef std::pair<
      std::string,
      std::vector<std::function<bool(std::map<std::string, bool>)>>>
      instrumentSetUp;
  typedef std::vector<std::tuple<std::string, Observer *>>
      instrumentObserverOptions;

  void initLayout(
      std::pair<instrumentSetUp, instrumentObserverOptions> *setUp = nullptr);
  virtual void addInstrument();

protected:
  virtual void loadSideEffects(){};

private slots:
  void loadRunNumber();

private:
  void loadAndAnalysis(const std::string &run);
  void
  initInstrument(std::pair<instrumentSetUp, instrumentObserverOptions> *setUp);
  virtual void setUpInstrumentAnalysisSplitter();
  std::pair<instrumentSetUp, instrumentObserverOptions> setupInstrument();

  BaseInstrumentView *m_view;
  BaseInstrumentModel *m_model;
  int m_currentRun;
  std::string m_currentFile;
  VoidObserver *m_loadRunObserver;
  QWidget *m_analysisPaneView;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_BASEINSTRUMENTPRESENTER_H_ */
