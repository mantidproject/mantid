#ifndef MANTIDQTMANTIDWIDGETS_MWRUNFILES_H_
#define MANTIDQTMANTIDWIDGETS_MWRUNFILES_H_

#include "MantidQtMantidWidgets/ui_MWRunFiles.h"
#include "MantidQtMantidWidgets/MantidWidget.h"
#include "WidgetDllOption.h"
#include <QString>
#include <QSettings>
#include <QComboBox>
#include <QMessageBox>

namespace MantidQt
{
  namespace MantidWidgets
  {
    /** 
    This class defines a widget for file searching. It allows either single or multiple files
    to be specified.

    @author Martyn Gigg, Tessella Support Services plc
    @date 24/02/2009

    Copyright &copy; 2009 STFC Rutherford Appleton Laboratories

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>    
    */

    class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS MWRunFiles : public MantidWidget
    {
      Q_OBJECT
      Q_PROPERTY(QString label READ getLabelText WRITE setLabelText)
      Q_PROPERTY(bool multipleFiles READ allowMultipleFiles WRITE allowMultipleFiles)
      Q_PROPERTY(bool optional READ isOptional WRITE isOptional)

    public:
      MWRunFiles(QWidget *parent=NULL);

      // property accessors/modifiers
      QString getLabelText() const;
      void setLabelText(const QString & text);
      bool allowMultipleFiles() const;
      void allowMultipleFiles(const bool);
      bool isOptional() const;
      void isOptional(const bool);


      // Standard setters/getters
      void setExtensionList(const QStringList & exts);
      bool isValid() const;
      const std::vector<std::string>& getFileNames() const;
      virtual QString getFile1() const;

    signals:
      void fileChanged();

    public slots:
      virtual void instrumentChange(const QString &newInstr);

    private:
      bool m_allowMultipleFiles;
      bool m_isOptional;

    protected:
      Ui::MWRunFiles m_uiForm;
      ///constains the name of the instrument that the runs files are for
      QString m_instrPrefix;
      /// the first directory listed in the users save path, or empty if none are defined in the Mantid*properties files
      QString m_defDir;
      /// An array of valid file names derived from the entries in the leNumber LineEdit
      std::vector<std::string> m_files;
      QString m_lastDir;
      QString m_fileFilter;


      virtual QString openFileDia();
      void readRunNumAndRanges();
      void readCommasAndHyphens(const std::string &in, std::vector<std::string> &out);

      protected slots:
        virtual void browseClicked();
        virtual void readEntries();
    };

    class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS MWRunFile : public MWRunFiles
    {
      Q_OBJECT

    public:
      MWRunFile(QWidget *parent=NULL);

      /** Returns the user entered filename, throws if the file is not found or mulitiple
      *  files were entered
      *  @return a filename validated against a FileProperty
      *  @throw invalid_argument if the file couldn't be found or multiple files were entered
      */
      QString getFileName() const {return getFile1();}
    signals:
      void fileChanged();
      public slots:
        void suggestFilename(const QString &newName);
    protected:
      /// it is possible to set and change the default value for this widget, this stores the last default value given to it
      QString m_suggestedName;
      /// stores if the widget has been changed by the user away from its default value
      bool m_userChange;
      virtual QString openFileDia();

      private slots:
        void browseClicked();
        void instrumentChange(const QString &);
        void readEntries();
    };
  }
}

#endif // MANTIDQTMANTIDWIDGETS_MWRUNFILES_H_