// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_ALFVIEWPRESENTER_H_
#define MANTIDQT_CUSTOMINTERFACES_ALFVIEWPRESENTER_H_

#include "DllConfig.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "ALFView_view.h"
#include "ALFView_model.h"
#include "observerPattern.h"

namespace MantidQt {
	namespace CustomInterfaces {
/** ALCInterface : Custom interface for Avoided Level Crossing analysis
 */
        class MANTIDQT_DIRECT_DLL ALFView_presenter : public QObject{
          Q_OBJECT

public:
  ALFView_presenter( ALFView_view *view, ALFView_model *model);
  ~ALFView_presenter(){};
  void initLayout();

private slots:
  void loadRunNumber(); 
  void loadBrowsedFile(const std::string);

private:
  void loadAndAnalysis(const std::string &run);
  ALFView_view *m_view;
  ALFView_model *m_model;
  int m_currentRun;
  loadObserver *m_loadRunObserver;
  generalObserver *m_browseObserver;
        };
}// customInterfaces
}// MantidQt




#endif /* MANTIDQT_CUSTOMINTERFACES_ALFVIEWPRESENTER_H_ */
