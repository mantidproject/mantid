#include <cmath>
#include <limits>
#include <stdexcept>

#include "MantidKernel/Logger.h"

#include "MantidVatesSimpleGuiViewWidgets/ColorUpdater.h"
#include "MantidVatesSimpleGuiViewWidgets/ColorSelectionWidget.h"
#include "MantidVatesSimpleGuiViewWidgets/AutoScaleRangeGenerator.h"
#include "MantidVatesAPI/ColorScaleGuard.h"

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqDataRepresentation.h>
#include <pqPipelineRepresentation.h>
#include <pqScalarsToColors.h>
#include <pqServerManagerModel.h>
#include <pqSMAdaptor.h>

#include "vtk_jsoncpp.h"
#include <vtkCallbackCommand.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMProxy.h>
#include <vtkSMTransferFunctionProxy.h>

#include <QColor>
#include <QList>

namespace Mantid {
// static logger
Kernel::Logger g_log("ColorUpdater");

namespace Vates {
namespace SimpleGui {

ColorUpdater::ColorUpdater()
    : m_autoScaleState(true), m_logScaleState(false),
      m_minScale(std::numeric_limits<double>::min()),
      m_maxScale(std::numeric_limits<double>::max()) {}

ColorUpdater::~ColorUpdater() {}

/**
 * Set the lookup table to the autoscale values.
 * @returns A struct which contains the parameters of the looup table.
 */
VsiColorScale ColorUpdater::autoScale() {
  // Get the custom auto scale.
  VsiColorScale vsiColorScale = this->m_autoScaleRangeGenerator.getColorScale();

  // Set the color scale for all sources
  this->m_minScale = vsiColorScale.minValue;
  this->m_maxScale = vsiColorScale.maxValue;

  // If the view

  this->m_logScaleState = vsiColorScale.useLogScale;

  // Update the lookup tables, i.e. react to a color scale change
  colorScaleChange(this->m_minScale, this->m_maxScale);

  return vsiColorScale;
}

void ColorUpdater::colorMapChange(pqPipelineRepresentation *repr,
                                  const Json::Value &model) {
  pqScalarsToColors *lut = repr->getLookupTable();
  if (!lut) {
    // Got a bad proxy, so just return
    return;
  }
  vtkSMProxy *lutProxy = lut->getProxy();
  vtkSMTransferFunctionProxy::ApplyPreset(lutProxy, model, true);
}

/**
 * React to a change of the color scale settings.
 * @param min The lower end of the color scale.
 * @param max The upper end of the color scale.
 */
void ColorUpdater::colorScaleChange(double min, double max) {
  if (min >= max)
    return;

  this->m_minScale = min;
  this->m_maxScale = max;

  try {
    // Update for all sources and all reps
    pqServer *server = pqActiveObjects::instance().activeServer();
    pqServerManagerModel *smModel =
        pqApplicationCore::instance()->getServerManagerModel();
    const QList<pqPipelineSource *> sources =
        smModel->findItems<pqPipelineSource *>(server);

    // For all sources
    foreach (pqPipelineSource *source, sources) {
      const QList<pqView *> views = source->getViews();
      // For all views
      foreach (pqView *view, views) {
        const QList<pqDataRepresentation *> reps =
            source->getRepresentations(view);
        // For all representations
        foreach (pqDataRepresentation *rep, reps) {
          this->updateLookupTable(rep);
        }
      }
    }
  } catch (std::invalid_argument &) {
    return;
  }
}

/**
 * Update the lookup table.
 * @param representation The representation for which the lookup table is
 * updated.
 */
void ColorUpdater::updateLookupTable(pqDataRepresentation *representation) {
  pqScalarsToColors *lookupTable = representation->getLookupTable();

  if (lookupTable) {
    // Set the scalar range values
    lookupTable->setScalarRange(this->m_minScale, this->m_maxScale);

    vtkSMProxy *proxy = representation->getProxy();
    vtkSMProxy *lutProxy =
        pqSMAdaptor::getProxyProperty(proxy->GetProperty("LookupTable"));
    vtkSMProxy *scalarOpacityFunctionProxy =
        lutProxy ? pqSMAdaptor::getProxyProperty(
                       lutProxy->GetProperty("ScalarOpacityFunction"))
                 : nullptr;

    if (scalarOpacityFunctionProxy) {
      vtkSMTransferFunctionProxy::RescaleTransferFunction(
          scalarOpacityFunctionProxy, this->m_minScale, this->m_maxScale);
    }

    // Need to set a lookup table lock here. This does not affect
    // setScalarRange,
    // but blocks setWholeScalarRange which gets called by ParaView overrides
    // our
    // setting when a workspace is loaded for the first time.
    lookupTable->setScalarRangeLock(true);

    representation->getProxy()->UpdateVTKObjects();
    representation->renderViewEventually();
  } else {
    throw std::invalid_argument("Cannot get LUT for representation");
  }
}

/**
 * React to changing the log scale option
 * @param state The state to which the log scale is being changed.
 */
void ColorUpdater::logScale(int state) {
  this->m_logScaleState = state;

  // Update for all sources and all reps
  pqServer *server = pqActiveObjects::instance().activeServer();
  pqServerManagerModel *smModel =
      pqApplicationCore::instance()->getServerManagerModel();
  const QList<pqPipelineSource *> sources =
      smModel->findItems<pqPipelineSource *>(server);

  // For all sources
  foreach (pqPipelineSource *source, sources) {
    const QList<pqView *> views = source->getViews();
    // For all views
    foreach (pqView *view, views) {
      const QList<pqDataRepresentation *> reps =
          source->getRepresentations(view);
      // For all representations
      foreach (pqDataRepresentation *rep, reps) {
        // Set the logarithmic (linear) scale
        auto lut = rep->getLookupTable();
        if (lut) {
          pqSMAdaptor::setElementProperty(
              rep->getLookupTable()->getProxy()->GetProperty("UseLogScale"),
              this->m_logScaleState);
          if (m_logScaleState) {
            vtkSMTransferFunctionProxy::MapControlPointsToLogSpace(
                rep->getLookupTable()->getProxy());
          } else {
            vtkSMTransferFunctionProxy::MapControlPointsToLinearSpace(
                rep->getLookupTable()->getProxy());
          }
        }
      }
    }
  }

  this->colorScaleChange(this->m_minScale, this->m_maxScale);
}

/**
 * This function takes information from the color selection widget and
 * sets it into the internal state variables.
 * @param cs : Reference to the color selection widget
 */
void ColorUpdater::updateState(ColorSelectionWidget *cs) {
  this->m_autoScaleState = cs->getAutoScaleState();
  this->m_logScaleState = cs->getLogScaleState();
  this->m_minScale = cs->getMinRange();
  this->m_maxScale = cs->getMaxRange();
  this->m_autoScaleRangeGenerator.updateLogScaleSetting(this->m_logScaleState);
}

/**
 * @return  the current auto scaling state
 */
bool ColorUpdater::isAutoScale() { return this->m_autoScaleState; }

/**
 * @return the current logarithmic scaling state
 */
bool ColorUpdater::isLogScale() { return this->m_logScaleState; }

/**
 * @return the current maximum range for the color scaling
 */
double ColorUpdater::getMaximumRange() { return this->m_maxScale; }

/**
 * @return the current minimum range for the color scaling
 */
double ColorUpdater::getMinimumRange() { return this->m_minScale; }

/**
 * This function prints out the values of the current state of the
 * color updater.
 */
void ColorUpdater::print() {
  std::cout << "Auto Scale: " << this->m_autoScaleState << '\n';
  std::cout << "Log Scale: " << this->m_logScaleState << '\n';
  std::cout << "Min Range: " << this->m_minScale << '\n';
  std::cout << "Max Range: " << this->m_maxScale << '\n';
}

/**
 * Initializes the color scale
 */
void ColorUpdater::initializeColorScale() {
  m_autoScaleRangeGenerator.initializeColorScale();
}

/**
 * Data that has to be passed to the callback defined below for when
 * the user edits the color range in the Paraview color map editor,
 * which needs to notify/modify the VSI simple color selection widget
 * (update the min/max values).
 */
struct ColorCallbackData {
  ColorUpdater *colorUpdater;
  ColorSelectionWidget *csel;
};
ColorCallbackData ccdata;

/**
 * Observe the vtkCommand::ModifiedEvent for the proxy property
 * 'RGBPoints' of a pqColorToolbar. This method installs a VTK
 * callback for that property (for when the user edits the color scale
 * interactively in the ParaQ color map editor).
 *
 * @param repr Paraview representation whose color editor we have to observe
 * @param cs The simple color selection widget (the VSI one, not the ParaQ one)
 */
void ColorUpdater::observeColorScaleEdited(pqPipelineRepresentation *repr,
                                           ColorSelectionWidget *cs) {
  if (!repr)
    return;

  pqScalarsToColors *lut = repr->getLookupTable();
  if (!lut)
    return;

  vtkSMProxy *lutProxy = lut->getProxy();
  if (!lutProxy)
    return;

  // User updates the color scale (normally the range)
  // Prepare the callback. Vtk callbacks:
  // http://www.vtk.org/Wiki/VTK/Tutorials/Callbacks
  vtkSmartPointer<vtkCallbackCommand> CRChangeCallback =
      vtkSmartPointer<vtkCallbackCommand>::New();
  CRChangeCallback->SetCallback(colorScaleEditedCallbackFunc);
  // note this uses the same ccdata which would misbehave if we wanted multiple
  // VSI instances
  ccdata.colorUpdater = this;
  ccdata.csel = cs;
  CRChangeCallback->SetClientData(&ccdata);
  // install callback
  vtkSMDoubleVectorProperty *points = vtkSMDoubleVectorProperty::SafeDownCast(
      lutProxy->GetProperty("RGBPoints"));

  if (points) {
    points->AddObserver(vtkCommand::ModifiedEvent, CRChangeCallback);
  }

  // User clicks on the log-scale tick box
  vtkSmartPointer<vtkCallbackCommand> LogScaleCallback =
      vtkSmartPointer<vtkCallbackCommand>::New();
  LogScaleCallback->SetCallback(logScaleClickedCallbackFunc);
  LogScaleCallback->SetClientData(&ccdata);
  vtkSMIntVectorProperty *logScaleProp = vtkSMIntVectorProperty::SafeDownCast(
      lutProxy->GetProperty("UseLogScale"));
  if (logScaleProp)
    logScaleProp->AddObserver(vtkCommand::ModifiedEvent, LogScaleCallback);
}

/**
 * Callback/hook that runs every time the user edits the color map (in
 * the Paraview color map/scale editor). This callback must be
 * attached to the RGBPoints property of the Proxy for the color
 * lookup table. Note that this goes in the opposite direction than
 * most if not all of the updates made by the ColorUpdater.
 *
 * @param caller the proxy object that calls this (or the callback has been set
 *to it with AddObserver
 * @param eventID vtkCommand event ID for callbacks, not used here
 *
 * @param clientData expects a ColorCallBackData struct that has the
 *ColorUpdater object which set
 * the callback, and ColorSelectionWidget object of this VSI window. Never use
 *this method with
 * different data.
 *
 * @param callData callback specific data which takes different forms
 * depending on events, not used here.
 */
void ColorUpdater::colorScaleEditedCallbackFunc(vtkObject *caller,
                                                long unsigned int eventID,
                                                void *clientData,
                                                void *callData) {
  UNUSED_ARG(eventID);
  UNUSED_ARG(callData);

  // this won't help much. You must make sure that clientData is a proper
  // ColorCallBackData struct
  ColorCallbackData *data = static_cast<ColorCallbackData *>(clientData);
  if (!data) {
    return;
  }

  // a pseudo-this
  ColorUpdater *pThis = data->colorUpdater;
  if (!pThis) {
    return;
  }

  ColorSelectionWidget *csel = data->csel;
  if (!csel) {
    return;
  }

  // This means that either
  //
  // A) the user clicked on the auto-scale check box of the
  // ColorSelectionWidget.
  // That will change color properties of the ParaQ and trigger this callback.
  // This condition
  // prevents the callback from ruining the state of the ColorSelectionWidget
  // (which is just
  // being set by the user and we do not want to update programmatically in this
  // case).
  //
  // B) We are in the middle of an operation where we don't want callbacks to
  // update the
  // color map (for example when switching views or doing several view updates
  // in a row)
  if (csel->inProcessUserRequestedAutoScale() ||
      csel->isIgnoringColorCallbacks() || csel->isColorScaleLocked()) {
    return;
  }

  // This vector has 4 values per color definition: data value (bin limit) + 3
  // R-G-B coordinates
  vtkSMDoubleVectorProperty *RGBPoints =
      vtkSMDoubleVectorProperty::SafeDownCast(caller);
  if (!RGBPoints)
    return;

  double *elems = RGBPoints->GetElements();
  int noe = RGBPoints->GetNumberOfElements();

  // there should be at least one data value/bin + one triplet of R-G-B values
  const int subtract = 4;
  if (noe < subtract)
    return;

  double newMin = elems[0];
  double newMax = elems[noe - subtract];

  if ((std::fabs(newMin - csel->getMinRange()) > 1e-14) ||
      (std::fabs(csel->getMaxRange() - newMax) > 1e-14)) {
    pThis->m_minScale = newMin;
    pThis->m_maxScale = newMax;
    csel->setMinMax(pThis->m_minScale, pThis->m_maxScale);

    if (csel->getAutoScaleState()) {
      csel->setAutoScale(false);
      pThis->m_autoScaleState = csel->getAutoScaleState();
    }
  }
}

/**
 * Callback/hook for when the user changes the log scale property of
 * the color map (in the Paraview color map/scale editor). This
 * callback must be attached to the UseLogScale property of the Proxy
 * for the color lookup table. Note that this goes in the opposite
 * direction than most if not all of the updates made by the
 * ColorUpdater.
 *
 * @param caller the proxy object that calls this (or the callback has
 * been set to it with AddObserver
 *
 * @param eventID vtkCommand event ID for callbacks, not used here
 *
 * @param clientData expects a ColorCallBackData struct that has the
 * ColorUpdater object which set the callback, and
 * ColorSelectionWidget object of this VSI window. Never use this
 * method with different data.
 *
 * @param callData callback specific data which takes different forms
 * depending on events, not used here.
 */
void ColorUpdater::logScaleClickedCallbackFunc(vtkObject *caller,
                                               long unsigned int eventID,
                                               void *clientData,
                                               void *callData) {
  UNUSED_ARG(eventID);
  UNUSED_ARG(callData);

  // this won't help much if you pass a wrong type.
  ColorCallbackData *data = static_cast<ColorCallbackData *>(clientData);
  if (!data) {
    return;
  }

  // pseudo-this
  ColorUpdater *pThis = data->colorUpdater;
  if (!pThis) {
    return;
  }

  ColorSelectionWidget *csel = data->csel;
  if (!csel) {
    return;
  }

  // A single element int vector which actually contains a 0/1 value
  vtkSMIntVectorProperty *useLogProp =
      vtkSMIntVectorProperty::SafeDownCast(caller);
  if (!useLogProp)
    return;

  int *elems = useLogProp->GetElements();
  int noe = useLogProp->GetNumberOfElements();
  if (noe < 1)
    return;

  int logState = elems[0];
  pThis->m_logScaleState = logState;
  csel->onSetLogScale(logState);
}

} // SimpleGui
} // Vates
} // Mantid
