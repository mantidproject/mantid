/***************************************************************************
    File                 : ExtensibleFileDialog.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2007 by Knut Franke, Ion Vasilief
    Email (use @ for *)  : knut.franke*gmx.de, ion_vasilief*yahoo.fr
    Description          : QFileDialog plus generic extension support

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef EXTENSIBLE_FILE_DIALOG_H
#define EXTENSIBLE_FILE_DIALOG_H

#include <QFileDialog>
#include <QPushButton>

//! QFileDialog plus generic extension support.
/**
 * This is a simple hack on top of QFileDialog that allows a custom extension
 *widget to be added to
 * the bottom of the dialog. A button is provided for toggling display of this
 *widget on/off.
 *
 * For the placement of button and extension widget, it is assumed that
 *QFileDialog uses a
 * QGridLayout as its top-level layout. Other layouts will probably lead to a
 *strange outlook,
 * although the functionality should stay intact.
 */
class ExtensibleFileDialog : public QFileDialog {
  Q_OBJECT

public:
  //! Constructor.
  /**
   * \param parent parent widget (only affects placement of the dialog)
   * \param extended flag: show/hide the advanced options on start-up
   * \param flags window flags
   */
  ExtensibleFileDialog(QWidget *parent = nullptr, bool extended = true,
                       Qt::WFlags flags = nullptr);
  //! Set the extension widget to be displayed when the user presses the toggle
  // button.
  void setExtensionWidget(QWidget *extension);

  //! Tells weather the dialog has a valid extension widget
  bool isExtendable() { return d_extension != nullptr; };
  bool isExtended() { return d_extension_toggle->isChecked(); };
  //! Toggle extension widget on/off
  void setExtended(bool extended);
  //! Sets the text to be displayed in the toggle button
  void setExtentionToggleButtonText(const QString &text) {
    d_extension_toggle->setText(text);
  };
  //! Enables/Disables editing of the file type filter
  void setEditableFilter(bool on = true);

private slots:
  void updateToggleButtonText(bool);

protected:
  //! Button for toggling display of extension on/off.
  QPushButton *d_extension_toggle;

private:
  //! The extension widget
  QWidget *d_extension;
  //! The layout row (of the assumed QGridLayout) used for extensions
  int d_extension_row;
};

#endif // ifndef EXTENSIBLE_FILE_DIALOG_H
