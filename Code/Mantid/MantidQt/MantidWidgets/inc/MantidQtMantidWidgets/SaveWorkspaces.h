#ifndef MANTIDQTMANTIDWIDGETS_SAVEWORKSPACES_H_
#define MANTIDQTMANTIDWIDGETS_SAVEWORKSPACES_H_

#include "MantidQtAPI/MantidDialog.h"
#include "WidgetDllOption.h"
#include <QHash>
#include <QSignalMapper>
#include <QGridLayout>
#include <QListWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QString>

#include "MantidAPI/FrameworkManager.h"

namespace MantidQt
{
  namespace MantidWidgets
  {
	/** 
    Implements a dialog box that allows users to save multiple Mantid workspaces

    @author Steve Williams

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>    
    */
    class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS SaveWorkspaces : public API::MantidDialog
    {
      Q_OBJECT

    public: 
      SaveWorkspaces(QWidget *parent, const QString & suggFname,
        QHash<const QCheckBox * const, QString> & defSavs);
      void initLayout();
      ///Returns the save extension expected the name algorithm
      static QString getSaveAlgExt(const QString & algName);

    signals:
      void closing();

    private:
      QLineEdit *m_fNameEdit;
      QListWidget *m_workspaces;
      QCheckBox *m_append;
      QString m_lastName;

      QHash<QCheckBox * const, QString> m_savFormats;
      typedef QHash<QCheckBox * const, QString>::const_iterator SavFormatsConstIt;
      
      void setupLine1(QHBoxLayout * const lineOne);
      void setupLine2(QHBoxLayout * const lineTwo, const QHash<const QCheckBox * const, QString> & defSavs);
      void readSettings();
      void setFileName(const QString & newName);
      void setupFormatTicks(const QHash<const QCheckBox * const, QString> & defSavs);
      void saveSettings() const;

      void addButtonsDisab(int row);
      void closeEvent(QCloseEvent *event);
      QString saveList(const QList<QListWidgetItem*> & list, const QString & algorithm, QString fileBase, bool toAppend);

    private slots:
      void saveSel();
      void setFileName(int row);
      void saveFileBrowse();

    };
  }
}

#endif //MANTIDQTMANTIDWIDGETS_SAVEWORKSPACES_H_
