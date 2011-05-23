#ifndef MANTIDQTMANTIDWIDGETS_MWRUNFILES_H_
#define MANTIDQTMANTIDWIDGETS_MWRUNFILES_H_

#include "ui_MWRunFiles.h"
#include "MantidQtAPI/MantidWidget.h"
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

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS MWRunFiles : public API::MantidWidget
    {
      Q_OBJECT
      
      Q_PROPERTY(bool findRunFiles READ isForRunFiles WRITE isForRunFiles)
      Q_PROPERTY(QString label READ getLabelText WRITE setLabelText)
      Q_PROPERTY(bool multipleFiles READ allowMultipleFiles WRITE allowMultipleFiles)
      Q_PROPERTY(bool optional READ isOptional WRITE isOptional)
      Q_PROPERTY(bool multiEntry READ doMultiEntry WRITE doMultiEntry)
      Q_PROPERTY(ButtonOpts buttonOpt READ doButtonOpt WRITE doButtonOpt)
      Q_PROPERTY(QString algorithmAndProperty READ getAlgorithmProperty WRITE setAlgorithmProperty)
      Q_PROPERTY(QStringList fileExtensions READ getFileExtensions WRITE setFileExtensions)
      Q_PROPERTY(bool extsAsSingleOption READ extsAsSingleOption WRITE extsAsSingleOption)
      Q_ENUMS(ButtonOpts)

    public:
      /// options for bringing up the load file dialog
      enum ButtonOpts
      {
        Text,                       ///< use a button (normally labelled "Browse")
        Icon,                       ///< use an icon
        None                        ///< disable the load file dialog
      };
      /// Flags for workspace entries
      enum
      {
        NO_ENTRY_NUM = -1,          ///< error in the entry number setting
        ALL_ENTRIES = -2            ///< use all entries (i.e. entry number was left blank)
      };

      ///Default constructor
      MWRunFiles(QWidget *parent=NULL);
      // property accessors/modifiers
      bool isForRunFiles() const;
      void isForRunFiles(const bool);
      QString getLabelText() const;
      void setLabelText(const QString & text);
      bool allowMultipleFiles() const;
      void allowMultipleFiles(const bool);
      bool isOptional() const;
      void isOptional(const bool);
      ButtonOpts doButtonOpt() const;
      void doButtonOpt(const ButtonOpts buttonOpt);
      bool doMultiEntry() const;
      void doMultiEntry(const bool);
      QString getAlgorithmProperty() const;
      void setAlgorithmProperty(const QString & name);
      QStringList getFileExtensions() const;
      void setFileExtensions(const QStringList & extensions);
      bool extsAsSingleOption() const;
      void extsAsSingleOption(const bool value);
      bool isEmpty() const;
      QString getText() const;

      // Standard setters/getters
      bool isValid() const;
      QStringList getFilenames() const;
      QString getFirstFilename() const;
      int getEntryNum() const;
      void setEntryNum(const int num);
      /// Overridden from base class to retrieve user input through a common interface
      QVariant getUserInput() const;      
      /// Sets a value on the widget through a common interface
      void setUserInput(const QVariant & value);
      /// flag a problem with the file the user entered, an empty string means no error
      void setFileProblem(const QString & message);
      /// Read settings from the given group
      void readSettings(const QString & group);
      /// Save settings in the given group
      void saveSettings(const QString & group);
      /// Alters the text label that contains the number of entries, normally run when the file is loaded
      void setNumberOfEntries(const int number);

    signals:
      /// Emitted when the file text changes
      void fileTextChanged(const QString &);
      /// Emitted when the editing has finished
      void fileEditingFinished();
      /// Emitted when a files have been found
      void filesFound();

    public slots:
      /// Set the file text
      void setFileText(const QString & text);
      /// Find the files within the text edit field and cache their full paths
      void findFiles();

    private:
      /// Create a file filter from a list of extensions
      QString createFileFilter();
      /// Create an extension list from the name algorithm and property
      QStringList getFileExtensionsFromAlgorithm(const QString & algName, const QString &propName);
      /// Open a file dialog
      QString openFileDialog();
      /// flag a problem with the supplied entry number, an empty string means no error
      void setEntryNumProblem(const QString & message);
      /// displays the validator red star if either m_fileProblem or m_entryNumProblem are not empty
      void refreshValidator();

    private slots:
      /// Browse clicked slot
      void browseClicked();
      /// currently checks only if the entry number is any integer > 0
      void checkEntry();

    private:
      /// Is the widget for run files or standard files
      bool m_findRunFiles;
      /// Allow multiple files
      bool m_allowMultipleFiles;
      /// Whether the widget can be empty
      bool m_isOptional;
      /// Whether to allow the user to state an entry number
      bool m_multiEntry;
      /// To use a browse button or icon or nothing at all
      ButtonOpts m_buttonOpt;
      /// Holds any error with the user entry for the filename, "" means no error
      QString m_fileProblem;
      /// If applicable holds any error with the user in entryNum, "" means no error
      QString m_entryNumProblem;
      /// The algorithm name and property (can be empty)
      QString m_algorithmProperty;
      /// The file extensions to look for
      QStringList m_fileExtensions;
      /// If true the exts are displayed as one option in the dialog
      bool m_extsAsSingleOption;

      /// The Ui form
      Ui::MWRunFiles m_uiForm;
      /// An array of valid file names derived from the entries in the leNumber LineEdit
      QStringList m_foundFiles;
      /// The last directory viewed by the browse dialog
      QString m_lastDir;
      /// A file filter for the file browser
      QString m_fileFilter;
    };
  }
}

#endif // MANTIDQTMANTIDWIDGETS_MWRUNFILES_H_
