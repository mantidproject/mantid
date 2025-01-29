// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/FindFilesThreadPoolManager.h"
#include "MantidQtWidgets/Common/MantidWidget.h"
#include "ui_FileFinderWidget.h"
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <memory>

namespace Mantid {
namespace API {
class IAlgorithm;
}
} // namespace Mantid

namespace MantidQt {
// namespace MantidWidgets {
// class DataSelector;
// }
namespace API {

/**
This class defines a widget for file searching. It allows either single or
multiple files
to be specified.

@author Martyn Gigg, Tessella Support Services plc
@date 24/02/2009
*/

class EXPORT_OPT_MANTIDQT_COMMON FileFinderWidget : public API::MantidWidget {
  Q_OBJECT

  Q_PROPERTY(bool findRunFiles READ isForRunFiles WRITE isForRunFiles)
  Q_PROPERTY(bool findDirectory READ isForDirectory WRITE isForDirectory)
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

public:
  /// options for bringing up the load file dialog
  enum ButtonOpts {
    Text, ///< use a button (normally labelled "Browse")
    Icon, ///< use an icon
    None  ///< disable the load file dialog
  };
  /// Flags for workspace entries
  enum {
    NO_ENTRY_NUM = -1, ///< error in the entry number setting
    ALL_ENTRIES = -2   ///< use all entries (i.e. entry number was left blank)
  };
  /// Options for the live button
  enum LiveButtonOpts {
    Hide, ///< Don't use the live button
    Show, ///< Display the live button
  };

  /// Default constructor
  FileFinderWidget(QWidget *parent = nullptr);

  // property accessors/modifiers
  bool isForRunFiles() const;
  void isForRunFiles(bool /*mode*/);
  bool isForDirectory() const;
  void isForDirectory(bool /*mode*/);
  QString getLabelText() const;
  void setLabelText(const QString &text);
  void setLabelMinWidth(int /*width*/);
  bool allowMultipleFiles() const;
  void allowMultipleFiles(bool /*allow*/);
  bool isOptional() const;
  void isOptional(bool /*optional*/);
  ButtonOpts doButtonOpt() const;
  void doButtonOpt(ButtonOpts buttonOpt);
  bool doMultiEntry() const;
  void doMultiEntry(bool /*multiEntry*/);
  const QString &getAlgorithmProperty() const;
  void setAlgorithmProperty(const QString &name);
  const QStringList &getFileExtensions() const;
  std::vector<std::string> getStringFileExtensions() const;
  void setFileExtensions(const QStringList &extensions);
  bool extsAsSingleOption() const;
  void extsAsSingleOption(bool value);
  LiveButtonOpts liveButtonState() const;
  void liveButtonState(LiveButtonOpts /*option*/);

  // Standard setters/getters
  void liveButtonSetChecked(bool /*checked*/);
  bool liveButtonIsChecked() const;
  bool isEmpty() const;
  QString getText() const;

  bool isValid() const;
  bool isSearching() const;
  const QStringList &getFilenames() const;
  QString getFirstFilename() const;
  int getEntryNum() const;
  void setEntryNum(const int num);
  /// Overridden from base class to retrieve user input through a common
  /// interface
  QVariant getUserInput() const override;
  /// Sets a value on the widget through a common interface
  void setUserInput(const QVariant &value) override;
  /// Sets a value on the widget but doesn't emit a signal to say it has changed
  void setText(const QString &value);
  /// flag a problem with the file the user entered, an empty string means no
  /// error
  void setFileProblem(const QString &message);
  /// Get file problem, empty string means no error.
  const QString &getFileProblem() const;
  /// Read settings from the given group
  void readSettings(const QString &group);
  /// Save settings in the given group
  void saveSettings(const QString &group);
  /// Alters the text label that contains the number of entries, normally run
  /// when the file is loaded
  void setNumberOfEntries(int number);
  /// Inform the widget of a running instance of MonitorLiveData to be used in
  /// stopLiveListener()
  void setLiveAlgorithm(const std::shared_ptr<Mantid::API::IAlgorithm> &monitorLiveData);
  /// Gets the instrument currently fixed to
  const QString &getInstrumentOverride() const;
  /// Overrides the value of default instrument
  void setInstrumentOverride(const QString &instName);
  /// Set the input read-only or not
  void setReadOnly(bool readOnly);
  /// Get the last directory
  const QString getLastDirectory() const { return m_lastDir; }
  /// Set the last directory
  void setLastDirectory(const QString &lastDir) { m_lastDir = lastDir; }
  /// Set an arbitrary validator on the line edit
  void setTextValidator(const QValidator *validator);
  void setUseNativeWidget(bool /*native*/);
  void setProxyModel(QAbstractProxyModel *proxyModel);

signals:
  /// Emitted when the file text changes
  void fileTextChanged(const QString & /*_t1*/);
  /// Emitted when the editing has finished
  void fileEditingFinished();
  /// Emitted when files finding starts.
  void findingFiles();
  /// Emitted when files have been found
  void filesFound();
  /// Emitted when files have been found that are different to what was found
  /// last time
  void filesFoundChanged();
  /// Emitted when file finding is finished (files may or may not have been
  /// found).
  void fileFindingFinished();
  /// Emitted when the live button is toggled
  void liveButtonPressed(bool /*_t1*/);
  /// Emitted when inspection of any found files is completed
  void fileInspectionFinished();

public slots:
  /// Set the file text and try and find it
  void setFileTextWithSearch(const QString &text);
  /// Just update the file text, useful for syncing two boxes
  void setFileTextWithoutSearch(const QString &text);
  /// Clear the search from the widget
  void clear();
  /// Find the files if the text edit field is modified
  void findFiles();
  /// Find the files within the text edit field and cache their full paths
  void findFiles(bool isModified);
  std::shared_ptr<const Mantid::API::IAlgorithm> stopLiveAlgorithm();

public:
  // Method for handling drop events
  void dropEvent(QDropEvent * /*unused*/) override;
  // called when a drag event enters the class
  void dragEnterEvent(QDragEnterEvent * /*unused*/) override;

private:
  /// Create a file filter from a list of extensions
  QString createFileFilter();
  /// Create an extension list from the name algorithm and property
  QStringList getFileExtensionsFromAlgorithm(const QString &algName, const QString &propName);
  /// Create an extension list from the name algorithm and property
  QStringList getFilesFromAlgorithm(const QString &algName, const QString &propName, const QString &filename);
  /// Open a file dialog
  QString openFileDialog();
  /// flag a problem with the supplied entry number, an empty string means no
  /// error
  void setEntryNumProblem(const QString &message);
  /// displays the validator red star if either m_fileProblem or
  /// m_entryNumProblem are not empty
  void refreshValidator();
  /// Turn on/off display of validator red star (default is on)
  void setValidatorDisplay(bool display);
  /// gets text to use for find files search parameters
  const QString findFilesGetSearchText(QString &searchText);
  /// handles findFiles background thread
  void runFindFiles(const QString &searchText);
  /// Helper method to create a FindFilesSearchParameters object
  FindFilesSearchParameters createFindFilesSearchParameters(const std::string &text) const;

private slots:
  /// Browse clicked slot
  void browseClicked();
  /// currently checks only if the entry number is any integer > 0
  void checkEntry();
  /// Slot called when file finding thread has finished.
  void inspectThreadResult(const FindFilesSearchResults &results = FindFilesSearchResults());

private:
  /// Is the widget for run files or standard files
  bool m_findRunFiles;
  /// If the widget is for directories
  bool m_isForDirectory;
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
  std::shared_ptr<Mantid::API::IAlgorithm> m_monitorLiveData;
  /// Whether validation red star is being shown
  bool m_showValidator;
  /// The Ui form
  Ui::FileFinderWidget m_uiForm;
  /// An array of valid file names derived from the entries in the leNumber
  /// LineEdit
  QStringList m_foundFiles;
  /// An array of the last valid file names found
  QStringList m_lastFoundFiles;
  /// The last directory viewed by the browse dialog
  QString m_lastDir;
  /// A file filter for the file browser
  QString m_fileFilter;
  /// Cache the default instrument name
  QString m_defaultInstrumentName;
  /// Expanded user input
  QString m_valueForProperty;
  /// Handle to a find files thread pool manager
  FindFilesThreadPoolManager m_pool;
  /// Handle to any results found
  FindFilesSearchResults m_cachedResults;
  /// non-native QFileDialog
  QFileDialog m_dialog;
  /// flag to control use of m_dialog
  bool m_useNativeDialog;
};

} // namespace API
} // namespace MantidQt

Q_DECLARE_METATYPE(MantidQt::API::FileFinderWidget::ButtonOpts)
Q_DECLARE_METATYPE(MantidQt::API::FileFinderWidget::LiveButtonOpts)
