/***************************************************************************
    File                 : CustomActionDialog.cpp
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
#include "CustomActionDialog.h"
#include "ApplicationWindow.h"

#include <QPushButton>
#include <QRadioButton>
#include <QLabel>
#include <QGroupBox>
#include <QComboBox>
#include <QLineEdit>
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QListWidget>
#include <QLayout>
#include <QFileDialog>
#include <QToolBar>
#include <QMenu>
#include <QImageReader>
#include <QShortcut>
#include <QMessageBox>

CustomActionDialog::CustomActionDialog(QWidget* parent, Qt::WFlags fl)
    : QDialog(parent, fl)
{
    setWindowTitle(tr("MantidPlot") + " - " + tr("Add Custom Action"));

    itemsList = new QListWidget();
    itemsList->setSelectionMode(QAbstractItemView::SingleSelection);
	itemsList->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
	itemsList->setSpacing(2);

    QGroupBox *gb1 = new QGroupBox();
	gb1->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));

	QGridLayout *gl1 = new QGridLayout(gb1);

	gl1->addWidget(new QLabel(tr("Folder")), 0, 0);
	folderBox = new QLineEdit();

	gl1->addWidget(folderBox, 0, 1);
	folderBtn = new QPushButton(tr("Choose &Folder"));
	gl1->addWidget(folderBtn, 0, 2);

	gl1->addWidget(new QLabel(tr("Script File")), 1, 0);
	fileBox = new QLineEdit();

	gl1->addWidget(fileBox, 1, 1);
	fileBtn = new QPushButton(tr("Choose &Script"));
	gl1->addWidget(fileBtn, 1, 2);

	gl1->addWidget(new QLabel(tr("Icon")), 2, 0);
	iconBox = new QLineEdit();
	gl1->addWidget(iconBox, 2, 1);
	iconBtn = new QPushButton(tr("Choose &Icon"));
	gl1->addWidget(iconBtn, 2, 2);

	gl1->addWidget(new QLabel(tr("Text")), 3, 0);
	textBox = new QLineEdit();
	gl1->addWidget(textBox, 3, 1);

	gl1->addWidget(new QLabel(tr("Tool Tip Text")), 4, 0);
	toolTipBox = new QLineEdit();
	gl1->addWidget(toolTipBox, 4, 1);

	gl1->addWidget(new QLabel(tr("Shortcut")), 5, 0);
	shortcutBox = new QLineEdit();
	gl1->addWidget(shortcutBox, 5, 1);

    menuBtn = new QRadioButton(tr("&Menu"));
    gl1->addWidget(menuBtn, 6, 0);
    menuBox = new QComboBox();
    gl1->addWidget(menuBox, 6, 1);

    toolBarBtn = new QRadioButton(tr("&Tool Bar"));
    toolBarBtn->setChecked(true);
    gl1->addWidget(toolBarBtn, 7, 0);
    toolBarBox = new QComboBox();
    gl1->addWidget(toolBarBox, 7, 1);
    gl1->setRowStretch(8, 1);
	gl1->setColumnStretch(1, 10);

	QHBoxLayout * bottomButtons = new QHBoxLayout();
	bottomButtons->addStretch();
	buttonSave = new QPushButton(tr("&Save"));
	buttonSave->setAutoDefault( true );
	bottomButtons->addWidget(buttonSave);
	
	buttonAdd = new QPushButton(tr("&Add"));
	buttonAdd->setAutoDefault( true );
	bottomButtons->addWidget(buttonAdd);

	buttonRemove = new QPushButton(tr("&Remove"));
	buttonRemove->setAutoDefault(true);
	bottomButtons->addWidget(buttonRemove);

	buttonCancel = new QPushButton(tr("&Close"));
	buttonCancel->setAutoDefault( true );
	bottomButtons->addWidget( buttonCancel );

	QHBoxLayout *vl = new QHBoxLayout();
	vl->addWidget(itemsList);
	vl->addWidget(gb1);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(vl);
	mainLayout->addLayout(bottomButtons);

	init();

	QShortcut *accelRemove = new QShortcut(QKeySequence(Qt::Key_Delete), this);
	connect(accelRemove, SIGNAL(activated()), this, SLOT(removeAction()));

	connect(buttonSave, SIGNAL(clicked()), this, SLOT(saveCurrentAction()));
	connect(buttonAdd, SIGNAL(clicked()), this, SLOT(addAction()));
	connect(buttonRemove, SIGNAL(clicked()), this, SLOT(removeAction()));
	connect(buttonCancel, SIGNAL(clicked()), this, SLOT(reject()));
	connect(iconBtn, SIGNAL(clicked()), this, SLOT(chooseIcon()));
	connect(fileBtn, SIGNAL(clicked()), this, SLOT(chooseFile()));
	connect(folderBtn, SIGNAL(clicked()), this, SLOT(chooseFolder()));
	connect(itemsList, SIGNAL(currentRowChanged(int)), this, SLOT(setCurrentAction(int)));
}

void CustomActionDialog::init()
{
	ApplicationWindow *app = (ApplicationWindow *)parent();
	folderBox->setText(app->customActionsDirPath);

	d_menus = app->customizableMenusList();
	d_app_toolbars = app->toolBarsList();
	QList<QMenu *> d_app_menus = app->menusList();

	QStringList toolBars, menus;
	foreach (QMenu *m, d_menus){
		if (!m->title().isEmpty()){
			menus << m->title().remove("&");
	   }
    }
	menus.sort();
	menuBox->addItems(menus);

	//Build the list of shortcut key sequences and keep it to memory to improve speed!
	foreach (QMenu *m, d_app_menus){
		QList<QAction *> actionsList = m->actions();
		foreach (QAction *a, actionsList){
			QString shortcut = a->shortcut().toString();
	    	if (!shortcut.isEmpty() && !d_app_shortcut_keys.contains(shortcut))
				d_app_shortcut_keys << shortcut;
	   }
    }

	foreach (QToolBar *t, d_app_toolbars){
		toolBars << t->windowTitle();
		QList<QAction *> actionsList = t->actions();
		foreach (QAction *a, actionsList){
			QString shortcut = a->shortcut().toString();
	    	if (!shortcut.isEmpty() && !d_app_shortcut_keys.contains(shortcut))
				d_app_shortcut_keys << shortcut;
		}
    }
    toolBars.sort();
	toolBarBox->addItems(toolBars);

	updateDisplayList();
}

void CustomActionDialog::updateDisplayList()
{
	itemsList->clear();

	QList<QAction *> actionsList = ((ApplicationWindow *)parentWidget())->customActionsList();
	foreach(QAction *action, actionsList){//add existing actions to the list widget
	    QString text = action->text();
        QString shortcut = action->shortcut().toString();
	    if (!shortcut.isEmpty())
            text += " (" + shortcut + ")";

        QListWidgetItem *it = new QListWidgetItem(text, itemsList);
        if (!action->icon().isNull())
            it->setIcon(action->icon());
        itemsList->addItem(it);
	}
	itemsList->setCurrentRow(0);
	setCurrentAction(0);
}

QAction* CustomActionDialog::addAction()
{
	QAction *action = NULL;
    ApplicationWindow *app = (ApplicationWindow *)parentWidget();
    if (!app)
        return action;

	if (validUserInput()){
    	action = new QAction(app);
		customizeAction(action);

        if (toolBarBtn->isChecked()){
            foreach (QToolBar *t, d_app_toolbars){
                if (t->windowTitle() == toolBarBox->currentText()){
                    app->addCustomAction(action, t->objectName());
                    break;
                }
            }
        } else {
            foreach (QMenu *m, d_menus){
                if (m->title().remove("&") == menuBox->currentText()){
                    action->setStatusTip(m->objectName());
                    app->addCustomAction(action, m->objectName());
                    break;
                }
            }
        }

		QString text = action->text();
		QString shortcut = action->shortcut().toString();
		if (!shortcut.isEmpty())
			text += " (" + shortcut + ")";

    	QListWidgetItem *it = new QListWidgetItem(text, itemsList);
    	if (!action->icon().isNull())
        	it->setIcon(action->icon());
    	itemsList->addItem(it);
    	itemsList->setCurrentItem(it);

    	saveAction(action);
	}
    return action;
}

bool CustomActionDialog::validUserInput()
{
	QString folder = folderBox->text();
    while (folder.isEmpty() || !QFileInfo(folder).exists() || !QFileInfo(folder).isReadable()){
        chooseFolder();
		folder = folderBox->text();
	}

	ApplicationWindow *app = (ApplicationWindow *)this->parent();
	QList<QAction *>actions = app->customActionsList();

	if (textBox->text().isEmpty()){
        QMessageBox::critical(app, tr("MantidPlot") + " - " + tr("Error"),
        tr("Please provide a description for your custom action!"));
        textBox->setFocus();
        return false;
    } else if (textBox->text().contains(".")){
        QMessageBox::critical(app, tr("MantidPlot") + " - " + tr("Error"),
        tr("Dot characters are not allowed in the description text!"));
        textBox->setFocus();
        textBox->setText(textBox->text().remove(".").simplified());
        return false;
    }

    QString text = textBox->text().remove(".").simplified();
    foreach(QAction *action, actions){
        if(action->text() == text){
            QMessageBox::critical(app, tr("MantidPlot") + " - " + tr("Error"),
            tr("You have already defined an action having description: %1 <br>Please provide a different description text!").arg(textBox->text()));
            textBox->setFocus();
            return false;
        }
    }

    QString file = fileBox->text();
    QFileInfo fi(file);
    if (file.isEmpty() || !fi.exists()){
        QMessageBox::critical(app, tr("MantidPlot") + " - " + tr("Error"),
        tr("The file you have specified doesn't exist, please choose a valid script file!"));
        fileBox->setFocus();
        return false;
    }

    QString iconPath = iconBox->text();
    QFileInfo iconInfo(iconPath);
    if (!iconPath.isEmpty() && (!iconInfo.exists() || !iconInfo.isFile() || !iconInfo.isReadable())){
        iconPath = QString();
        QMessageBox::critical(app, tr("MantidPlot") + " - " + tr("Error"),
        tr("The image file you have specified doesn't exist or can't be read, please choose another file!"));
        iconBox->setFocus();
        return false;
    }

	QStringList shortcuts = d_app_shortcut_keys;
	foreach (QAction *a, actions){
		QString shortcut = a->shortcut().toString();
	    if (!shortcut.isEmpty() && !shortcuts.contains(shortcut))
			shortcuts << shortcut;
	}

	shortcuts.sort();
	QString s;
	int i = 0, n = shortcuts.count();
	while(i < n-5){
		s += shortcuts[i] + "\t" + shortcuts[i+1] + "\t" + shortcuts[i+2];
		s += "\t" + shortcuts[i+3] + "\t" + shortcuts[i+4] + "\n";
		i += 5;
	}

	if (shortcuts.contains(shortcutBox->text().remove(QRegExp("\\s")))){
		QMessageBox::critical(app, tr("MantidPlot") + " - " + tr("Error"),
        tr("Please provide a different key sequence! The following shortcut key sequences are already assigned:") +
		"\n\n" + s);
       	shortcutBox->setFocus();
        return false;
	}

	return true;
}

void CustomActionDialog::customizeAction(QAction *action)
{
	if (!action)
		return;
	
	action->setText(textBox->text().remove(".").simplified());
    action->setData(QFileInfo(fileBox->text()).absoluteFilePath());

    QIcon icon = QIcon();
	QString iconPath = iconBox->text();
	QFileInfo iconInfo(iconPath);
    if (!iconPath.isEmpty() && iconInfo.exists()){
        icon = QIcon(iconPath);
        action->setIcon(icon);
        action->setIconText(iconInfo.absoluteFilePath());
    }

    if (!toolTipBox->text().isEmpty())
        action->setToolTip(toolTipBox->text().simplified());

    if (!shortcutBox->text().isEmpty())
        action->setShortcut(shortcutBox->text().remove(QRegExp("\\s")));
}

void CustomActionDialog::removeAction()
{
	QString s = tr("Are you sure you want to remove this action?");
    if (QMessageBox::Yes != QMessageBox::question(this, tr("MantidPlot") + " - " + tr("Remove Action"), s, QMessageBox::Yes, QMessageBox::Cancel))
        return;

	int row = itemsList->currentRow();
    QAction *action = actionAt(row);
	if (!action)
		return;
	
	ApplicationWindow *app = (ApplicationWindow *)parentWidget();
    QFile f(app->customActionsDirPath + "/" + action->text() + ".qca");
    f.remove();
	
	app->removeCustomAction(action);
	
    itemsList->takeItem(row);
	if (itemsList->count())
		setCurrentAction(0);
}

void CustomActionDialog::saveCurrentAction()
{
	int row = itemsList->currentRow(); 
	QAction *action = actionAt(row);
	if (!action)
		return;
	
	QList<QWidget *> list = action->associatedWidgets();
    QWidget *w = list[0];
   	QString parentName = w->objectName();
	if ((toolBarBtn->isChecked() && w->objectName() != toolBarBox->currentText()) || 
		(menuBtn->isChecked() && w->objectName() != menuBox->currentText())){
		//relocate action: create a new one and delete the old
		ApplicationWindow *app = (ApplicationWindow *)parent();
		QAction *newAction = new QAction(app);
		customizeAction(newAction);			
		if (toolBarBtn->isChecked()){
            foreach (QToolBar *t, d_app_toolbars){
                if (t->windowTitle() == toolBarBox->currentText()){
                    app->addCustomAction(newAction, t->objectName(), row);
                    break;
                }
            }
        } else {
            foreach (QMenu *m, d_menus){
                if (m->title().remove("&") == menuBox->currentText()){
                    action->setStatusTip(m->objectName());
                    app->addCustomAction(newAction, m->objectName(), row);
                    break;
                }
            }
        }
		saveAction(newAction);
		delete action;
	} else {
		customizeAction(action);
		saveAction(action);
	}
}

void CustomActionDialog::saveAction(QAction *action)
{
    if (!action)
        return;

    ApplicationWindow *app = (ApplicationWindow *)parent();
    QString fileName = app->customActionsDirPath + "/" + action->text() + ".qca";
    QFile f(fileName);
	if (!f.open( QIODevice::WriteOnly)){
		QApplication::restoreOverrideCursor();
		QMessageBox::critical(app, tr("MantidPlot") + " - " + tr("File Save Error"),
				tr("Could not write to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(fileName));
		return;
	}

    QTextStream out( &f );
    out.setCodec("UTF-8");
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
         << "<!DOCTYPE action>\n"
         << "<action version=\"1.0\">\n";

     out << "<text>" + action->text() + "</text>\n";
     out << "<file>" + action->data().toString() + "</file>\n";
     out << "<icon>" + action->iconText() + "</icon>\n";
     out << "<tooltip>" + action->toolTip() + "</tooltip>\n";
     out << "<shortcut>" + action->shortcut().toString() + "</shortcut>\n";

     QList<QWidget *> list = action->associatedWidgets();
     QWidget *w = list[0];
     out << "<location>" + w->objectName() + "</location>\n";
     out << "</action>\n";
}

void CustomActionDialog::chooseIcon()
{
    QList<QByteArray> list = QImageReader::supportedImageFormats();
	QString filter = tr("Images") + " (", aux1, aux2;
	for (int i=0; i<(int)list.count(); i++){
		aux1 = " *." + list[i] + " ";
		aux2 += " *." + list[i] + ";;";
		filter += aux1;
	}
	filter+=");;" + aux2;

	QString fn = QFileDialog::getOpenFileName(this, tr("MantidPlot - Load icon from file"), iconBox->text(), filter);
	if (!fn.isEmpty())
		iconBox->setText(fn);
}

void CustomActionDialog::chooseFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Choose script file"), fileBox->text());
    if (!fileName.isEmpty())
        fileBox->setText(fileName);
}

void CustomActionDialog::chooseFolder()
{
    ApplicationWindow *app = (ApplicationWindow *)parentWidget();

    QString dir = QFileDialog::getExistingDirectory(this, tr("Choose the custom actions folder"), app->customActionsDirPath);
    if (!dir.isEmpty() && QFileInfo(dir).isReadable()){
		QList<QAction *> actionsList = app->customActionsList();
    	foreach (QAction *a, actionsList)
            app->removeCustomAction(a);

        app->customActionsDirPath = dir;
        app->loadCustomActions();
		updateDisplayList();
        folderBox->setText(dir);
    }
}

QAction * CustomActionDialog::actionAt(int row)
{
	ApplicationWindow *app = (ApplicationWindow *)this->parent();
	QList<QAction *>actions = app->customActionsList();
	if (actions.isEmpty() || row < 0 || row >= actions.count())
        return 0;

    return actions.at(row);
}

void CustomActionDialog::setCurrentAction(int row)
{
    QAction *action = actionAt(row);
    if (!action)
        return;

    fileBox->setText(action->data().toString());
    textBox->setText(action->text());
    iconBox->setText(action->iconText());
    toolTipBox->setText(action->toolTip());
    shortcutBox->setText(action->shortcut().toString());

    QList<QWidget *> list = action->associatedWidgets();
    QWidget *w = NULL;
    if (!list.isEmpty())
        w = list[0];
	if (!w)
		return;

	if (w->isA("QToolBar")){
    	int index = toolBarBox->findText(((QToolBar*)w)->windowTitle());
    	if (index >= 0){
        	toolBarBox->setCurrentIndex(index);
        	toolBarBtn->setChecked(true);
    	}
	} else {
        int index = menuBox->findText(((QMenu*)w)->title().remove("&"));
        if (index >= 0){
            menuBox->setCurrentIndex(index);
            menuBtn->setChecked(true);
        }
    }
}

/*****************************************************************************
 *
 * Class CustomActionHandler
 *
 *****************************************************************************/

CustomActionHandler::CustomActionHandler(QAction *action)
     : d_action(action)
 {
     metFitTag = false;
     filePath = QString();
	 d_widget_name = QString();
 }

bool CustomActionHandler::startElement(const QString & /* namespaceURI */,
                                const QString & /* localName */,
                                const QString &qName,
                                const QXmlAttributes &attributes)
{
     if (!metFitTag && qName != "action") {
         errorStr = QObject::tr("The file is not a MantidPlot custom action file.");
         return false;
     }

     if (qName == "action") {
         QString version = attributes.value("version");
         if (!version.isEmpty() && version != "1.0") {
             errorStr = QObject::tr("The file is not an MantidPlot custom action version 1.0 file.");
             return false;
         }
         metFitTag = true;
     }

     currentText.clear();
     return true;
}

bool CustomActionHandler::endElement(const QString & /* namespaceURI */,
                              const QString & /* localName */,
                              const QString &qName)
{
    if (qName == "text")
        d_action->setText(currentText);
    else if (qName == "file")
        filePath = currentText;
    else if (qName == "icon" && !currentText.isEmpty()){
        d_action->setIcon(QIcon(currentText));
        d_action->setIconText(currentText);
    } else if (qName == "tooltip")
        d_action->setToolTip(currentText);
    else if (qName == "shortcut")
        d_action->setShortcut(currentText);
    else if (qName == "location"){
		d_widget_name = currentText;
		//use status tip to store the name of the destination menu (ugly hack!)
        d_action->setStatusTip(currentText);
    } else if (qName == "action")
        d_action->setData(filePath);

    return true;
}
