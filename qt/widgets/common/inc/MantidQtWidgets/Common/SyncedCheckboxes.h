// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_API_SYNCEDCHECKBOXES_H_
#define MANTIDQT_API_SYNCEDCHECKBOXES_H_

#include "DllOption.h"
#include "MantidKernel/System.h"
#include <QAction>
#include <QPushButton>
#include <QWidget>

namespace MantidQt {
namespace API {

/** QObject that links:

  - a checkable QAction (menu item)
  - and a checkable QAbstractButton (like a toolbar button)

  so that their checked status is consistent.
  Emits a single "toggled" signal if either one is toggled.

  @date 2011-12-06
*/
class EXPORT_OPT_MANTIDQT_COMMON SyncedCheckboxes : public QObject {
  Q_OBJECT

public:
  SyncedCheckboxes(QAction *menu, QAbstractButton *button,
                   bool checked = false);
  ~SyncedCheckboxes() override;
  void toggle(bool val);
  void setEnabled(bool val);
  void setVisible(bool val);

signals:
  /// Signal emitted when the check box is toggled
  void toggled(bool /*_t1*/);

public slots:
  void on_menu_toggled(bool /*val*/);
  void on_button_toggled(bool /*val*/);

private:
  QAction *m_menu;
  QAbstractButton *m_button;
};

} // namespace API
} // namespace MantidQt

#endif /* MANTIDQT_API_SYNCEDCHECKBOXES_H_ */
