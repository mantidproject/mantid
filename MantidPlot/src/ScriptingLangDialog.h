/***************************************************************************
        File                 : ScriptingLangDialog.h
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
#ifndef SCRIPTING_LANG_DIALOG_H
#define SCRIPTING_LANG_DIALOG_H

#include "Scripted.h"
#include "ScriptingEnv.h"

#include <QDialog>

class ApplicationWindow;
class QListWidget;
class QPushButton;

class ScriptingLangDialog : public QDialog, public Scripted {
  Q_OBJECT

public:
  ScriptingLangDialog(ScriptingEnv *env, ApplicationWindow *parent,
                      Qt::WFlags fl = nullptr);

public slots:
  void updateLangList();
  void accept() override;

private:
  QListWidget *langList;
  QPushButton *btnOK, *btnCancel;
};

#endif
