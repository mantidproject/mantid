%ModuleCode
#include "MantidQtWidgets/Common/Message.h"
using namespace MantidQt::MantidWidgets;
%End

%InitialisationCode
qRegisterMetaType<MantidQt::MantidWidgets::Message>();
%End

class MessageDisplay : QWidget {
%TypeHeaderCode
#include "MantidQtWidgets/Common/MessageDisplay.h"
%End

public:
  void attachLoggingChannel();
};
