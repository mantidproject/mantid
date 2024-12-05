// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------
// Includes
//----------------------------------
#include <boost/python.hpp>

#include "DllOption.h"
#include "MantidKernel/Instantiator.h"

#include <QHash>
#include <QList>
#include <QPointer>
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
class MantidHelpInterface;

/**
    This class is responsible for creating the correct dialog for an algorithm.
   If no specialized version is registered for that algorithm then the default is created.

    @author Martyn Gigg, Tessella Support Services plc
    @date 24/02/2009
*/
class EXPORT_OPT_MANTIDQT_COMMON InterfaceManager {

public:
  /// Create a new instance of the correct type of AlgorithmDialog
  AlgorithmDialog *createDialog(const std::shared_ptr<Mantid::API::IAlgorithm> &alg, QWidget *parent = nullptr,
                                bool forScript = false,
                                const QHash<QString, QString> &presetValues = (QHash<QString, QString>()),
                                const QString &optional_msg = QString(), const QStringList &enabled = QStringList(),
                                const QStringList &disabled = QStringList());

  /// Create an algorithm dialog for a given name and version
  AlgorithmDialog *createDialogFromName(const QString &algorithmName, const int version = -1, QWidget *parent = nullptr,
                                        bool forScript = false,
                                        const QHash<QString, QString> &presetValues = (QHash<QString, QString>()),
                                        const QString &optionalMsg = QString(),
                                        const QStringList &enabled = QStringList(),
                                        const QStringList &disabled = QStringList());

  /// Create a new instance of the correct type of UserSubWindow
  UserSubWindow *createSubWindow(const QString &interface_name, QWidget *parent = nullptr, bool isWindow = true);

  /**
   * Function that instantiates the help window.
   * @return the help window
   */
  MantidHelpInterface *createHelpWindow() const;

  /// Show a help page by URL
  void showHelpPage(const QString &url = QString());

  /// Show algorithm help by name and version
  void showAlgorithmHelp(const QString &name, const int version = -1);

  /// Show concept help by name
  void showConceptHelp(const QString &name);

  /// Show fit function help
  void showFitFunctionHelp(const QString &name = QString());

  /**
   * @param name of interface to show help for
   * @param area - folder for documentation in the interfaces directory
   * @param section - section in the HTML document
   */
  void showCustomInterfaceHelp(const QString &name, const QString &area = QString(),
                               const QString &section = QString());

  /// Open a web page by URL
  void showWebPage(const QString &url);

  /// Close the active help window
  void closeHelpWindow();

  /**
   * Launch the Python-based HelpWindow
   * @param url :: URL to pass to the Python HelpWindow
   */
  void launchPythonHelp(const QString &url);

  /**
   * Registration function for the help window factory.
   * @param factory the factory instance
   */
  static void registerHelpWindowFactory(Mantid::Kernel::AbstractInstantiator<MantidHelpInterface> *factory);

  /// Returns a list of existing UserSubWindows
  static QList<QPointer<UserSubWindow>> &existingInterfaces();

  /// The keys associated with UserSubWindow classes
  QStringList getUserSubWindowKeys() const;

  /// Constructor
  InterfaceManager();

  /// Destructor
  virtual ~InterfaceManager();

private:
  void notifyExistingInterfaces(UserSubWindow *newWindow);

  /// Handle to the help window factory
  static Mantid::Kernel::AbstractInstantiator<MantidHelpInterface> *m_helpViewer;
};

} // namespace API
} // namespace MantidQt

/// Used to register help window
#define REGISTER_HELPWINDOW(TYPE)                                                                                      \
  namespace {                                                                                                          \
  Mantid::Kernel::RegistrationHelper                                                                                   \
      register_helpviewer(((MantidQt::API::InterfaceManager::registerHelpWindowFactory(                                \
                               new Mantid::Kernel::Instantiator<TYPE, MantidHelpInterface>())),                        \
                           0));                                                                                        \
  }
