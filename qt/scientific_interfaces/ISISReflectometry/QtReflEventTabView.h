// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_QTREFLEVENTTABVIEW_H_
#define MANTID_CUSTOMINTERFACES_QTREFLEVENTTABVIEW_H_

#include "DllConfig.h"
#include "ui_ReflEventTabWidget.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

// Forward decs
class IReflEventTabPresenter;

/** QtReflEventTabView : Provides an interface for the "Event" tab in the
Reflectometry interface.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL QtReflEventTabView : public QWidget {
  Q_OBJECT
public:
  /// Constructor
  QtReflEventTabView(QWidget *parent = nullptr);
  /// Destructor
  ~QtReflEventTabView() override;
  /// Returns the presenter managing this view
  IReflEventTabPresenter *getPresenter() const;

private:
  /// Initialise the interface
  void initLayout();

  /// The widget
  Ui::ReflEventTabWidget m_ui;
  /// The presenter
  std::unique_ptr<IReflEventTabPresenter> m_presenter;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_QTREFLEVENTTABVIEW_H_ */
