// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/SyncedCheckboxes.h"

namespace MantidQt {
namespace API {

//----------------------------------------------------------------------------------------------
/** Constructor that links a menu and a button
 *
 * @param menu :: menu to link
 * @param button :: button to link
 * @param checked :: state (checked or not) that they start in
 */
SyncedCheckboxes::SyncedCheckboxes(QAction *menu, QAbstractButton *button, bool checked)
    : m_menu(menu), m_button(button) {
  m_menu->setCheckable(true);
  m_button->setCheckable(true);
  m_menu->setChecked(checked);
  m_button->setChecked(checked);
  // Now connect each signal to this object
  connect(m_menu, SIGNAL(toggled(bool)), this, SLOT(on_menu_toggled(bool)));
  connect(m_button, SIGNAL(toggled(bool)), this, SLOT(on_button_toggled(bool)));
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */

//----------------------------------------------------------------------------------------------
/** Manually toggle the state of both checkboxes
 *
 * @param val :: True to check the boxes.
 */
void SyncedCheckboxes::toggle(bool val) {
  // Set both GUI elements
  m_button->blockSignals(true);
  m_button->setChecked(val);
  m_button->blockSignals(false);
  m_menu->blockSignals(true);
  m_menu->setChecked(val);
  m_menu->blockSignals(false);
  // Re-transmit the signal
  emit toggled(val);
}

//----------------------------------------------------------------------------------------------
/** Enable or disable both the menu and the checkboxes
 *
 * @param val :: true for Enabled
 */
void SyncedCheckboxes::setEnabled(bool val) {
  m_menu->setEnabled(val);
  m_button->setEnabled(val);
}

//----------------------------------------------------------------------------------------------
/** Set the visibility of both the menu and the checkboxes
 *
 * @param val :: true for visible
 */
void SyncedCheckboxes::setVisible(bool val) {
  m_menu->setVisible(val);
  m_button->setVisible(val);
}

//----------------------------------------------------------------------------------------------
/** Slot called when the menu is toggled */
void SyncedCheckboxes::on_menu_toggled(bool val) {
  // Adjust the state of the other
  m_button->blockSignals(true);
  m_button->setChecked(val);
  m_button->blockSignals(false);
  // Re-transmit the signal
  emit toggled(val);
}

//----------------------------------------------------------------------------------------------
/** Slot called when the button is toggled */
void SyncedCheckboxes::on_button_toggled(bool val) {
  // Adjust the state of the other
  m_menu->blockSignals(true);
  m_menu->setChecked(val);
  m_menu->blockSignals(false);
  // Re-transmit the signal
  emit toggled(val);
}

} // namespace API
} // namespace MantidQt
