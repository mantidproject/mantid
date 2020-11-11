%ModuleCode
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
// Allows suppression of namespaces within the module
using namespace MantidQt::MantidWidgets;
%End

%Include ../../../common_sip/exceptions.sip

%InitialisationCode
qRegisterMetaType<std::string>("StdString");
qRegisterMetaType<Mantid::API::Workspace_sptr>("Workspace");
%End

//----------------------------------------------------------------------------
// Classes
// ---------------------------------------------------------------------------


//------------------------------------------------
// InstrumentWidgetTab
//-------------------------------------------------
class InstrumentWidgetTab: QFrame
{
%TypeHeaderCode
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetTab.h"
%End
public:
	// This is a duplicate from the same enum in InstrumentWidget, just so you
	// can do like InstrumentWidgetRenderTab.FULL3D
	enum SurfaceType { FULL3D, CYLINDRICAL_X, CYLINDRICAL_Y, CYLINDRICAL_Z,
                     SPHERICAL_X, SPHERICAL_Y, SPHERICAL_Z,
                     RENDERMODE_SIZE };
private:
  InstrumentWidgetTab(const InstrumentWidgetTab &);
};


//---------------------------------------------------
// InstrumentWidget
//---------------------------------------------------
class InstrumentWidget : QWidget {
%TypeHeaderCode
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
%End

public:

  // Note this enum is also defined in InstrumentWidgetTab
  enum SurfaceType { FULL3D, CYLINDRICAL_X, CYLINDRICAL_Y, CYLINDRICAL_Z,
                     SPHERICAL_X, SPHERICAL_Y, SPHERICAL_Z,
                     RENDERMODE_SIZE };
  enum Tab { RENDER, PICK, MASK, TREE };


  // constructor
  InstrumentWidget(const QString &ws_name, QWidget *parent /TransferThis/ = 0,
                   bool reset_geometry = true, bool autoscaling = true,
                   double scale_min = 0.0, double scale_max = 0.0,
                   bool set_default_view = true) throw (std::runtime_error);


  bool overlay(const QString & ws_name);
%Docstring
  Overlay ... whatever
%End

  int getCurrentTab() const;
%Docstring
  Get the index of the current tab.
  It can be used with getTab(tab)

  Returns:
       an integer as the index of current tab
%End


  // get tab (1)
  InstrumentWidgetTab * getTab(const QString & title) const;
%Docstring
    Returns a handler to the requested tab

    Args:

        title The full title of a tab in the window

    Returns:
        a pointer to the requested tab widget

%End

  // get tab (2)
  InstrumentWidgetTab * getTab(const Tab tab) const;
%Docstring
    Returns a handler to the requested tab

    Args:
        tab One of the Tab enumeration types:
	    InstrumentWidget.RENDER,InstrumentWidget.PICK,
	    InstrumentWidget.MASK,InstrumentWidget.TREE

    Returns:
        a pointer to the requested tab widget

%End

  // get render tab
  InstrumentWidgetRenderTab *getRenderTab(const Tab tab) const;

  // get pick tab
  InstrumentWidgetPickTab *getPickTab(const Tab tab) const;

  // select tab
  void selectTab(Tab tab);


  void setBinRange(double min_value, double max_value);
%Docstring
    Updates the integration range over which the colours
    are calculated

    Args:
        min_value The minimum value over which the data is integrated
        max_value The maximum value over which the data is integrated
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


// Render Tab
class InstrumentWidgetRenderTab: InstrumentWidgetTab
{
%TypeHeaderCode
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetRenderTab.h"
%End

public:

  // set surface type
  void setSurfaceType(int index);
%Docstring
    Set the surface type of the current window.

    Args:
        type A known suface type: FULL3D, CYLINDRICAL_X, CYLINDRICAL_Y,
                                  CYLINDRICAL_Z,SPHERICAL_X, SPHERICAL_Y,
                                  SPHERICAL_Z
%End

  // set min value
  void setMinValue(double value, bool apply);
  // set max value
  void setMaxValue(double value, bool apply);
  // set Axis at Full 3D
  void setAxis(const QString &axisNameArg);
  // Enable/disable auto scaling checkbox
  void setColorMapAutoscaling(bool on);

private:
  // constructors all private
  InstrumentWidgetRenderTab();
  InstrumentWidgetRenderTab(const InstrumentWidgetRenderTab &);

};

// Pick tab
class InstrumentWidgetPickTab: InstrumentWidgetTab
{
    %TypeHeaderCode
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h"
    %End

    private:
        // constructors all private
        InstrumentWidgetPickTab();
        InstrumentWidgetPickTab(const InstrumentWidgetPickTab &);

};
