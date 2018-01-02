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
