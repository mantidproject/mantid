%ModuleCode
#include "MantidQtWidgets/Common/Message.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidQtWidgets/Common/AlgorithmDialog.h"
// Allows suppression of namespaces within the module
using namespace MantidQt::MantidWidgets;
using namespace MantidQt::API;
using namespace Mantid::API;
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
  // Not constructible
  Configurable();
  Configurable(const Configurable&);
};

class MessageDisplay : QWidget, Configurable {
%TypeHeaderCode
#include "MantidQtWidgets/Common/MessageDisplay.h"
%End

public:
  void attachLoggingChannel();
};

class AlgorithmDialog: QDialog {
%TypeHeaderCode
#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "MantidAPI/AlgorithmObserver.h"
#include <boost/python/extract.hpp>
%End

public:

  /// Set if the keep open option is shown.
  void setShowKeepOpen(const bool showOption);

  /// Set if the keep open option is shown.
  bool isShowKeepOpen() const;

  /// Create the layout of the widget. Can only be called once.
  void initializeLayout();

  /// Is this dialog initialized
  bool isInitialized() const;

  /// Set the algorithm associated with this dialog
  //void setAlgorithm(Mantid::API::IAlgorithm_sptr);
  void setAlgorithm(SIP_PYOBJECT);
%MethodCode
  sipCpp->setAlgorithm(boost::python::extract<Mantid::API::IAlgorithm_sptr>(a0));
%End
  /// Set a list of suggested values
  void setPresetValues(const QHash<QString, QString> &presetValues);
  /// Set whether this is intended for use from a script or not
  void isForScript(bool forScript);
  /// If true then execute the algorithm on acceptance
  void executeOnAccept(bool on);
  /// Set an optional message to be displayed at the top of the dialog
  void setOptionalMessage(const QString &message);
  /// Set comma-separated-list of enabled parameter names
  void addEnabledAndDisableLists(const QStringList &enabled,
                                 const QStringList &disabled);
  /// Add an AlgorithmObserver to the algorithm
  void addAlgorithmObserver(SIP_PYOBJECT);
%MethodCode
  sipCpp->addAlgorithmObserver(boost::python::extract<Mantid::API::AlgorithmObserver*>(a0));
%End
private:
  /// DefaultConstructor
  AlgorithmDialog();
  AlgorithmDialog(const AlgorithmDialog&);
};
