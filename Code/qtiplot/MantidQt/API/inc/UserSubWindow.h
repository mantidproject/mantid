#ifndef MANTIDQT_API_USERSUBWINDOW_H_
#define MANTIDQT_API_USERSUBWINDOW_H_


/* Used to register classes into the factory. Creates a global object in an
* anonymous namespace. The object itself does nothing, but the comma operator
* is used in the call to its constructor to effect a call to the factory's
* subscribe method.
*/
#define DECLARE_SUBWINDOW(classname) \
  namespace { \
    Mantid::Kernel::RegistrationHelper \
    register_subwindow_##classname \
    (((MantidQt::API::UserSubWindowFactory::Instance().subscribe<classname>()), 0)); \
  }

//----------------------------------
// Includes
//----------------------------------
#include "DllOption.h"
#include "InterfaceFactory.h"

#include <QWidget>
#include <QStringList>
#include <QLabel>
#include <set>

#include <Poco/Message.h>

//----------------------------------
// Qt Forward declarations
//----------------------------------

//----------------------------------
// Mantid Forward declarations
//----------------------------------

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
    This is the base class all customised user interfaces that do not wish to be tied
    to a specific Mantid algorithm but rather customised for user's requirements

    @author Martyn Gigg, Tessella Support Services plc
    @date 18/03/2009

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
class EXPORT_OPT_MANTIDQT_API UserSubWindow : public QWidget
{
  Q_OBJECT

public:
  /// Name of the interface
  static std::string name() { return "UserSubWindow::name() default Reimplement static name() method."; }
  /// A list of aliases
  static std::set<std::string> aliases() { return std::set<std::string>(); }

public:
  /// DefaultConstructor
  UserSubWindow(QWidget* parent = 0);
  /// Destructor
  virtual ~UserSubWindow();
  /// Create the layout of the widget. Can only be called once.
  void initializeLayout();
  /// Run local Python init code. Calls overridable function in specialized interface
  void initializeLocalPython();
  /// Is this dialog initialized
  bool isInitialized() const;
  /// Has the Python initialization function been run
  bool isPyInitialized() const;

  /// A boost 'slot' for the Mantid signal channel connection
  void mantidLogReceiver(const Poco::Message & msg);
  
signals:
  /// Emitted to start a (generally small) script running
  void runAsPythonScript(const QString& code);
  ///Mantid log message recieved
  void logMessageReceived(const QString & msg);

protected:
  /**@name Virtual Functions */
  //@{
  /// To be overridden to set the appropriate layout
  virtual void initLayout() = 0;
  /// Run local Python setup code
  virtual void initLocalPython() {}
  //@}
  
  /// Raise a dialog box giving some information
  void showInformationBox(const QString & message) const;
  
  /// Run a piece of python code and return any output that was written to stdout
  QString runPythonCode(const QString & code, bool no_output = false);
  QString openFileDialog(const bool save, const QStringList &exts);
  QLabel* newValidator(QWidget *parent);

private:
  // This is so that it can set the name
  // I can't pass anything as an argument to the constructor as I am using
  // the DynamicFactory
  friend class InterfaceManagerImpl;

  /// Set the interface name
  void setInterfaceName(const QString & iface_name);
  /// Connect this object to Mantid's signal channel
  bool connectToMantidSignal();
  /// Has this already been initialized
  bool m_bIsInitialized;
  /// Has the python initialization been run
  bool m_isPyInitialized;
  /// Store the name of the interface
  QString m_ifacename;
};

}
}

#endif //MANTIDQT_API_USERSUBWINDOW_H_
