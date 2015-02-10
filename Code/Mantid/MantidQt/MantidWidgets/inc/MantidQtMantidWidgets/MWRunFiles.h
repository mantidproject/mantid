#ifndef MANTIDQTMANTIDWIDGETS_MWRUNFILES_H_
#define MANTIDQTMANTIDWIDGETS_MWRUNFILES_H_

#include "ui_MWRunFiles.h"
#include "MantidQtAPI/MantidWidget.h"
#include "WidgetDllOption.h"
#include <QString>
#include <QSettings>
#include <QComboBox>
#include <QMessageBox>
#include <QStringList>
#include <QThread>
#include <boost/shared_ptr.hpp>

namespace Mantid { namespace API { class IAlgorithm; } }

namespace MantidQt
{
  namespace MantidWidgets
  {
    /**
     * A class to allow the asyncronous finding of files.
     */
    class FindFilesThread : public QThread
    {
      Q_OBJECT

    public:
      /// Constructor.
      FindFilesThread(QObject *parent = NULL);
      /// Set the various file-finding values / options.
      void set(QString text, bool isForRunFiles, bool isOptional, const QString & defaultInstrumentName = "", const QString & algorithmProperty = "");

      /// Returns the error string.  Empty if no error was caught.
      std::string error() const { return m_error; }
      /// Returns the vector of "unpacked" filenames.  Empty if no files were found, or if there was an error.
      std::vector<std::string> filenames() const { return m_filenames; }
      /// Returns a string value that can be used to put in to another instance of the algorithm to avoid searching again
      QString valueForProperty() const { return m_valueForProperty; }

    protected:
      /// Override parent class run().
      virtual void run();

    private:
      /// Use the specified algorithm and property to find files instead of using the FileFinder.
      void getFilesFromAlgorithm();

      /// Storage for any error thrown while trying to find files.
      std::string m_error;
      /// Filenames found during execution of the thread.
      std::vector<std::string> m_filenames;
      /// Stores the string value to be used as input for an algorithm property
      QString m_valueForProperty;

      /// File name text typed in by the user.
      std::string m_text;

      QString m_algorithm;
      QString m_property;
      bool m_isForRunFiles;
      bool m_isOptional;
      QString m_defaultInstrumentName;
    };

    /** 
    This class defines a widget for file searching. It allows either single or multiple files
    to be specified.

    @author Martyn Gigg, Tessella Support Services plc
    @date 24/02/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
      Q_PROPERTY(LiveButtonOpts liveButton READ liveButtonState WRITE liveButtonState)
      Q_PROPERTY(QString instrumentOverride READ getInstrumentOverride WRITE setInstrumentOverride)
      Q_ENUMS(ButtonOpts)
      Q_ENUMS(LiveButtonOpts)

      friend class DataSelector;

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
      /// Options for the live button
      enum LiveButtonOpts
      {
        Hide,            ///< Don't use the live button
        AlwaysShow,      ///< Show whether a connection is possible or not (will be disabled)
        ShowIfCanConnect ///< Only show if able to connect to the live data server
      };

      ///Default constructor
      MWRunFiles(QWidget *parent=NULL);

      /// Destructor
      ~MWRunFiles();

      // property accessors/modifiers
      bool isForRunFiles() const;
      void isForRunFiles(bool);
      QString getLabelText() const;
      void setLabelText(const QString & text);
      void setLabelMinWidth(int);
      bool allowMultipleFiles() const;
      void allowMultipleFiles(bool);
      bool isOptional() const;
      void isOptional(bool);
      ButtonOpts doButtonOpt() const;
      void doButtonOpt(ButtonOpts buttonOpt);
      bool doMultiEntry() const;
      void doMultiEntry(bool);
      QString getAlgorithmProperty() const;
      void setAlgorithmProperty(const QString & name);
      QStringList getFileExtensions() const;
      void setFileExtensions(const QStringList & extensions);
      bool extsAsSingleOption() const;
      void extsAsSingleOption(bool value);
      LiveButtonOpts liveButtonState() const;
      void liveButtonState(LiveButtonOpts);

      // Standard setters/getters
      void liveButtonSetEnabled(bool);
      void liveButtonSetChecked(bool);
      bool liveButtonIsChecked() const;
      bool isEmpty() const;
      QString getText() const;

      bool isValid() const;
      bool isSearching() const;
      QStringList getFilenames() const;
      QString getFirstFilename() const;
      int getEntryNum() const;
      void setEntryNum(const int num);
      /// Overridden from base class to retrieve user input through a common interface
      QVariant getUserInput() const;
      /// Sets a value on the widget through a common interface
      void setUserInput(const QVariant & value);
      /// Sets a value on the widget but doesn't emit a signal to say it has changed
      void setText(const QString & value);
      /// flag a problem with the file the user entered, an empty string means no error
      void setFileProblem(const QString & message);
      /// Get file problem, empty string means no error.
      QString getFileProblem();
      /// Read settings from the given group
      void readSettings(const QString & group);
      /// Save settings in the given group
      void saveSettings(const QString & group);
      /// Alters the text label that contains the number of entries, normally run when the file is loaded
      void setNumberOfEntries(int number);
      /// Inform the widget of a running instance of MonitorLiveData to be used in stopLiveListener()
      void setLiveAlgorithm(const boost::shared_ptr<Mantid::API::IAlgorithm>& monitorLiveData);
      /// Gets the instrument currently fixed to
      QString getInstrumentOverride();
      /// Overrides the value of default instrument
      void setInstrumentOverride(const QString & instName);

    signals:
      /// Emitted when the file text changes
      void fileTextChanged(const QString &);
      /// Emitted when the editing has finished
      void fileEditingFinished();
      // Emitted when files finding starts.
      void findingFiles();
      /// Emitted when files have been found
      void filesFound();
      /// Emitted when file finding is finished (files may or may not have been found).
      void fileFindingFinished();
      /// Emitted when the live button is toggled
      void liveButtonPressed(bool);
      /// Signal emitted after asynchronous checking of live stream availability
      void liveButtonSetEnabledSignal(bool);

    public slots:
      /// Set the file text and try and find it
      void setFileTextWithSearch(const QString & text);
      /// Just update the file text, useful for syncing two boxes
      void setFileTextWithoutSearch(const QString & text);
      /// Find the files within the text edit field and cache their full paths
      void findFiles();
      boost::shared_ptr<const Mantid::API::IAlgorithm> stopLiveAlgorithm();
      
    protected:
      //Method for handling drop events
      void dropEvent(QDropEvent *);
      //called when a drag event enters the class
      void dragEnterEvent(QDragEnterEvent *);

    private:
      /// Create a file filter from a list of extensions
      QString createFileFilter();
      /// Create an extension list from the name algorithm and property
      QStringList getFileExtensionsFromAlgorithm(const QString & algName, const QString &propName);
      /// Create an extension list from the name algorithm and property
      QStringList getFilesFromAlgorithm(const QString & algName, const QString &propName, const QString &filename);
      /// Open a file dialog
      QString openFileDialog();
      /// flag a problem with the supplied entry number, an empty string means no error
      void setEntryNumProblem(const QString & message);
      /// displays the validator red star if either m_fileProblem or m_entryNumProblem are not empty
      void refreshValidator();
      /// Called asynchronously to check the availability of the live stream
      void checkLiveConnection();

    private slots:
      /// Browse clicked slot
      void browseClicked();
      /// currently checks only if the entry number is any integer > 0
      void checkEntry();
      /// Slot called when file finding thread has finished.
      void inspectThreadResult();

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
      /// If or when live button will be shown
      LiveButtonOpts m_liveButtonState;
      /// Handle on a running instance of MonitorLiveData
      boost::shared_ptr<Mantid::API::IAlgorithm> m_monitorLiveData;

      /// The Ui form
      Ui::MWRunFiles m_uiForm;
      /// An array of valid file names derived from the entries in the leNumber LineEdit
      QStringList m_foundFiles;
      /// The last directory viewed by the browse dialog
      QString m_lastDir;
      /// A file filter for the file browser
      QString m_fileFilter;
      /// Thread to allow asynchronous finding of files.
      FindFilesThread * m_thread;

      QString m_defaultInstrumentName;
    };
  }
}

#endif // MANTIDQTMANTIDWIDGETS_MWRUNFILES_H_
