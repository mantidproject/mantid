// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_ALFVIEW_H_
#define MANTIDQT_CUSTOMINTERFACES_ALFVIEW_H_

#include "ALFView_model.h"
#include "ALFView_presenter.h"
#include "ALFView_view.h"
#include "DllConfig.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "MantidQtWidgets/Common/ObserverPattern.h"

namespace MantidQt {
namespace CustomInterfaces {
/** ALFView : Custom interface for looking at ALF data
 */
class MANTIDQT_DIRECT_DLL ALFView : public API::UserSubWindow {
  Q_OBJECT

public:
  ALFView(QWidget *parent = nullptr);
  ~ALFView() {
    delete m_presenter;
    delete m_model;
    delete m_extractSingleTubeObserver;
    delete m_averageTubeObserver;
  };
  static std::string name() { return "ALF View"; }
  static QString categoryInfo() { return "Direct"; }

protected:
  void initLayout() override;

private:

typedef std::pair < std::string,
      std::vector<std::function<bool(std::map<std::string, bool>)>>>
          instrumentSetUp;
  typedef std::vector<std::tuple<std::string, Observer *>>
      instrumentObserverOptions;

  std::pair<instrumentSetUp, instrumentObserverOptions> initInstrument();

  ALFView_view *m_view;
  ALFView_model *m_model;
  ALFView_presenter *m_presenter;
  VoidObserver *m_extractSingleTubeObserver;
  VoidObserver *m_averageTubeObserver;

};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_ALFVIEW_H_ */
