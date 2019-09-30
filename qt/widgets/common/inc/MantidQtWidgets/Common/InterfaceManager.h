// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_API_DIALOGMANAGER_H_
#define MANTIDQT_API_DIALOGMANAGER_H_

//----------------------------------
// Includes
//----------------------------------
#include "DllOption.h"
//#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/Instantiator.h"

#include <QHash>
#include <QString>
#include <QStringList>

//----------------------------------
// Qt Forward declarations
//----------------------------------
class QWidget;

//----------------------------------
// Mantid forward declarations
//----------------------------------
namespace Mantid {
namespace API {
class IAlgorithm;
}
} // namespace Mantid

// Top level namespace for this library
namespace MantidQt {

namespace API {

//----------------------------------
// Forward declarations
//----------------------------------
class AlgorithmDialog;
class UserSubWindow;
class VatesViewerInterface;
class MantidHelpInterface;

/**
    This class is responsible for creating the correct dialog for an algorithm.
   If
    no specialized version is registered for that algorithm then the default is
   created

    @author Martyn Gigg, Tessella Support Services plc
    @date 24/02/2009
*/
class EXPORT_OPT_MANTIDQT_COMMON InterfaceManager {

public:
  /// Create a new instance of the correct type of AlgorithmDialog
  AlgorithmDialog *createDialog(
      boost::shared_ptr<Mantid::API::IAlgorithm> alg, QWidget *parent = nullptr,
      bool forScript = false,
      const QHash<QString, QString> &presetValues = (QHash<QString, QString>()),
      const QString &optional_msg = QString(),
      const QStringList &enabled = QStringList(),
      const QStringList &disabled = QStringList());

  /// Create an algorithm dialog for a given name and version
  AlgorithmDialog *createDialogFromName(
      const QString &algorithmName, const int version = -1,
      QWidget *parent = nullptr, bool forScript = false,
      const QHash<QString, QString> &presetValues = (QHash<QString, QString>()),
      const QString &optionalMsg = QString(),
      const QStringList &enabled = QStringList(),
      const QStringList &disabled = QStringList());

  /// Create a new instance of the correct type of UserSubWindow
  UserSubWindow *createSubWindow(const QString &interface_name,
                                 QWidget *parent = nullptr);

  /**
   * Function that instantiates the Vates simple user interface.
   * @return the Vates simple user interface
   */
  VatesViewerInterface *createVatesSimpleGui() const;
  /**
   * Registration function for the Vates simple interface factory.
   * @param factory the factory instance
   */
  static void registerVatesGuiFactory(
      Mantid::Kernel::AbstractInstantiator<VatesViewerInterface> *factory);

  /**
   * Function that instantiates the help window.
   * @return the help window
   */
  MantidHelpInterface *createHelpWindow() const;

  /// @param url Relative URL of help page to show.
  void showHelpPage(const QString &url = QString());

  /// @param page Wiki page to show help for
  void showWikiPage(const QString &page = QString());

  /// @param name of algorithm to show help for
  /// @param version of algorithm
  void showAlgorithmHelp(const QString &name, const int version = -1);

  /// @param name of concept to show help for
  void showConceptHelp(const QString &name);

  /// @param name of fit function to show help for
  void showFitFunctionHelp(const QString &name = QString());

  /// @param name of interface to show help for
  void showCustomInterfaceHelp(const QString &name);

  /// @param url of web page to open in browser
  void showWebPage(const QString &url);

  void closeHelpWindow();

  /**
   * Registration function for the help window factory.
   * @param factory the factory instance
   */
  static void registerHelpWindowFactory(
      Mantid::Kernel::AbstractInstantiator<MantidHelpInterface> *factory);

  /// The keys associated with UserSubWindow classes
  QStringList getUserSubWindowKeys() const;

  /// Getter for vates libraries availablity
  static bool hasVatesLibraries();

  /// Constructor
  InterfaceManager();
  /// Destructor
  virtual ~InterfaceManager();

private:
  void notifyExistingInterfaces(UserSubWindow *newWindow);

  /// Handle to the Vates simple user interface factory
  static Mantid::Kernel::AbstractInstantiator<VatesViewerInterface>
      *m_vatesGuiFactory;
  /// Handle to the help window factory
  static Mantid::Kernel::AbstractInstantiator<MantidHelpInterface>
      *m_helpViewer;
};
} // namespace API
} // namespace MantidQt

/*
 * Used to register Vates GUI
 */
#define REGISTER_VATESGUI(TYPE)                                                \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper register_vatesgui(                        \
      ((MantidQt::API::InterfaceManager::registerVatesGuiFactory(              \
           new Mantid::Kernel::Instantiator<TYPE, VatesViewerInterface>())),   \
       0));                                                                    \
  }

/// Used to register help window
#define REGISTER_HELPWINDOW(TYPE)                                              \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper register_helpviewer(                      \
      ((MantidQt::API::InterfaceManager::registerHelpWindowFactory(            \
           new Mantid::Kernel::Instantiator<TYPE, MantidHelpInterface>())),    \
       0));                                                                    \
  }
#endif // MANTIDQT_API_DIALOGMANAGER
