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
  InstrumentWidget(const QString &wsName, QWidget *parent /TransferThis/ = nullptr,
                   bool resetGeometry = true, bool autoscaling = true,
                   double scaleMin = 0.0, double scaleMax = 0.0,
                   bool setDefaultView = true);
};

