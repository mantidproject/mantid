#pragma once

//----------------------------------
// Includes
//----------------------------------
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
 * This class is responsible for managing algorithm dialogs and interface windows.
 * It also provides a mechanism for registering help window factories.
 *
 * @author Martyn Gigg
 * @date 24/02/2009
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

  /// Open a web page by URL
  void showWebPage(const QString &url);

  /// Close the active help window
  void closeHelpWindow();

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
                               new Mantid::Kernel::Instantiator<TYPE, MantidQt::API::MantidHelpInterface>())),         \
                           0));                                                                                        \
  }
