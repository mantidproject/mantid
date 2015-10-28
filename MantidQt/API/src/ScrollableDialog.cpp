//----------------------------------
// Includes
//----------------------------------
#include "MantidQtAPI/ScrollableDialog.h"

using namespace MantidQt::API;

//-----------------------------------------------------------------------------
/**
 * Constructor.
 *
 * @param parent Optional pointer to parent widget.
 * @param f Optional window flags to set on construction.
 */
ScrollableDialog::ScrollableDialog(QWidget *parent, Qt::WindowFlags f)
    : QDialog(parent, f), m_scrollbars(this) {}

//-----------------------------------------------------------------------------
/// Destructor
ScrollableDialog::~ScrollableDialog() {}

//-----------------------------------------------------------------------------
/**
 * Check whether this dialog is currently scrollable.
 *
 * @return Returns true if the dialog is currently scrollable, false otherwise.
 */
bool ScrollableDialog::scrollable() const { return m_scrollbars.enabled(); }

//-----------------------------------------------------------------------------
/**
 * Enable or disable scrollable behaviour on this dialog.
 *
 * @param enable Whether this dialog should be scrollable or not.
 */
void ScrollableDialog::setScrollable(bool enable) {
  m_scrollbars.setEnabled(enable);
}
