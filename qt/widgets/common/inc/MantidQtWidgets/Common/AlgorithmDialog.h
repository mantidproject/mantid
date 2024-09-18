// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

/* Used to register classes into the factory. creates a global object in an
 * anonymous namespace. The object itself does nothing, but the comma operator
 * is used in the call to its constructor to effect a call to the factory's
 * subscribe method.
 */

#define DECLARE_DIALOG(classname)                                                                                      \
  namespace {                                                                                                          \
  Mantid::Kernel::RegistrationHelper register_dialog_##classname(                                                      \
      ((MantidQt::API::AlgorithmDialogFactory::Instance().subscribe<classname>(#classname)), 0));                      \
  }

//----------------------------------
// Includes
//----------------------------------
#include "AlgorithmDialogFactory.h"
#include "DllOption.h"

// Could have forward declared this but it makes it easier to use from
// inheriting classes if it is included here
#include "MantidAPI/AlgorithmObserver.h"
#include "MantidAPI/IAlgorithm.h"

#include <QDialog>
#include <QHash>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>

//----------------------------------
// Qt Forward declarations
//----------------------------------
class QLabel;
class QLineEdit;
class QComboBox;
class QCheckBox;
class QPushButton;
class QHBoxLayout;
class QSignalMapper;
class QLayout;

//----------------------------------
// Mantid Forward declarations
//----------------------------------
namespace Mantid {
namespace Kernel {
class Property;
}
} // namespace Mantid

// Top-level namespace for this library
namespace MantidQt {

namespace API {

//----------------------------------
// Forward declarations
//----------------------------------
class InterfaceManager;

/**
    This class should be the basis for all customised algorithm dialogs.

    @author Martyn Gigg, Tessella Support Services plc
    @date 24/02/2009
*/
class EXPORT_OPT_MANTIDQT_COMMON AlgorithmDialog : public QDialog, Mantid::API::AlgorithmObserver {

  Q_OBJECT

public:
  /// DefaultConstructor
  AlgorithmDialog(QWidget *parent = nullptr);
  /// Destructor
  ~AlgorithmDialog() override;

  /// Set if the keep open option is shown.
  void setShowKeepOpen(const bool showOption);

  /// Set if the keep open option is shown.
  bool isShowKeepOpen() const;

  /// Create the layout of the widget. Can only be called once.
  void initializeLayout();

  /// Is this dialog initialized
  bool isInitialized() const;

protected:
  /** @name Virtual functions */
  //@{
  /// This does the work and must be overridden in each deriving class
  virtual void initLayout() = 0;

  /// Parse out the values entered into the dialog boxes. Use
  /// storePropertyValue()
  /// to store the <name, value> pair in the base class so that they can be
  /// retrieved later
  virtual void parseInput();

  /// Save the input history of an accepted dialog
  virtual void saveInput();
  //@}

  /** @name Algorithm information */
  // InterfaceManager needs to be able to reset the algorithm as I can't pass it
  // in use a
  // constructor
  friend class InterfaceManager;

  /// Get the algorithm pointer
  Mantid::API::IAlgorithm_sptr getAlgorithm() const;

  /// Get a pointer to the named property
  Mantid::Kernel::Property *getAlgorithmProperty(const QString &propName) const;
  /// Return a true if the given property requires user input
  bool requiresUserInput(const QString &propName) const;

  /// Get an input value from the form, dealing with blank inputs etc
  QString getInputValue(const QString &propName) const;
  /// Get a property validator label
  QLabel *getValidatorMarker(const QString &propname);

  /// Adds a property (name,value) pair to the stored map
  void storePropertyValue(const QString &name, const QString &value);

  /// Removes a property (name, value) pair from the stored map
  void removePropertyValue(const QString &name);

  /// Set properties on this algorithm by pulling values from the tied widgets
  bool setPropertyValues(const QStringList &skipList = QStringList());
  bool setPropertyValue(const QString &pName, bool validateOthers);

  void showValidators();
  //@}

  /** @name Dialog information */
  //@{
  /// Get the message string
  const QString &getOptionalMessage() const;

  /// Add the optional message to the given layout.
  void addOptionalMessage(QVBoxLayout *mainLay);

  /// Get the usage boolean value
  bool isForScript() const;

  /// Is there a message string available
  bool isMessageAvailable() const;

  /// Check is a given property should have its control enabled or not
  bool isWidgetEnabled(const QString &propName) const;
  //@}

  /** @name Helper functions */
  //@{
  /// Tie a widget to a property
  QWidget *tie(QWidget *widget, const QString &property, QLayout *parent_layout = nullptr, bool readHistory = true);

  /// Untie a widget to a property
  void untie(const QString &property);

  /// Open a file dialog to select a file.
  QString openFileDialog(const QString &propName);

  /// Open a file dialog to select many file.
  QStringList openMultipleFileDialog(const QString &propName);

  /// Fill a combo box for the named algorithm's allowed values
  void fillAndSetComboBox(const QString &propName, QComboBox *optionsBox) const;

  /// Fill in the necessary input for a text field
  void fillLineEdit(const QString &propName, QLineEdit *field);

  /// Create a row layout of buttons with specified text
  QLayout *createDefaultButtonLayout(const QString &helpText = QString("?"), const QString &loadText = QString("Run"),
                                     const QString &cancelText = QString("Close"),
                                     const QString &keepOpenText = QString("Keep Open"));

  /// Create a help button for this algorithm
  QPushButton *createHelpButton(const QString &helpText = QString("?")) const;

  /// Flag an input workspace combobox with its property name
  void flagInputWS(QWidget *inputWidget);
  //@}

  /// Retrieve a text value for a property from a widget
  QString getValue(QWidget *widget);

signals:
  /// Emitted when alg completes and dialog is staying open
  void algCompletedSignal();

  void closeEventCalled();

protected slots:

  /// A default slot that can be used for an OK button.
  void accept() override;

  /// A default slot that can be used for a rejected button.
  void reject() override;

  /// Help button clicked;
  virtual void helpClicked();

  /// Keep open checkbox clicked;
  virtual void keepOpenChanged(int state);

  /// Keep the running algorithm has completed
  virtual void algorithmCompleted();

  /// Executes the algorithm in a separate thread
  virtual void executeAlgorithmAsync();
  /// Removes the algorithm from the manager.
  virtual void removeAlgorithmFromManager();
  /// Enable to exit button
  void enableExitButton();

protected:
  /// Parse out the input from the dialog
  void parse();
  /// Test if the given name's widget has been explicity asked to be enabled
  bool requestedToKeepEnabled(const QString &propName) const;
  /// Get the property value from either the previous input store or from Python
  /// argument
  /// @param propName :: Name of the property
  /// @return Previous value. If there is no value, empty string is returned
  QString getPreviousValue(const QString &propName) const;
  /// Set a value based on any old input that we have
  void setPreviousValue(QWidget *widget, const QString &property);
  /// Handle completion of algorithm started while staying open
  void finishHandle(const Mantid::API::IAlgorithm *alg) override;
  /// Handle completion of algorithm started while staying open
  void errorHandle(const Mantid::API::IAlgorithm *alg, const std::string &what) override;
  void closeEvent(QCloseEvent *evt) override;

  /// The following methods were made public for testing in
  /// GenericDialogDemo.cpp
public:
  /// Set the algorithm associated with this dialog
  void setAlgorithm(const Mantid::API::IAlgorithm_sptr & /*alg*/);
  /// Set a list of suggested values
  void setPresetValues(const QHash<QString, QString> &presetValues);
  /// Set whether this is intended for use from a script or not
  void isForScript(bool forScript);
  /// If true then execute the algorithm on acceptance
  void executeOnAccept(bool on);
  /// Set an optional message to be displayed at the top of the dialog
  void setOptionalMessage(const QString &message);
  /// Set comma-separated-list of enabled parameter names
  void addEnabledAndDisableLists(const QStringList &enabled, const QStringList &disabled);
  /// Add an AlgorithmObserver to the algorithm
  void addAlgorithmObserver(Mantid::API::AlgorithmObserver *observer);
  /// Disable the exit button
  void disableExitButton();

protected:
  /** @name Member variables. */
  //@{
  /// The algorithm associated with this dialog
  Mantid::API::IAlgorithm_sptr m_algorithm;

  /// The name of the algorithm
  QString m_algName;
  /// The properties associated with this dialog
  QStringList m_algProperties;

  /// A map of property <name, value> pairs that have been taken from the dialog
  QHash<QString, QString> m_propertyValueMap;

  /// A list pointers to the widget for each property
  QHash<QString, QWidget *> m_tied_properties;

  /// A boolean indicating whether this is for a script or not
  bool m_forScript;
  /// A list of property names that have been passed from Python
  QStringList m_python_arguments;
  /// A list of property names that should have their widgets enabled
  QStringList m_enabled;
  /// A list of property names that the user has requested to be disabled
  /// (overrides those in enabled)
  QStringList m_disabled;
  /// The message string to be displayed at the top of the widget; if it exists.
  QString m_strMessage;
  /// Whether to keep the dialog box open after alg execution
  bool m_keepOpen;
  /// Is the message string empty or not
  bool m_msgAvailable;

  /// Whether the layout has been initialized
  bool m_isInitialized;
  /// Flag if the input should be parsed automatically on initialization
  bool m_autoParseOnInit;

  /// A list of labels to use as validation markers
  QHash<QString, QLabel *> m_validators;

  /// A map where key = property name; value = the error for this property (i.e.
  /// it is not valid).
  QHash<QString, QString> m_errors;

  /// A list of property names whose widgets handle their own validation
  QStringList m_noValidation;

  /// Store a list of the names of input workspace boxes
  QVector<QWidget *> m_inputws_opts;

  /// Store a list of output workspace text edits
  QVector<QLineEdit *> m_outputws_fields;

  /// A map to keep track of replace workspace button presses
  QHash<QPushButton *, int> m_wsbtn_tracker;

  // the keep open checkbox control
  QCheckBox *m_keepOpenCheckBox;

  QPushButton *m_okButton, *m_exitButton;

  /// A list of AlgorithmObservers to add to the algorithm prior to execution
  std::vector<Mantid::API::AlgorithmObserver *> m_observers;

  /// Enable the close button when the timer fires
  QTimer m_btnTimer;
  /// A flag to track whether the status of the algorithm is being tracked
  bool m_statusTracked;
  //@}
};
} // namespace API
} // namespace MantidQt