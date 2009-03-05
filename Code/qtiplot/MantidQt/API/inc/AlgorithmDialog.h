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
    (((MantidQt::API::DialogFactory::Instance().subscribe<MantidQt::CustomDialogs::classname>(#classname)), 0)); \
  }

//----------------------------------
// Includes
//----------------------------------
#include "DllOption.h"
#include "DialogFactory.h"

#include "MantidAPI/Algorithm.h"

#include <QDialog>
#include <QString>
#include <QHash>
#include <QMap>

//----------------------------------
// Qt Forward declarations
//----------------------------------
class QLabel;
class QLineEdit;

//----------------------------------
// Mantid Forward declarations
//----------------------------------
namespace Mantid
{
namespace Kernel
{
  class Property;
}
namespace API
{
  class Algorithm;
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
class DialogManagerImpl;

/** 
    This class gives a basic dialog that is not tailored to a particular 
    algorithm.

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

  /// This does the work and must be overridden in each deriving class
  virtual void initLayout() = 0;

  /// Parse out the values entered into the dialog boxes. Use addPropertyValueToMap()
  /// to store the <name, value> pair in the base class so that they can be retrieved later
  virtual void parseInput() = 0;

  /// Get the algorithm pointer
  Mantid::API::Algorithm* getAlgorithm() const;

  /// Get the usage boolean value
  bool isForScript() const;

  /// Get the message string
  const QString & getOptionalMessage() const;

  /// Is there a message string available
  bool isMessageAvailable() const;

  /// Get the a named property 
  Mantid::Kernel::Property* getAlgorithmProperty(const QString & propName) const;
  
  /// Get a property validator label
  QLabel* getValidatorMarker(const QString & propname) const;
  
  /// Adds a property (name,value) pair to the stored map
  void addPropertyValueToMap(const QString & name, const QString & value);

  /// Checks the properties to find if they are valid
  bool validateProperties();

  /// Set the properties that have been parsed from the dialog
  bool setPropertyValues();

  /// Open a file dialog to select an existing file
  QString openLoadFileDialog(const QString & propName); 

  //  Set old input for line edit field
  void setOldLineEditInput(const QString & propName, QLineEdit* field);
	      
protected slots:
  
  /// A default slot that can be used for an OK button.
  virtual void accept();

private:
  // This is so that it can set the algorithm and initialize the layout.
  // I can't pass the algorithm as an argument to the constructor as I am using
  // the DynamicFactory
  friend class DialogManagerImpl;
  
  /// Set the algorithm associated with this dialog
  void setAlgorithm(Mantid::API::Algorithm*);
  
  /// Set whether this is intended for use from a script or not
  void isForScript(bool forScript);

  /// Set an optional message to be displayed at the top of the dialog
  void setOptionalMessage(const QString & message);

  /// This sets up the labels that are to be used to mark whether a property is valid.
  void createValidatorLabels();

  /** @name Member variables. */
  //@{
  /// The algorithm associated with this dialog
  Mantid::API::Algorithm *m_algorithm;

  ///The name of the algorithm
  QString m_algName;

  /// A map of property <name, value> pairs
  QHash<QString, QString> m_propertyValueMap;

  /// A boolean indicating whether this is for a sciprt or not
  bool m_forScript;

  /// The message string to be displayed at the top of the widget; if it exists.
  QString m_strMessage;

  /// Is the message string empty or not
  bool m_msgAvailable;

  /// Whether the layout has been initialized
  bool m_bIsInitialized;

  /// The properties associated with this algorithm
  QMap<QString, Mantid::Kernel::Property*> m_algProperties;

  /// A list of labels to use as validation markers
  QHash<QString, QLabel*> m_validators;
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
