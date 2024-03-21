// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
// This file defines the Python bindings for the
// _instrumentview library.
%Module(name=_instrumentviewqt5)

%UnitCode
#if defined(_MSC_VER)
  # pragma warning( disable: 4290 )
#elif defined(__GNUC__) && !defined(__clang__)
  // We need default visibility for this module so that
  // python can see the init function.
  #pragma GCC visibility push(default)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wpedantic"
  #if __GNUC__ >= 5
    #pragma GCC diagnostic ignored "-Wsuggest-override"
  #endif
#elif defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif
%End

// PyQt5 widgets
%Import QtWidgets/QtWidgetsmod.sip

%ModuleCode
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
#include "MantidKernel/WarningSuppressions.h"

#if defined(__GNUC__) && !defined(__clang__)
  // We need default visibility for this module so that
  // python can see the init function.
  #pragma GCC visibility push(default)
#endif
MSVC_DIAG_OFF(4290)
GNU_DIAG_OFF("pedantic")
GNU_DIAG_OFF("suggest-override")
GNU_DIAG_OFF("missing-field-initializers")

// Allows suppression of namespaces within the module
using namespace MantidQt::MantidWidgets;
%End

%Include ../../../sip/exceptions.sip
%Include ../../../sip/string.sip

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
  Overlay
%End

  int getCurrentTab() const;
%Docstring
  Get the index of the current tab.
  It can be used with getTab(tab)

  Returns:
       an integer as the index of current tab
%End


  InstrumentWidgetTab * getTab(const QString & title) const;
%Docstring
    Returns a handler to the requested tab by tab name/title

    Args:

        title The full title of a tab in the window

    Returns:
        a pointer to the requested tab widget

%End

  // get tab (2)
  InstrumentWidgetTab * getTab(const Tab tab) const;
%Docstring
    Returns a handler to the requested tab by tab index

    Args:
        tab One of the Tab enumeration types:
	    InstrumentWidget.RENDER,InstrumentWidget.PICK,
	    InstrumentWidget.MASK,InstrumentWidget.TREE

    Returns:
        a pointer to the requested tab widget

%End

  // get render tab
  InstrumentWidgetRenderTab *getRenderTab(const Tab tab) const;
%Docstring
  InstrumentWidgetRenderTab *getRenderTab(const Tab tab) const
  ------------------------------------------------------------
      Get the handler to the 'RENDER' tab inside instrument view
      if the given tab is a Render tag

      Args:
        tab: One of the Tab enumeration types

      Returns:
          a pointer to the InstrumentWidgetRenderTab object.

%End


  // get pick tab
  InstrumentWidgetPickTab *getPickTab(const Tab tab) const;
%Docstring
  InstrumentWidgetRenderTab *getPickTab(const Tab tab) const
  ----------------------------------------------------------
     Get the handler to the 'PICK' tab inside instrument view
     if the given tab is a pick tag

    Args:
       tab: One of the Tab enumeration types

     Returns:
         a pointer to the InstrumentWidgetPickTab object.

%End

  // select tab
  void selectTab(Tab tab);
%Docstring
  void selectTab(Tab tab)
  ---------------------------------------------------------
   Select tab

   Args:
     tab: One of the Tab enumeration types

   Returns:
       a pointer to the InstrumentWidgetPickTab object.

%End

  void setBinRange(double min_value, double max_value);
%Docstring
    void setBinRange(double min_value, double max_value)
    ----------------------------------------------------
    Updates the integration range over which the colours
    are calculated

    Args:
        min_value The minimum value over which the data is integrated
        max_value The maximum value over which the data is integrated
%End

  void replaceWorkspace(const std::string &wsName, const std::string &newInstrumentWindowName);
%Docstring
  void replaceWorkspace(const std::string &wsName, const std::string &newInstrumentWindowName)
  --------------------------------------------------------------------------------------------
  Replace the workspace being shown by the instrument viewer.

  Args:
      wsName The name of the workspace to show
      newInstrumentWindowName The new title of the window
%End

  bool isThreadRunning();
%Docstring
  bool isThreadRunning()
  ----------------------
  Return whether the instrument loading thread is still executing.
%End

  void waitForThread() /ReleaseGIL/;
%Docstring
  void waitForThread()
  --------------------
  Blocks until the instrument loading thread is finished.
%End

  void saveImage(QString filename);
%Docstring
  void saveImage(QString filename)
  --------------------------------
  Saves the current scene as a png image to a file.
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
    void setSurfaceType(int index)
    ------------------------------
    Set the surface type of the current window.

    Args:
      index: index of the type of known surface type (0-7), including
             0: FULL3D
             1: CYLINDRICAL_X, 2: CYLINDRICAL_Y, 3: CYLINDRICAL_Z,
             4: SPHERICAL_X,   5: SPHERICAL_Y,   6: SPHERICAL_Z,
             7: SideBySide
%End

  // set maintain aspect ratio
  void setMaintainAspectRatio(bool on);
%Docstring
    void setMaintainAspectRatio(bool on)
    ------------------------------
    Set the maintain aspect ratio option in the current window.

    Args:
      on: boolean indicating whether to turn maintain aspect ratio on or off
%End

  void setMinValue(double value, bool apply);
%Docstring
    void setMinValue(double value, bool apply)
    ----------------------------------------------------
    Set minimum value on the colormap scale.

    Args:
        value: minimum value for the color bar
        apply: flag to apply the change other than color bar by sending singal out.
%End

  void setMaxValue(double value, bool apply);
%Docstring
    void setMaxValue(double value, bool apply)
    ----------------------------------------------------
    Set maximum value on the colormap scale.

    Args:
        value: maximum value for the color bar
        apply: flag to apply the change other than color bar by sending singal out.
%End

  void setRange(double minValue, double maxValue, bool apply);
%Docstring
    void setRange(double minValue, double maxValue, bool apply)
    -----------------------------------------------------------
    Set minimum and maximum values on the colormap scale.

    Args:
        minValue: minimum value for the color bar
        maxValue: maximum value for the color bar
        apply: flag to apply the change other than color bar by sending singal out.
%End

  void setLegendScaleType(int index);
%Docstring
    void setLegendScaleType(int index)
    ----------------------------------

    Args:
      index: valid option including [0, 1, 2] where
          0: Linear
          1: SymmetricLog10
          2: Power
%End

  // set Axis at Full 3D
  void setAxis(const QString &axisNameArg);
%Docstring
    void setAxis(const QString &axisNameArg)
    ----------------------------------------------------
    Set axis to plot in FULL3D surface type

    Args:
       axisNameAr: name of the Axis to plot against including (Z+, Z-, Y+, Y-, X+, X-)
%End

  // Enable/disable auto scaling checkbox
  void setColorMapAutoscaling(bool on);
%Docstring
    setColorMapAutoscaling(bool on)
    ----------------------------------------------------
    Enable or disable autoscaling option for color map

    Args:
       on: turn on or off autoscaling
%End

  void resetView();
%Docstring
    void resetView()
    ----------------
    Reset the view widget
%End

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
