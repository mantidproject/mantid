/***************************************************************************
    File                 : Note.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Tilman Hoener zu
 Siederdissen
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
    Description          : Notes window class

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
#ifndef NOTE_H
#define NOTE_H

#include "MantidQtWidgets/Common/IProjectSerialisable.h"
#include "MdiSubWindow.h"
#include <QTextEdit>

class ApplicationWindow;

/**\brief Notes window class.
 *
 * \section future Future Plans
 * - Search and replace
 */
class Note : public MdiSubWindow {
  Q_OBJECT

public:
  Note(const QString &label, QWidget *parent, const QString &name = QString(),
       Qt::WFlags f = nullptr);
  ~Note() override{};

  static MantidQt::API::IProjectSerialisable *
  loadFromProject(const std::string &lines, ApplicationWindow *app,
                  const int fileVersion);
  std::string saveToProject(ApplicationWindow *app) override;
  std::vector<std::string> getWorkspaceNames() override;

  void setName(const QString &name);

public slots:
  QTextEdit *editor() { return te; };
  void modifiedNote();

  // QTextEdit methods
  QString text() { return te->toPlainText(); }
  void setText(const QString &s) { te->setText(s); }

  void print() override;
  void exportPDF(const QString &fileName) override;
  QString exportASCII(const QString &filename = QString::null);

private:
  QTextEdit *te;
};

#endif
