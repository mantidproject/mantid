// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_QTREFLSETTINGSTABVIEW_H_
#define MANTID_CUSTOMINTERFACES_QTREFLSETTINGSTABVIEW_H_

#include "DllConfig.h"
#include "ui_ReflSettingsTabWidget.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

// Forward decs
class IReflSettingsTabPresenter;

/** QtReflSettingsTabView : Provides an interface for the "Settings" tab in the
ISIS Reflectometry interface.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL QtReflSettingsTabView : public QWidget {
  Q_OBJECT
public:
  /// Constructor
  QtReflSettingsTabView(QWidget *parent = nullptr);
  /// Destructor
  ~QtReflSettingsTabView() override;
  /// Returns the presenter managing this view
  IReflSettingsTabPresenter *getPresenter() const;

private:
  /// Initialise the interface
  void initLayout();

  /// The widget
  Ui::ReflSettingsTabWidget m_ui;
  /// The presenter
  std::unique_ptr<IReflSettingsTabPresenter> m_presenter;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_QTREFLSETTINGSTABVIEW_H_ */
