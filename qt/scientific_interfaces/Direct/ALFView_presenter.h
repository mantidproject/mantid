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

namespace MantidQt {
	namespace CustomInterfaces {
/** ALCInterface : Custom interface for Avoided Level Crossing analysis
 */
class MANTIDQT_DIRECT_DLL ALFView : public API::UserSubWindow {
  Q_OBJECT


public:
  ALFView(QWidget *parent = nullptr);
  ~ALFView(){};
  static std::string name() { return "ALF View"; }
  static QString categoryInfo() { return "Direct"; }

protected:
  void initLayout() override;

private slots:
  void loadRunNumber(); 
  void loadBrowsedFile(const std::string &fileName);

private:
  ALFView_view *m_view;
};
}// customInterfaces
}// MantidQt




#endif /* MANTIDQT_CUSTOMINTERFACES_ALFVIEWPRESENTER_H_ */
