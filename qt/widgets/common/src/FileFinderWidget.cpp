// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FileFinderWidget.h"
#include "MantidQtWidgets/Common/DropEventHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/VectorHelper.h"

#include <Poco/File.h>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QHash>
#include <QMimeData>
#include <QStringList>
#include <QUrl>
#include <QtConcurrentRun>

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace MantidQt::API;
namespace DropEventHelper = MantidQt::MantidWidgets::DropEventHelper;

////////////////////////////////////////////////////////////////////
// FileFinderWidget
////////////////////////////////////////////////////////////////////

FileFinderWidget::FileFinderWidget(QWidget *parent)
    : MantidWidget(parent), m_findRunFiles(true), m_isForDirectory(false), m_allowMultipleFiles(false),
      m_isOptional(false), m_multiEntry(false), m_buttonOpt(Text), m_fileProblem(""), m_entryNumProblem(""),
      m_algorithmProperty(""), m_fileExtensions(), m_extsAsSingleOption(true), m_liveButtonState(Hide),
      m_showValidator(true), m_foundFiles(), m_lastFoundFiles(), m_lastDir(), m_fileFilter(), m_pool(), m_dialog(),
      m_useNativeDialog(true) {

  m_uiForm.setupUi(this);

  connect(m_uiForm.fileEditor, SIGNAL(textChanged(const QString &)), this, SIGNAL(fileTextChanged(const QString &)));
  connect(m_uiForm.fileEditor, SIGNAL(editingFinished()), this, SIGNAL(fileEditingFinished()));
  connect(m_uiForm.browseBtn, SIGNAL(clicked()), this, SLOT(browseClicked()));
  connect(m_uiForm.browseIco, SIGNAL(clicked()), this, SLOT(browseClicked()));

  connect(this, SIGNAL(fileEditingFinished()), this, SLOT(findFiles()));
  connect(m_uiForm.entryNum, SIGNAL(textChanged(const QString &)), this, SLOT(checkEntry()));
  connect(m_uiForm.entryNum, SIGNAL(editingFinished()), this, SLOT(checkEntry()));

  m_uiForm.fileEditor->clear();

  if (m_multiEntry) {
    m_uiForm.entryNum->show();
    m_uiForm.numEntries->show();
  } else {
    m_uiForm.entryNum->hide();
    m_uiForm.numEntries->hide();
  }

  doButtonOpt(m_buttonOpt);

  liveButtonState(m_liveButtonState);
  connect(m_uiForm.liveButton, SIGNAL(toggled(bool)), this, SIGNAL(liveButtonPressed(bool)));

  setFocusPolicy(Qt::StrongFocus);
  setFocusProxy(m_uiForm.fileEditor);

  // First try default save directory
  m_lastDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory"));

  // If that fails pick the first data search directory
  if (m_lastDir.isEmpty()) {
    QStringList dataDirs =
        QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("datasearch.directories"))
            .split(";", Qt::SkipEmptyParts);

    if (!dataDirs.isEmpty())
      m_lastDir = dataDirs[0];
  }

  // this for accepts drops, but the underlying text input does not.
  this->setAcceptDrops(true);
  m_uiForm.fileEditor->setAcceptDrops(false);
}

/**
 * Returns if this widget is for run file searching or not
 * @returns True if this widget searches for run files, false otherwise
 */
bool FileFinderWidget::isForRunFiles() const { return m_findRunFiles; }

/**
 * Sets whether this widget is for run file searching or not
 * @param mode :: True if this widget searches for run files, false otherwise
 */
void FileFinderWidget::isForRunFiles(const bool mode) { m_findRunFiles = mode; }

/**
 * Returns if this widget is for selecting a directory or not.
 * @return True if selecting a directory
 */
bool FileFinderWidget::isForDirectory() const { return m_isForDirectory; }

/**
 * Sets directory searching mode.
 * @param mode True to search for directories only
 */
void FileFinderWidget::isForDirectory(const bool mode) {
  clear();
  m_isForDirectory = mode;
}

/**
 * Return the label text on the widget
 * @returns The current value of the text on the label
 */
QString FileFinderWidget::getLabelText() const { return m_uiForm.textLabel->text(); }

/**
 * Set the text on the label
 * @param text :: A string giving the label to use for the text
 */
void FileFinderWidget::setLabelText(const QString &text) {
  m_uiForm.textLabel->setText(text);
  m_uiForm.textLabel->setVisible(!text.isEmpty());
}

/** Set the minimum width on the label widget
 *  @param width The new minimum width of the widget
 */
void FileFinderWidget::setLabelMinWidth(const int width) { m_uiForm.textLabel->setMinimumWidth(width); }

/**
 * Return whether this widget allows multiple files to be specified within the
 * edit box
 * @returns True if multiple files can be specified, false otherwise
 */
bool FileFinderWidget::allowMultipleFiles() const { return m_allowMultipleFiles; }

/**
 * Set whether this widget allows multiple files to be specifed or not
 * @param allow :: If true then the widget will accept multiple files else only
 * a single file may be specified
 */
void FileFinderWidget::allowMultipleFiles(const bool allow) {
  m_allowMultipleFiles = allow;
  findFiles();
}

/**
 * Return whether empty input is allowed
 */
bool FileFinderWidget::isOptional() const { return m_isOptional; }

/**
 * Sets if the text field is optional
 * @param optional :: Set the optional status of the text field
 */
void FileFinderWidget::isOptional(const bool optional) {
  m_isOptional = optional;
  findFiles();
}

/**
 * Returns the preference for how the dialog control should be
 * @return the setting
 */
FileFinderWidget::ButtonOpts FileFinderWidget::doButtonOpt() const { return m_buttonOpt; }

/**
 * Set how the browse should appear
 * @param buttonOpt the preference for the control, if there will be one, to
 * activate the dialog box
 */
void FileFinderWidget::doButtonOpt(const FileFinderWidget::ButtonOpts buttonOpt) {
  m_buttonOpt = buttonOpt;
  if (buttonOpt == None) {
    m_uiForm.browseBtn->hide();
    m_uiForm.browseIco->hide();
  } else if (buttonOpt == Text) {
    m_uiForm.browseBtn->show();
    m_uiForm.browseIco->hide();
  } else if (buttonOpt == Icon) {
    m_uiForm.browseBtn->hide();
    m_uiForm.browseIco->show();
  }
}

/**
 * Whether to find the number of entries in the file or assume (the
 * normal situation) of one entry
 * @return true if the widget is to look for multiple entries
 */
bool FileFinderWidget::doMultiEntry() const { return m_multiEntry; }

/**
 * Set to true to enable the period number box
 * @param multiEntry whether to show the multiperiod box
 */
void FileFinderWidget::doMultiEntry(const bool multiEntry) {
  m_multiEntry = multiEntry;
  if (m_multiEntry) {
    m_uiForm.entryNum->show();
    m_uiForm.numEntries->show();
  } else {
    m_uiForm.entryNum->hide();
    m_uiForm.numEntries->hide();
  }
  refreshValidator();
}

/**
 * Returns the algorithm name
 * @returns The algorithm name
 */
QString FileFinderWidget::getAlgorithmProperty() const { return m_algorithmProperty; }

/**
 * Sets an algorithm name that can be tied to this widget
 * @param text :: The name of the algorithm and property in the form
 * [AlgorithmName|PropertyName]
 */
void FileFinderWidget::setAlgorithmProperty(const QString &text) { m_algorithmProperty = text; }
/**
 * Returns the list of file extensions the widget will search for.
 * @return list of file extensions
 */
QStringList FileFinderWidget::getFileExtensions() const { return m_fileExtensions; }

std::vector<std::string> FileFinderWidget::getStringFileExtensions() const {
  std::vector<std::string> extensions;
  std::transform(m_fileExtensions.begin(), m_fileExtensions.end(), std::back_inserter(extensions),
                 [](const QString &extension) { return extension.toStdString(); });

  return extensions;
}

/**
 * Sets the list of file extensions the dialog will search for. Only taken
 * notice of if AlgorithmProperty not set.
 * @param extensions :: list of file extensions
 */
void FileFinderWidget::setFileExtensions(const QStringList &extensions) {
  m_fileExtensions = extensions;
  m_fileFilter.clear();
}

/**
 * Returns whether the file dialog should display the exts as a single list or
 * as multiple items
 * @return boolean
 */
bool FileFinderWidget::extsAsSingleOption() const { return m_extsAsSingleOption; }

/**
 * Sets whether the file dialog should display the exts as a single list or as
 * multiple items
 * @param value :: If true the file dialog wil contain a single entry will all
 * filters
 */
void FileFinderWidget::extsAsSingleOption(const bool value) { m_extsAsSingleOption = value; }

/// Returns whether the live button is being shown;
FileFinderWidget::LiveButtonOpts FileFinderWidget::liveButtonState() const { return m_liveButtonState; }

void FileFinderWidget::liveButtonState(const LiveButtonOpts option) {
  m_liveButtonState = option;
  if (m_liveButtonState == Hide) {
    m_uiForm.liveButton->hide();
  } else if (m_liveButtonState == Show) {
    m_uiForm.liveButton->show();
  }
}

void FileFinderWidget::liveButtonSetChecked(const bool checked) { m_uiForm.liveButton->setChecked(checked); }

bool FileFinderWidget::liveButtonIsChecked() const { return m_uiForm.liveButton->isChecked(); }

/**
 * Is the input within the widget valid?
 * @returns True of the file names within the widget are valid, false otherwise
 */
bool FileFinderWidget::isValid() const { return m_uiForm.valid->isHidden(); }

/**
 * Is the widget currently searching
 * @return True if a search is inprogress
 */
bool FileFinderWidget::isSearching() const { return m_pool.isSearchRunning(); }

/**
 * Returns the names of the files found
 * @return an array of filenames entered in the box
 */
QStringList FileFinderWidget::getFilenames() const { return m_foundFiles; }

/** Safer than using getRunFiles()[0] in the situation were there are no files
 *  @return an empty string is returned if no input files have been defined or
 * one of the files can't be found, otherwise it's the name of the first input
 * file
 *  @throw invalid_argument if one of the files couldn't be found it then no
 * filenames are returned
 */
QString FileFinderWidget::getFirstFilename() const {
  if (m_foundFiles.isEmpty())
    return "";
  else
    return m_foundFiles[0];
}

/** Check if any text, valid or not, has been entered into the line edit
 *  @return true if no text has been entered
 */
bool FileFinderWidget::isEmpty() const { return m_uiForm.fileEditor->text().isEmpty(); }

/** The verbatum, unexpanded text, that was entered into the box
 *  @return the contents shown in the Line Edit
 */
QString FileFinderWidget::getText() const { return m_uiForm.fileEditor->text(); }

/** The number the user entered into the entryNum lineEdit
 * or NO_ENTRY_NUM on error. Checking if isValid is true should
 * eliminate the possiblity of getting NO_ENTRY_NUM
 */
int FileFinderWidget::getEntryNum() const {
  if (m_uiForm.entryNum->text().isEmpty() || (!m_multiEntry)) {
    return ALL_ENTRIES;
  }
  if (isValid()) {
    bool isANumber;
    const int period = m_uiForm.entryNum->text().toInt(&isANumber);
    if (isANumber) {
      return period;
    }
  }
  return NO_ENTRY_NUM;
}

/** Set the entry displayed in the box to this value
 * @param num the period number to use
 */
void FileFinderWidget::setEntryNum(const int num) { m_uiForm.entryNum->setText(QString::number(num)); }

/**
 * Retrieve user input from this widget. This expands the current
 * file text to include the found absolute paths so that no more
 * searching is required
 * NOTE: This knows nothing of periods yet
 * @returns A QVariant containing the text string for the algorithm property
 */
QVariant FileFinderWidget::getUserInput() const { return QVariant(m_valueForProperty); }

/**
 * "Silently" sets the value of the widget.  It does NOT emit a signal to say it
 * has changed, and as far as the file finding routine is concerned it has not
 * been modified and so it will NOT go searching for files.
 *
 * @param value A QString containing text to be entered into the widget
 */

void FileFinderWidget::setText(const QString &value) { m_uiForm.fileEditor->setText(value); }

/**
 * Sets a value on the widget through a common interface. The
 * QVariant is assumed to be text and to contain a file string. Note that this
 * is primarily here for use within the AlgorithmDialog.
 *
 * Emits fileEditingFinished(), and changes to state of the text box widget
 * to be "modified", so that the file finder will try and find the file(s).
 *
 * @param value A QVariant containing user text
 */
void FileFinderWidget::setUserInput(const QVariant &value) {
  m_uiForm.fileEditor->setText(value.toString());
  m_uiForm.fileEditor->setModified(true);
  emit fileEditingFinished(); // Which is connected to slot findFiles()
}

/**
 * Flag a problem with the file the user entered, an empty string means no error
 * but
 * there may be an error with the entry box if enabled. Errors passed here are
 * shown first
 * @param message :: A message to include or "" for no error
 */
void FileFinderWidget::setFileProblem(const QString &message) {
  m_fileProblem = message;
  refreshValidator();
}

/**
 * Return the error.
 * @returns A string explaining the error.
 */
QString FileFinderWidget::getFileProblem() const { return m_fileProblem; }

/**
 * Save settings to the given group
 * @param group :: The name of the group key to save to
 */
void FileFinderWidget::saveSettings(const QString &group) {
  QSettings settings;
  settings.beginGroup(group);

  settings.setValue("last_directory", m_lastDir);

  settings.endGroup();
}

/** Writes the total number of periods in a file to the NumEntries
 *  Qlabel
 *  @param number the number to write, if this is < 1 a ? will be displayed in
 * it's place
 */
void FileFinderWidget::setNumberOfEntries(const int number) {
  const QString total = number > 0 ? QString::number(number) : "?";
  { m_uiForm.numEntries->setText("/" + total); }
}

/** Inform the widget of a running instance of MonitorLiveData to be used in
 * stopLiveListener().
 *  Note that the type passed in is IAlgorithm and that no check is made that it
 * actually refers
 *  to an instance of MonitorLiveData.
 *  @param monitorLiveData The running algorithm
 */
void FileFinderWidget::setLiveAlgorithm(const IAlgorithm_sptr &monitorLiveData) { m_monitorLiveData = monitorLiveData; }

/**
 * Gets the instrument currently set by the override property.
 *
 * If no override is set then the instrument set by default instrument
 *configurtion
 * option will be used and this function returns an empty string.
 *
 * @return Name of instrument, empty if not set
 */
QString FileFinderWidget::getInstrumentOverride() { return m_defaultInstrumentName; }

/**
 * Sets an instrument to fix the widget to.
 *
 * If an instrument name is geven then the widget will only look for files for
 *that
 * instrument, providing na empty string will remove this restriction and will
 *search
 * using the default instrument.
 *
 * @param instName Name of instrument, empty to disable override
 */
void FileFinderWidget::setInstrumentOverride(const QString &instName) {
  m_defaultInstrumentName = instName;
  findFiles(true);
}

/**
 * Set the file text.  This is different to setText in that it emits findFiles,
 *as well
 * changing the state of the text box widget to "modified = true" which is a
 *prerequisite
 * of findFiles.
 *
 * @param text :: The text string to set
 */
void FileFinderWidget::setFileTextWithSearch(const QString &text) {
  setFileTextWithoutSearch(text);
  findFiles();
}
/**
 * Set the file text but do not search
 * @param text :: The text string to set
 */
void FileFinderWidget::setFileTextWithoutSearch(const QString &text) {
  m_uiForm.fileEditor->setText(text);
  m_uiForm.fileEditor->setModified(true);
}

/**
 * Clears the search string and found files from the widget.
 */
void FileFinderWidget::clear() {
  m_foundFiles.clear();
  m_uiForm.fileEditor->setText("");
}

/**
 * Finds the files if the user has changed the parameter text.
 */
void FileFinderWidget::findFiles() { findFiles(m_uiForm.fileEditor->isModified()); }

/**
 * Finds the files specified by the user in a background thread.
 */
void FileFinderWidget::findFiles(bool isModified) {
  auto searchText = m_uiForm.fileEditor->text();
  if (m_isForDirectory) {
    m_foundFiles.clear();
    if (searchText.isEmpty()) {
      if (!m_isOptional)
        setFileProblem("A directory must be provided");
      else
        setFileProblem("");
    } else {
      setFileProblem("");
      m_foundFiles.append(searchText);
    }
    return;
  }

  if (isModified) {
    // Reset modified flag.
    m_uiForm.fileEditor->setModified(false);
    searchText = findFilesGetSearchText(searchText);
    runFindFiles(searchText);
  } else {
    // Make sure errors are correctly set if we didn't run
    inspectThreadResult(m_cachedResults);
  }
}

/**
 * Gets the search text to find files with.
 * @param searchText :: text entered by user
 * @return search text to create search params with
 */
const QString FileFinderWidget::findFilesGetSearchText(QString &searchText) {
  // If we have an override instrument then add it in appropriate places to
  // the search text
  if (!m_defaultInstrumentName.isEmpty()) {
    // Regex to match a selection of run numbers as defined here:
    // mantidproject.org/MultiFileLoading
    // Also allowing spaces between delimiters as this seems to work fine
    const std::string runNumberString = "([0-9]+)([:+-] ?[0-9]+)? ?(:[0-9]+)?";
    boost::regex runNumberExp(runNumberString, boost::regex::extended);
    // Regex to match a list of run numbers delimited by commas
    const std::string runListString = "(" + runNumberString + ")(, ?(" + runNumberString + "))*";
    boost::regex runNumberListExp(runListString, boost::regex::extended);

    // See if we can just prepend the instrument and be done
    if (boost::regex_match(searchText.toStdString(), runNumberExp)) {
      searchText = m_defaultInstrumentName + searchText;
    }
    // If it is a list we need to prepend the instrument to all run numbers
    else if (boost::regex_match(searchText.toStdString(), runNumberListExp)) {
      QStringList runNumbers = searchText.split(",", Qt::SkipEmptyParts);
      QStringList newRunNumbers;

      for (auto &runNumber : runNumbers)
        newRunNumbers << m_defaultInstrumentName + runNumber.simplified();

      searchText = newRunNumbers.join(",");
    }
  }
  return searchText;
}

/**
 * creates background thread for find files
 * @param searchText :: text to create search parameters from
 */
void FileFinderWidget::runFindFiles(const QString &searchText) {
  emit findingFiles();

  const auto parameters = createFindFilesSearchParameters(searchText.toStdString());
  m_pool.createWorker(this, parameters);
}

/** Calls cancel on a running instance of MonitorLiveData.
 *  Requires that a handle to the MonitorLiveData instance has been set via
 * setLiveListener()
 *  @return A handle to the cancelled algorithm (usable if the method is called
 * directly)
 */
IAlgorithm_const_sptr FileFinderWidget::stopLiveAlgorithm() {
  IAlgorithm_const_sptr theAlgorithmBeingCancelled = m_monitorLiveData;
  if (m_monitorLiveData && m_monitorLiveData->isRunning()) {
    m_monitorLiveData->cancel();
    m_monitorLiveData.reset();
  }
  return theAlgorithmBeingCancelled;
}

/**
 * Called when the file finding thread finishes.  Inspects the result
 * of the thread, and emits fileFound() if it has been successful.
 */
void FileFinderWidget::inspectThreadResult(const FindFilesSearchResults &results) {
  // Update caches before we might exit early
  m_cachedResults = results;
  m_valueForProperty = QString::fromStdString(results.valueForProperty);
  m_foundFiles.clear();
  m_lastFoundFiles.clear();

  // Early exit on failure
  if (!results.error.empty()) {
    setFileProblem(QString::fromStdString(results.error));
    emit fileInspectionFinished();
    return;
  }

  for (const auto &filename : results.filenames) {
    m_foundFiles.append(QString::fromStdString(filename));
  }
  if (m_foundFiles.isEmpty() && !isOptional()) {
    setFileProblem("No files found. Check search paths and instrument selection.");
  } else if (m_foundFiles.count() > 1 && this->allowMultipleFiles() == false) {
    setFileProblem("Multiple files specified.");
  } else {
    setFileProblem("");
  }

  emit fileInspectionFinished();

  // Only emit the signal if file(s) were found
  if (!m_foundFiles.isEmpty())
    emit filesFound();
  if (m_lastFoundFiles != m_foundFiles)
    emit filesFoundChanged();
}

/**
 * Read settings from the given group
 * @param group :: The name of the group key to retrieve data from
 */
void FileFinderWidget::readSettings(const QString &group) {
  QSettings settings;
  settings.beginGroup(group);
  m_lastDir = settings.value("last_directory", "").toString();

  if (m_lastDir == "") {
    QStringList datadirs =
        QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("datasearch.directories"))
            .split(";", Qt::SkipEmptyParts);
    if (!datadirs.isEmpty())
      m_lastDir = datadirs[0];
  }

  settings.endGroup();
}

/**
 * Set a new file filter for the file dialog based on the given extensions
 * @returns A string containing the file filter
 */
QString FileFinderWidget::createFileFilter() {
  QStringList fileExts;
  if (m_algorithmProperty.isEmpty()) {
    if (!m_fileExtensions.isEmpty()) {
      fileExts = m_fileExtensions;
    } else if (isForRunFiles()) {
      std::vector<std::string> exts = ConfigService::Instance().getFacility().extensions();
      for (auto &ext : exts) {
        fileExts.append(QString::fromStdString(ext));
      }
    } else {
    }
  } else {
    QStringList elements = m_algorithmProperty.split("|");
    if (elements.size() == 2) {
      fileExts = getFileExtensionsFromAlgorithm(elements[0], elements[1]);
    }
  }

  QString allFiles("All Files (*)");
  if (!fileExts.isEmpty()) {

    // The list may contain upper and lower cased versions, ensure these are on
    // the same line
    // I want this ordered
    QList<QPair<QString, QStringList>> finalIndex;
    QStringListIterator sitr(fileExts);
    QString ext = sitr.next();
    finalIndex.append(qMakePair(ext.toUpper(), QStringList(ext)));
    while (sitr.hasNext()) {
      ext = sitr.next();
      QString key = ext.toUpper();
      bool found(false);
      const int itemCount = finalIndex.count();
      for (int i = 0; i < itemCount; ++i) {
        if (key == finalIndex[i].first) {
          finalIndex[i].second.append(ext);
          found = true;
          break;
        }
      }
      if (!found) {
        finalIndex.append(qMakePair(key, QStringList(ext)));
      }
    }

    // The file filter consists of three parts, which we will combine to create
    // the
    // complete file filter:
    QString dataFiles("Data Files (");
    QString individualFiles("");

    if (extsAsSingleOption()) {
      QListIterator<QPair<QString, QStringList>> itr(finalIndex);
      while (itr.hasNext()) {
        const QStringList values = itr.next().second;

        individualFiles += "*" + values.join(" *") + ";;";
        dataFiles += "*" + values.join(" *") + " ";
      }
      // Don't remove final ;; from individualFiles as we are going to tack on
      // allFiles anyway
      dataFiles.chop(1); // Remove last space
      dataFiles += ");;";
    } else {
      QListIterator<QPair<QString, QStringList>> itr(finalIndex);
      while (itr.hasNext()) {
        const QStringList values = itr.next().second;
        dataFiles += "*" + values.join(" *") + ";;";
      }
    }
    return dataFiles + individualFiles + allFiles;
  } else {
    return allFiles;
  }
}

/**
 * Create a list of file extensions from the given algorithm
 * @param algName :: The name of the algorithm
 * @param propName :: The name of the property
 * @returns A list of file extensions
 */
QStringList FileFinderWidget::getFileExtensionsFromAlgorithm(const QString &algName, const QString &propName) {
  Mantid::API::IAlgorithm_sptr algorithm =
      Mantid::API::AlgorithmManager::Instance().createUnmanaged(algName.toStdString());
  QStringList fileExts;
  if (!algorithm)
    return fileExts;
  algorithm->initialize();
  Property *prop = algorithm->getProperty(propName.toStdString());
  FileProperty *fileProp = dynamic_cast<FileProperty *>(prop);
  auto *multiFileProp = dynamic_cast<MultipleFileProperty *>(prop);

  std::vector<std::string> allowed;
  QString preferredExt;

  if (fileProp) {
    allowed = fileProp->allowedValues();
    preferredExt = QString::fromStdString(fileProp->getDefaultExt());
  } else if (multiFileProp) {
    allowed = multiFileProp->allowedValues();
    preferredExt = QString::fromStdString(multiFileProp->getDefaultExt());
  } else {
    return fileExts;
  }

  std::vector<std::string>::const_iterator iend = allowed.end();
  int index(0);
  for (std::vector<std::string>::const_iterator it = allowed.begin(); it != iend; ++it) {
    if (!it->empty()) {
      QString ext = QString::fromStdString(*it);
      fileExts.append(ext);
      if (ext == preferredExt) {
        fileExts.move(index, 0);
      }
      ++index;
    }
  }

  return fileExts;
}

/** Lauches a load file browser allowing a user to select multiple
 *  files
 *  @return the names of the selected files as a comma separated list
 */
QString FileFinderWidget::openFileDialog() {
  QStringList filenames;
  QString dir;

  auto prevFileNames = getText().split(",", Qt::SkipEmptyParts);
  for (auto &prevFileName : prevFileNames)
    prevFileName = prevFileName.trimmed();

  if (!prevFileNames.empty() && QFileInfo(prevFileNames[0]).isAbsolute()) {
    if (QFileInfo(prevFileNames[0]).isFile()) {
      dir = QFileInfo(prevFileNames[0]).absoluteDir().path();
    } else {
      dir = prevFileNames[0];
    }
  } else {
    dir = m_lastDir;
  }

  if (m_fileFilter.isEmpty()) {
    m_fileFilter = createFileFilter();
  }

  if (!m_useNativeDialog) {
    if (m_isForDirectory) {
      m_dialog.setOption(QFileDialog::DontResolveSymlinks, false);
      m_dialog.setFileMode(QFileDialog::Directory);
    } else {
      m_dialog.setNameFilter(m_fileFilter);
      m_dialog.setOption(QFileDialog::DontResolveSymlinks);
      if (m_allowMultipleFiles)
        m_dialog.setFileMode(QFileDialog::ExistingFiles);
      else
        m_dialog.setFileMode(QFileDialog::ExistingFile);
    }
    m_dialog.setDirectory(dir);
    if (m_dialog.exec())
      filenames = m_dialog.selectedFiles();
  } else {
    if (m_isForDirectory) {
      QString file = QFileDialog::getExistingDirectory(this, "Select directory", dir);
      if (!file.isEmpty())
        filenames.append(file);
    } else if (m_allowMultipleFiles) {
      filenames = QFileDialog::getOpenFileNames(this, "Open file", dir, m_fileFilter, nullptr,
                                                QFileDialog::DontResolveSymlinks);
    } else {
      QString file =
          QFileDialog::getOpenFileName(this, "Open file", dir, m_fileFilter, nullptr, QFileDialog::DontResolveSymlinks);
      if (!file.isEmpty())
        filenames.append(file);
    }
  }

  if (filenames.isEmpty()) {
    return "";
  }
  m_lastDir = QFileInfo(filenames.front()).absoluteDir().path();
  return filenames.join(", ");
}

/** flag a problem with the supplied entry number, an empty string means no
 * error.
 *  file errors take precedence of these errors
 *  @param message the message to display
 */
void FileFinderWidget::setEntryNumProblem(const QString &message) {
  m_entryNumProblem = message;
  refreshValidator();
}

/** Checks the data m_fileProblem and m_entryNumProblem to see if the validator
 * label needs to be displayed. Validator always hidden if m_showValidator set
 * false.
 */
void FileFinderWidget::refreshValidator() {
  if (m_showValidator) {
    if (!m_fileProblem.isEmpty()) {
      m_uiForm.valid->setToolTip(m_fileProblem);
      m_uiForm.valid->show();
    } else if (!m_entryNumProblem.isEmpty() && m_multiEntry) {
      m_uiForm.valid->setToolTip(m_entryNumProblem);
      m_uiForm.valid->show();
    } else {
      m_uiForm.valid->hide();
    }
  } else {
    m_uiForm.valid->hide();
  }
}

/** This slot opens a file browser
 */
void FileFinderWidget::browseClicked() {
  QString uFile = openFileDialog();
  if (uFile.trimmed().isEmpty())
    return;

  m_uiForm.fileEditor->setText(uFile);
  m_uiForm.fileEditor->setModified(true);

  emit fileEditingFinished();
}

/** Currently just checks that entryNum contains an int > 0 and hence might be a
 *  valid entry number
 */
void FileFinderWidget::checkEntry() {
  if (m_uiForm.entryNum->text().isEmpty()) {
    setEntryNumProblem("");
    return;
  }

  bool good;
  const int num = m_uiForm.entryNum->text().toInt(&good);
  if (!good) {
    setEntryNumProblem("The entry number must be an integer");
    return;
  }
  if (num < 1) {
    setEntryNumProblem("The entry number must be an integer > 0");
    return;
  }

  setEntryNumProblem("");
}

/**
 * Called when an item is dropped
 * @param de :: the drop event data package
 */
void FileFinderWidget::dropEvent(QDropEvent *de) {
  const QMimeData *mimeData = de->mimeData();
  const auto filenames = DropEventHelper::getFileNames(de);
  if (!filenames.empty()) {
    m_uiForm.fileEditor->setText(filenames[0]);
    de->acceptProposedAction();
  } else if (mimeData->hasText()) {
    QString text = mimeData->text();
    m_uiForm.fileEditor->setText(text);
    de->acceptProposedAction();
  }
}

/**
 * Called when an item is dragged onto a control
 * @param de :: the drag event data package
 */
void FileFinderWidget::dragEnterEvent(QDragEnterEvent *de) {
  const QMimeData *mimeData = de->mimeData();
  if (mimeData->hasUrls()) {
    auto listurl = mimeData->urls();
    if (listurl.empty())
      return;
    if (!listurl[0].isLocalFile())
      return;
    de->acceptProposedAction();
  } else if (mimeData->hasText()) {
    QString text = mimeData->text();
    if (text.contains(" = mtd[\""))
      de->setDropAction(Qt::IgnoreAction);
    else
      de->acceptProposedAction();
  }
}

/**
 * Sets the text read-only or editable
 * and the Browse button disabled or enabled.
 * If read-only, disable the red asterisk validator.
 * @param readOnly :: [input] whether read-only or editable
 */
void FileFinderWidget::setReadOnly(bool readOnly) {
  m_uiForm.fileEditor->setReadOnly(readOnly);
  m_uiForm.browseBtn->setEnabled(!readOnly);
  setValidatorDisplay(!readOnly);
  refreshValidator();
}

/**
 * Turn on/off the display of the red validator star.
 * Validation is still performed, this just controls the display of the
 * result.
 * @param display :: [input] whether to show validator result or not
 */
void FileFinderWidget::setValidatorDisplay(bool display) { m_showValidator = display; }

FindFilesSearchParameters FileFinderWidget::createFindFilesSearchParameters(const std::string &text) const {
  FindFilesSearchParameters parameters;
  parameters.searchText = text;
  parameters.isOptional = isOptional();
  parameters.isForRunFiles = isForRunFiles();
  parameters.extensions = getStringFileExtensions();

  // parse the algorithm - property name string
  QStringList elements = m_algorithmProperty.split("|");
  if (elements.size() == 2) {
    parameters.algorithmName = elements[0].toStdString();
    parameters.algorithmProperty = elements[1].toStdString();
  }

  return parameters;
}

void FileFinderWidget::setTextValidator(const QValidator *validator) { m_uiForm.fileEditor->setValidator(validator); }

void FileFinderWidget::setUseNativeWidget(bool native) {
  m_useNativeDialog = native;
  m_dialog.setOption(QFileDialog::DontUseNativeDialog);
  m_dialog.setOption(QFileDialog::ReadOnly);
}

void FileFinderWidget::setProxyModel(QAbstractProxyModel *proxyModel) { m_dialog.setProxyModel(proxyModel); }
