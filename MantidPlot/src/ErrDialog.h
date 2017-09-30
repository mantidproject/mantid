/***************************************************************************
    File                 : ErrDialog.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu
 Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Add error bars dialog

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
#ifndef ERRDIALOG_H
#define ERRDIALOG_H

#include <QDialog>
#include <QList>
class QLabel;
class QComboBox;
class QRadioButton;
class QLineEdit;
class QPushButton;
class QGroupBox;
class QButtonGroup;
class QCheckBox;
class MdiSubWindow;
class ApplicationWindow;

//! Add error bars dialog
class ErrDialog : public QDialog {
  Q_OBJECT

public:
  //! Constructor
  /**
   * @param parent :: parent widget
   * @param fl :: window flags
   */
  ErrDialog(ApplicationWindow *parent, Qt::WFlags fl = nullptr);

private:
  QLabel *textLabel1;
  QComboBox *nameLabel, *tableNamesBox, *colNamesBox;
  QGroupBox *groupBox2;
  QGroupBox *groupBox1, *groupBox3;
  QButtonGroup *buttonGroup1, *buttonGroup2;
  QRadioButton *standardBox, *columnBox;
  QRadioButton *percentBox;
  QLineEdit *valueBox;
  QRadioButton *xErrBox;
  QRadioButton *yErrBox;
  QPushButton *buttonAdd;
  QPushButton *buttonCancel;
  QList<MdiSubWindow *> srcTables;

  QRadioButton *mantidBox;
  QCheckBox *drawAllErrors;

protected slots:
  //! Set all string in the current language
  void languageChange() override;

public slots:
  //! Add a plot definition
  void add();
  //! Supply the dialog with a curves list
  void setCurveNames(const QStringList &names);
  //! Supply the dialog with a tables list
  void setSrcTables(QList<MdiSubWindow *> tables);
  //! Select a table
  void selectSrcTable(int tabnr);

signals:
  //! This is usually connected to the main window's defineErrorBars() slot
  void options(const QString &curveName, int type, const QString &percent,
               int direction, bool all);
  //! This is usually connected to the main window's defineErrorBars() slot
  void options(const QString &curveName, const QString &errColumnName,
               int direction);
};

#endif // ERRDIALOG_H
