/***************************************************************************
    File                 : CustomActionDialog.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2007 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : Custom Action dialog

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
#ifndef CustomActionDialog_H
#define CustomActionDialog_H

#include <QDialog>
#include <QXmlDefaultHandler>

class QGroupBox;
class QPushButton;
class QRadioButton;
class QComboBox;
class QListWidget;
class QLineEdit;
class QMenu;
class QToolBar;

class CustomActionDialog : public QDialog
{
    Q_OBJECT

public:
	//! Constructor
	/**
	 * @param parent :: parent widget (must be the application window!=
	 * @param fl :: window flags
	 */
    CustomActionDialog( QWidget* parent, Qt::WFlags fl = 0 );

private slots:
	void chooseIcon();
	void chooseFile();
	void chooseFolder();
	QAction* addAction();
	void removeAction();
	void setCurrentAction(int);
	void saveCurrentAction();

private:
	void init();
	void updateDisplayList();
	QAction* actionAt(int row);
	void saveAction(QAction *action);
	void customizeAction(QAction *action);
	bool validUserInput();
	QStringList d_app_shortcut_keys;

	QList<QMenu *> d_menus;
	QList<QToolBar *> d_app_toolbars;

    QListWidget *itemsList;
    QPushButton *buttonCancel, *buttonAdd, *buttonRemove, *buttonSave;
    QPushButton *folderBtn, *fileBtn, *iconBtn;
    QLineEdit *folderBox, *fileBox, *iconBox, *textBox, *toolTipBox, *shortcutBox;
    QRadioButton *menuBtn, *toolBarBtn;
    QComboBox *menuBox, *toolBarBox;
};

class CustomActionHandler : public QXmlDefaultHandler
{
public:
    CustomActionHandler(QAction *action);

    bool startElement(const QString &namespaceURI, const QString &localName,
                       const QString &qName, const QXmlAttributes &attributes);
    bool endElement(const QString &namespaceURI, const QString &localName,
                     const QString &qName);
    bool characters(const QString &str){currentText += str; return true;};
    bool fatalError(const QXmlParseException &){return false;};
    QString errorString() const {return errorStr;};
	QString parentName(){return d_widget_name;};

private:
    bool metFitTag;
    QString currentText;
    QString errorStr;
    QString filePath;
	QString d_widget_name;
    QAction *d_action;
};
#endif
