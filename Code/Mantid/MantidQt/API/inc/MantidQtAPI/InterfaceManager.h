#ifndef MANTIDQT_API_DIALOGMANAGER_H_
#define MANTIDQT_API_DIALOGMANAGER_H_

//----------------------------------
// Includes
//----------------------------------
#include "DllOption.h"
//#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/Instantiator.h"

#include <QString>
#include <QStringList>
#include <QHash>

//----------------------------------
// Qt Forward declarations
//----------------------------------
class QWidget;

//----------------------------------
// Mantid forward declarations
//----------------------------------
namespace Mantid 
{
namespace API
{
  class IAlgorithm;
}

}

// Top level namespace for this library
namespace MantidQt
{

namespace API
{

//----------------------------------
// Forward declarations
//----------------------------------
class AlgorithmDialog;
class UserSubWindow;
class VatesViewerInterface;
class MantidHelpInterface;

/** 
    This class is responsible for creating the correct dialog for an algorithm. If 
    no specialized version is registered for that algorithm then the default is created
    
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
class EXPORT_OPT_MANTIDQT_API InterfaceManager
{

public:
  
  /// Create a new instance of the correct type of AlgorithmDialog
  AlgorithmDialog* createDialog(boost::shared_ptr<Mantid::API::IAlgorithm> alg, QWidget* parent = 0,
        bool forScript = false, const QHash<QString,QString> & presetValues = (QHash<QString,QString>()),
        const QString & optional_msg = QString(), const QStringList & enabled = QStringList(),
        const QStringList & disabled = QStringList());

  /// Create an algorithm dialog for a given name and version
  AlgorithmDialog* createDialogFromName(const QString& algorithmName, const int version = -1,
                                        QWidget* parent = 0, bool forScript = false,
                                        const QHash<QString,QString> & presetValues = (QHash<QString,QString>()),
                                        const QString & optionalMsg = QString(), const QStringList & enabled = QStringList(),
                                        const QStringList & disabled = QStringList());

  /// Create a new instance of the correct type of UserSubWindow
  UserSubWindow* createSubWindow(const QString & interface_name, QWidget* parent = 0);

  /**
   * Function that instantiates the Vates simple user interface.
   * @return the Vates simple user interface
   */
  VatesViewerInterface *createVatesSimpleGui() const;
  /**
   * Registration function for the Vates simple interface factory.
   * @param factory the factory instance
   */
  static void registerVatesGuiFactory(Mantid::Kernel::AbstractInstantiator<VatesViewerInterface> *factory);

  /**
   * Function that instantiates the help window.
   * @return the help window
   */
  MantidHelpInterface *createHelpWindow() const;
  /**
   * Registration function for the help window factory.
   * @param factory the factory instance
   */
  static void registerHelpWindowFactory(Mantid::Kernel::AbstractInstantiator<MantidHelpInterface> *factory);

  /// The keys associated with UserSubWindow classes
  QStringList getUserSubWindowKeys() const;

  /// Getter for vates libraries availablity
  static bool hasVatesLibraries();

  ///Constructor
  InterfaceManager();
  ///Destructor
  virtual ~InterfaceManager();

private:
  /// Handle to the Vates simple user interface factory
  static Mantid::Kernel::AbstractInstantiator<VatesViewerInterface> *m_vatesGuiFactory;
  /// Handle to the help window factory
  static Mantid::Kernel::AbstractInstantiator<MantidHelpInterface> *m_helpViewer;
};

}
}

/*
 * Used to register Vates GUI
 */
#define REGISTER_VATESGUI(TYPE) \
  namespace { \
  Mantid::Kernel::RegistrationHelper \
  register_vatesgui \
  (((MantidQt::API::InterfaceManager::registerVatesGuiFactory \
  (new Mantid::Kernel::Instantiator<TYPE, VatesViewerInterface>())), 0)); \
}

/// Used to register help window
#define REGISTER_HELPWINDOW(TYPE) \
  namespace { \
  Mantid::Kernel::RegistrationHelper \
  register_helpviewer \
  (((MantidQt::API::InterfaceManager::registerHelpWindowFactory \
  (new Mantid::Kernel::Instantiator<TYPE, MantidHelpInterface>())), 0)); \
}
#endif //MANTIDQT_API_DIALOGMANAGER
