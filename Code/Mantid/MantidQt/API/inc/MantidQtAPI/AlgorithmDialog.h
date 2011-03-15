#ifndef MANTIDQT_API_ALGORITHMDIALOG_H_
#define MANTIDQT_API_ALGORITHMDIALOG_H_

/* Used to register classes into the factory. creates a global object in an
* anonymous namespace. The object itself does nothing, but the comma operator
* is used in the call to its constructor to effect a call to the factory's
* subscribe method.
*/

#define DECLARE_DIALOG(classname) \
  namespace { \
    Mantid::Kernel::RegistrationHelper \
    register_dialog_##classname \
    (((MantidQt::API::AlgorithmDialogFactory::Instance().subscribe<classname>(#classname)), 0)); \
  }

//----------------------------------
// Includes
//----------------------------------
#include "DllOption.h"
#include "InterfaceFactory.h"

// Could have forward declared this but it makes it easier to use from
// inheriting classes if it is included here
#include "MantidAPI/IAlgorithm.h"

#include <QDialog>
#include <QString>
#include <QHash>
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
namespace Mantid
{
  namespace Kernel
  {
    class Property;
  }
}

//Top-level namespace for this library
namespace MantidQt
{

namespace API 
{

//----------------------------------
// Forward declarations
//----------------------------------
class InterfaceManagerImpl;

/** 
    This class should be the basis for all customised algorithm dialogs.

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
class EXPORT_OPT_MANTIDQT_API AlgorithmDialog : public QDialog
{
  
  Q_OBJECT

public:

  /// DefaultConstructor
  AlgorithmDialog(QWidget* parent = 0);
  /// Destructor
  virtual ~AlgorithmDialog();

  /// Create the layout of the widget. Can only be called once.
  void initializeLayout();

  /// Is this dialog initialized
  bool isInitialized() const;

protected:

  /** @name Virtual functions */
  //@{
  /// This does the work and must be overridden in each deriving class
  virtual void initLayout() = 0;

  /// Parse out the values entered into the dialog boxes. Use storePropertyValue()
  /// to store the <name, value> pair in the base class so that they can be retrieved later
  virtual void parseInput();

  /// Save the input history of an accepted dialog
  virtual void saveInput();
  //@}

  /** @name Algorithm information */
  // InterfaceManager needs to be able to reset the algorithm as I can't pass it in use a 
  // constructor
  friend class InterfaceManagerImpl;
  
  /// Set the algorithm associated with this dialog
  void setAlgorithm(Mantid::API::IAlgorithm*);
  
  /// Get the algorithm pointer
  Mantid::API::IAlgorithm* getAlgorithm() const;

  /// Get a pointer to the named property 
  Mantid::Kernel::Property* getAlgorithmProperty(const QString & propName) const;
  /// Return a true if the given property requires user input
  bool requiresUserInput(const QString & propName) const;

  /// Get an input value from the form, dealing with blank inputs etc
  QString getInputValue(const QString& propName) const;
  /// Get a property validator label
  QLabel* getValidatorMarker(const QString & propname) const;
  
  /// Adds a property (name,value) pair to the stored map
  void storePropertyValue(const QString & name, const QString & value);

  /// Set the properties that have been parsed from the dialog
  bool setPropertyValues();
  //@}

  /** @name Dialog information */
  //@{
  /// Get the message string
  const QString & getOptionalMessage() const;

  /// Add the optional message to the given layout.
  void addOptionalMessage(QVBoxLayout *mainLay);

  /// Get the usage boolean value
  bool isForScript() const;
 
  /// Is there a message string available
  bool isMessageAvailable() const;

  /// Check is a given property should have its control enabled or not
  bool isWidgetEnabled(const QString & propName) const;
  //@}

  /** @name Helper functions */
  //@{
  ///Tie a widget to a property
  QWidget* tie(QWidget* widget, const QString & property, QLayout* parent_layout, bool readHistory = true);
  
  /// Open a file dialog to select a file.
  QString openFileDialog(const QString & propName);

  /// Fill a combo box for the named algorithm's allowed values
  void fillAndSetComboBox(const QString & propName, QComboBox* optionsBox) const;

  /// Set the state of a check box for the named algorithm's boolean property
  void setCheckBoxState(const QString & propName, QCheckBox* checkBox) const;
  
  /// Fill in the necessary input for a text field 
  void fillLineEdit(const QString & propName, QLineEdit* field);

  /// Create a row layout of buttons with specified text
  QHBoxLayout *createDefaultButtonLayout(const QString & helpText = QString("?"),
					 const QString & loadText = QString("Run"), 
					 const QString & cancelText = QString("Cancel"));

  /// Create a help button for this algorithm
  QPushButton* createHelpButton(const QString & helpText = QString("?")) const;
  
  /// Create a push button that when pressed will cause the input workspace to be replaced.
  QPushButton* createReplaceWSButton(QLineEdit *outputEdit);

  /// Flag an input workspace combobox with its property name
  void flagInputWS(QWidget *inputWidget);
  //@}
						       
 protected slots:
  
  /// A default slot that can be used for an OK button.
  virtual void accept();

  /// Help button clicked;
  virtual void helpClicked();

  /// Replace WS
  void replaceWSClicked(QWidget *outputEdit);

private:

  /// Parse out the input from the dialog
  void parse();
  /// Set a list of suggested values  
  void setPresetValues(const QString & presetValues);
  /// Set comma-separated-list of enabled parameter names
  void setEnabledNames(const QString & enabledNames);
  /// Test if the given name's widget should be left enabled
  bool isInEnabledList(const QString& propName) const;
  /// Set whether this is intended for use from a script or not
  void isForScript(bool forScript);
  /// Set an optional message to be displayed at the top of the dialog
  void setOptionalMessage(const QString & message);
  /// Retrieve a text value for a property from a widget
  QString getValue(QWidget *widget);
  /// Set a value based on any old input that we have
  void setPreviousValue(QWidget *widget, const QString & property);
    
private:
  /** @name Member variables. */
  //@{
  /// The algorithm associated with this dialog
  Mantid::API::IAlgorithm *m_algorithm;

  ///The name of the algorithm
  QString m_algName;
  /// The properties associated with this dialog
  QStringList m_algProperties;
  /// A map of property <name, value> pairs that have been taken from the dialog
  QHash<QString, QString> m_propertyValueMap;
  /// A list pointers to PropertyWidget objects
  QHash<QString, QWidget*> m_tied_properties;
  /// A boolean indicating whether this is for a script or not
  bool m_forScript;
  /// A list of property names that have been passed from Python
  QStringList m_python_arguments;
  /// A list of property names that should have their widgets enabled
  QStringList m_enabledNames;
  /// The message string to be displayed at the top of the widget; if it exists.
  QString m_strMessage;
  /// Is the message string empty or not
  bool m_msgAvailable;

  /// Whether the layout has been initialized
  bool m_isInitialized;

  /// A list of labels to use as validation markers
  mutable QHash<QString, QLabel*> m_validators;
  /// A list of property names whose widgets handle their own validation
  QStringList m_noValidation;

  /// Store a list of the names of input workspace boxes
  QVector<QWidget*> m_inputws_opts;

  /// Store a list of output workspace text edits
  QVector<QLineEdit*> m_outputws_fields;

  /// A map to keep track of replace workspace button presses
  QHash<QPushButton*, int> m_wsbtn_tracker;

  /// A signal mapper
  QSignalMapper *m_signal_mapper;
  //@}
};

}
}

// The main page of the doxygen
/** @mainpage MantidQt
 * A library containing a set of Qt dialog widgets that are specialized to particular algorithms. There
 * is also a basic dialog that provides default functionality
 */


#endif //MANTIDQT_API_ALGORITHMDIALOG_H_
