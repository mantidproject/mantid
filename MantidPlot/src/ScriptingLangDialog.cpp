/***************************************************************************
        File                 : ScriptingLangDialog.cpp
        Project              : QtiPlot
--------------------------------------------------------------------
        Copyright            : (C) 2006 by Knut Franke, Ion Vasilief
        Email (use @ for *)  : knut.franke*gmx.de, ion_vasilief*yahoo.fr
        Description          : Dialog for changing the current scripting
                               language

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
#include "ScriptingLangDialog.h"
#include "ApplicationWindow.h"

#include <QLayout>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>

ScriptingLangDialog::ScriptingLangDialog(ScriptingEnv *env,
                                         ApplicationWindow *parent,
                                         Qt::WFlags fl)
    : QDialog(parent, fl), Scripted(env) {
  setWindowTitle(tr("MantidPlot - Select scripting language"));

  langList = new QListWidget(this);

  btnOK = new QPushButton(tr("OK"));
  btnCancel = new QPushButton(tr("Cancel"));

  QHBoxLayout *hbox1 = new QHBoxLayout();
  hbox1->addStretch();
  hbox1->addWidget(btnOK);
  hbox1->addWidget(btnCancel);

  QVBoxLayout *vl = new QVBoxLayout(this);
  vl->addWidget(langList);
  vl->addLayout(hbox1);

  connect(btnOK, SIGNAL(clicked()), this, SLOT(accept()));
  connect(btnCancel, SIGNAL(clicked()), this, SLOT(close()));
  connect(langList, SIGNAL(itemActivated(QListWidgetItem *)), this,
          SLOT(accept()));

  updateLangList();
}

void ScriptingLangDialog::updateLangList() {
  langList->clear();
  langList->insertItems(0, ScriptingLangManager::languages());
  QListWidgetItem *current =
      langList->findItems(scriptingEnv()->objectName(), Qt::MatchExactly)
          .first();
  if (current)
    langList->setCurrentItem(current);
}

void ScriptingLangDialog::accept() {
  ApplicationWindow *app = static_cast<ApplicationWindow *>(parent());
  if (app->setScriptingLanguage(langList->currentItem()->text()))
    close();
}
