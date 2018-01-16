%ModuleCode
#include "MantidQtWidgets/Common/Message.h"
// Allows suppression of namespaces within the module
using namespace MantidQt::MantidWidgets;
%End

%InitialisationCode
qRegisterMetaType<MantidQt::MantidWidgets::Message>();
%End

// ----------------------------------------------------------------------------
// Classes
// ----------------------------------------------------------------------------

class Configurable {
%TypeHeaderCode
#include "MantidQtWidgets/Common/Configurable.h"
%End

public:
  void readSettings(const QSettings &storage);
  void writeSettings(QSettings *storage);

private:
  // Not constructible or copyable
  Configurable();
  Configurable(const Configurable&);
};

class MessageDisplay : QWidget, Configurable {
%TypeHeaderCode
#include "MantidQtWidgets/Common/MessageDisplay.h"
%End

public:
  MessageDisplay(QWidget *parent = 0);
  void attachLoggingChannel();
};


class AlgorithmDialog: QDialog {
%TypeHeaderCode
#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "MantidAPI/AlgorithmObserver.h"
#include <boost/python/extract.hpp>
using namespace MantidQt::API;
%End
public:
  void setShowKeepOpen(const bool showOption);
  bool isShowKeepOpen() const;
  void initializeLayout();
  bool isInitialized() const;
  void setAlgorithm(SIP_PYOBJECT);
%MethodCode
  sipCpp->setAlgorithm(boost::python::extract<Mantid::API::IAlgorithm_sptr>(a0));
%End
  void setPresetValues(const QHash<QString, QString> &presetValues);
  void isForScript(bool forScript);
  void executeOnAccept(bool on);
  void setOptionalMessage(const QString &message);
  void addEnabledAndDisableLists(const QStringList &enabled,
                                 const QStringList &disabled);
  void addAlgorithmObserver(SIP_PYOBJECT);
%MethodCode
  sipCpp->addAlgorithmObserver(boost::python::extract<Mantid::API::AlgorithmObserver*>(a0));
%End
private:
  AlgorithmDialog();
  AlgorithmDialog(const AlgorithmDialog&);
};


class GenericDialog: AlgorithmDialog {
%TypeHeaderCode
#include "MantidQtWidgets/Common/GenericDialog.h"
using namespace MantidQt::API;
%End
public:
  GenericDialog(QWidget *parent = nullptr);
};


class InterfaceManager {
%TypeHeaderCode
#include "MantidQtWidgets/Common/InterfaceManager.h"
using namespace MantidQt::API;
%End
public:
  InterfaceManager();
  AlgorithmDialog *createDialogFromName(
      const QString &algorithmName, const int version = -1,
      QWidget *parent = nullptr, bool forScript = false,
      const QHash<QString, QString> &presetValues = QHash<QString, QString>(),
      const QString &optionalMsg = QString(),
      const QStringList &enabled = QStringList(),
      const QStringList &disabled = QStringList());
};

// ---------------------------------
// WorkspaceWidget and dependencies
// ---------------------------------



class MantidDisplayBase /Abstract/ {
%TypeHeaderCode
#include "MantidQtWidgets/Common/MantidDisplayBase.h"
%End
private:
  MantidDisplayBase();
};

class MantidTreeModel : MantidDisplayBase {
%TypeHeaderCode
#include "MantidQtWidgets/Common/MantidTreeModel.h"
%End
public:
  MantidTreeModel();
private:
  MantidTreeModel(const MantidTreeModel &);
};

class WorkspaceTreeWidget : QWidget {
%TypeHeaderCode
#include "MantidQtWidgets/Common/WorkspacePresenter/WorkspaceTreeWidget.h"
%End
public:
  WorkspaceTreeWidget(MantidDisplayBase *mdb /Transfer/,
                      QWidget *parent /TransferThis/ = nullptr);
};

// Implementation to override context menu
class WorkspaceTreeWidgetSimple : WorkspaceTreeWidget {
%TypeHeaderCode
#include "MantidQtWidgets/Common/WorkspacePresenter/WorkspaceTreeWidgetSimple.h"
%End
public:
  WorkspaceTreeWidgetSimple(MantidDisplayBase *mdb /Transfer/,
                            QWidget *parent /TransferThis/ = nullptr);
};
