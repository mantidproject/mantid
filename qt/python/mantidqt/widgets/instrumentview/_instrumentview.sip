%ModuleCode
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
// Allows suppression of namespaces within the module
using namespace MantidQt::MantidWidgets;
%End

%InitialisationCode
qRegisterMetaType<std::string>("StdString");
qRegisterMetaType<Mantid::API::Workspace_sptr>("Workspace");
%End

//----------------------------------------------------------------------------
// Classes
// ----------------------------------------------------------------------------

class InstrumentWidget : QWidget {
%TypeHeaderCode
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
%End
public:
  InstrumentWidget(const QString &ws_name, QWidget *parent /TransferThis/ = nullptr,
                   bool reset_geometry = true, bool autoscaling = true,
                   double scale_min = 0.0, double scale_max = 0.0,
                   bool set_default_view = true);
  bool overlay(const QString & ws_name);
};

