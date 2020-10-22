%ModuleCode
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
// Allows suppression of namespaces within the module
using namespace MantidQt::MantidWidgets;
%End

%Include ../../../common_sip/exceptions.sip
%Include ../../../common_sip/string.sip

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
#include "MantidPythonInterface/core/ExtractSharedPtr.h"
%End
public:
  InstrumentWidget(const QString &ws_name, QWidget *parent /TransferThis/ = 0,
                   bool reset_geometry = true, bool autoscaling = true,
                   double scale_min = 0.0, double scale_max = 0.0,
                   bool set_default_view = true) throw (std::runtime_error);
  bool overlay(const QString & ws_name);
  void replaceWorkspace(const std::string &wsName, SIP_PYOBJECT, const std::string &newInstrumentWindowName);
  %MethodCode
  try {
    using Mantid::PythonInterface::ExtractSharedPtr;
    sipCpp->replaceWorkspace(*a0, ExtractSharedPtr<Mantid::API::Workspace>(a1)(), *a2);
  } catch(const std::exception & exc) {
      SIP_BLOCK_THREADS
      PyErr_SetString(PyExc_RuntimeError, exc.what());
      SIP_UNBLOCK_THREADS
      return NULL;
    } catch(...) {
      sipRaiseUnknownException();
      return NULL;
    }
  %End
};

class InstrumentWidgetEncoder {
%TypeHeaderCode
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetEncoder.h"
%End
public:
  InstrumentWidgetEncoder();
  QMap<QString, QVariant> encode(const InstrumentWidget &obj,
                                 const QString &projectPath,
                                 const bool saveMask = true) /ReleaseGIL/;
};

class InstrumentWidgetDecoder : QObject{
%TypeHeaderCode
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetDecoder.h"
%End
public:
  InstrumentWidgetDecoder();
  void decode(const QMap<QString, QVariant> &map,
              InstrumentWidget &obj,
              const QString &projectPath,
              const bool loadMask = true) /ReleaseGIL/;
};
