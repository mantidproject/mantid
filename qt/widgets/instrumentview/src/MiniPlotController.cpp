#include "MantidQtWidgets/InstrumentView/MiniPlotController.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
#include "MantidQtWidgets/InstrumentView/MiniPlot.h"
#include "MantidQtWidgets/InstrumentView/PeakMarker2D.h"
#include "MantidQtWidgets/InstrumentView/ProjectionSurface.h"
#include "MantidQtWidgets/InstrumentView/UnwrappedSurface.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Unit.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QMenu>
#include <QMessageBox>
#include <QSignalMapper>
#include <QSettings>

using Mantid::API::AnalysisDataService;
using Mantid::API::AlgorithmManager;
using Mantid::API::IPeaksWorkspace;
using Mantid::API::MatrixWorkspace;
using Mantid::API::WorkspaceFactory;
using Mantid::Geometry::IDetector;
using Mantid::Geometry::OrientedLattice;
using Mantid::Kernel::Logger;

namespace {
/**
 * Assume template type is an enum and translate it to an int
 * @param value A value of type EnumType
 * @return Value as an integer
 */
template <typename EnumType> int toInt(const EnumType &value) {
  return static_cast<int>(value);
}
/**
 * Assume template type is an enum and translate it to an int
 * @param value A value of type int
 * @return Value as an EnumType
 */
template <typename EnumType> EnumType fromInt(int value) {
  return static_cast<EnumType>(value);
}

// Get the phi angle between the detector with reference to the origin
// Makes assumptions about beam direction. Legacy code and not robust.
double getPhi(const Mantid::Kernel::V3D &pos) {
  return std::atan2(pos[1], pos[0]);
}

// Calculate the phi angle between detector and beam, and then offset.
// Makes assumptions about beam direction. Legacy code and not robust.
double getPhiOffset(const Mantid::Kernel::V3D &pos, const double offset) {
  double avgPos = getPhi(pos);
  return avgPos < 0 ? -(offset + avgPos) : offset - avgPos;
}

/// Static logger
Logger g_log("Miniplot");
}

namespace MantidQt {
namespace MantidWidgets {

/**
* Constructor.
* @param instrWidget A pointer to a InstrumentWidget instance.
* @param miniplot The plot widget.
*/
MiniPlotController::MiniPlotController(InstrumentWidget *instrWidget,
                                       MiniPlot *miniplot)
    : QObject(instrWidget), m_instrWidget(instrWidget), m_miniplot(miniplot),
      m_plotType(PlotType::Single), m_enabled(true),
      m_tubeXUnits(TubeXUnits::DETECTOR_ID), m_currentDetID(-1) {
  initActions();
  connect(m_miniplot, SIGNAL(contextMenuRequested(QPoint)), this,
          SLOT(showContextMenu(QPoint)));
  connect(m_miniplot, SIGNAL(clickedAtDataCoord(double, double)), this,
          SLOT(addPeak(double, double)));
}

/**
 * Load persistent settings
 * @param settings A QSettings instance opened at the correct group
 */
void MiniPlotController::loadSettings(const QSettings &settings) {
  setTubeXUnits(fromInt<TubeXUnits>(settings.value("TubeXUnits", 0).toInt()));
  setPlotType(fromInt<PlotType>(
      settings.value("PlotType", toInt<PlotType>(PlotType::Single)).toInt()));
}

void MiniPlotController::saveSettings(QSettings &settings) const {
  settings.setValue("TubeXUnits", toInt<TubeXUnits>(m_tubeXUnits));
  settings.setValue("PlotType", toInt<PlotType>(m_plotType));
}

/**
* Update the miniplot for a selected detector. The curve data depend on the
* plot type.
* @param pickID :: A pick ID of an instrument component.
*/
void MiniPlotController::setPlotData(size_t pickID) {
  if (!m_enabled) {
    m_miniplot->removeActiveCurve();
    return;
  }
  m_currentDetID = std::numeric_limits<size_t>::max();

  if (m_plotType == PlotType::DetectorSum) {
    m_plotType = PlotType::Single;
  }

  const auto &actor = m_instrWidget->getInstrumentActor();
  const auto &componentInfo = actor.componentInfo();
  if (componentInfo.isDetector(pickID)) {
    if (m_plotType == PlotType::Single) {
      m_currentDetID = pickID;
      plotSingle(pickID);
    } else if (m_plotType == PlotType::TubeSum ||
               m_plotType == PlotType::TubeIntegral) {
      plotTube(pickID);
    } else {
      throw std::logic_error("setPlotData: Unexpected plot type.");
    }
  } else {
    m_miniplot->removeActiveCurve();
  }
}

/**
* Set curve data from multiple detectors: sum their spectra.
* @param detIDs :: A list of detector IDs.
*/
void MiniPlotController::setPlotData(std::vector<size_t> detIDs) {
  setPlotType(PlotType::DetectorSum);
  clear();
  std::vector<double> x, y;
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  const auto &actor = m_instrWidget->getInstrumentActor();
  actor.sumDetectors(detIDs, x, y);
  QApplication::restoreOverrideCursor();
  if (!x.empty()) {
    MiniPlotCurveData data{
        actor.getWorkspace()->getAxis(0)->unit()->unitID().data(),
        "multiple",
        x,
        y,
        {}};
    m_miniplot->setActiveCurve(std::move(data));
  }
}

/**
 * Updates the plot type
 * @param type The requested type for the new plot
 */
void MiniPlotController::setPlotType(MiniPlotController::PlotType type) {
  m_plotType = type;
  emit plotTypeChanged(getPlotCaption());
}

/**
* Update the miniplot for a selected detector.
*/
void MiniPlotController::updatePlot() { m_miniplot->update(); }

/**
* Clear the plot.
*/
void MiniPlotController::clear() { m_miniplot->removeActiveCurve(); }

/**
* Plot data for a detector.
* @param detid :: ID of the detector to be plotted.
*/
void MiniPlotController::plotSingle(size_t detid) {
  auto plotData = prepareDataForSinglePlot(detid);
  if (plotData.x.empty() || plotData.y.empty())
    return;

  m_miniplot->setActiveCurve(std::move(plotData));
  // find any markers
  auto surface = m_instrWidget->getSurface();
  if (surface) {
    QList<PeakMarker2D *> markers = surface->getMarkersWithID(detid);
    for (const auto marker : markers) {
      addPeakMarker(*marker);
    }
  }
}

/**
 * Add a label for a single crystal peak
 * @param marker A reference to the marker class
 */
void MiniPlotController::addPeakMarker(const PeakMarker2D &marker) {
  const auto &peak = marker.getPeak();
  addPeakMarker(peak);
}

/**
 * Add a label for a single crystal peak
 * @param peak A reference to the peak instance
 */
void MiniPlotController::addPeakMarker(const Mantid::Geometry::IPeak &peak) {
  QString xunit = m_miniplot->getXUnits();
  if (xunit.isEmpty())
    return;
  double peakX(-1.0);
  if (xunit == "dSpacing") {
    peakX = peak.getDSpacing();
  } else if (xunit == "Wavelength") {
    peakX = peak.getWavelength();
  } else {
    peakX = peak.getTOF();
  }
  double ymax(-1.0), _(-1.0);
  std::tie(_, ymax) = m_miniplot->getYLimits();
  // arbitrarily place the label at 85% of the y-axis height
  const double peakY = 0.85 * ymax;
  m_miniplot->addPeakLabel(peakX, peakY, PeakMarker2D::formatLabel(peak));
}

/**
* Plot data integrated either over the detectors in a tube or over time bins.
* If m_plotSum == true the miniplot displays the accumulated data in a tube
* against time of flight.
* If m_plotSum == false the miniplot displays the data integrated over the time
* bins. The values are
* plotted against the length of the tube, but the units on the x-axis can be one
* of the following:
*   DETECTOR_ID
*   LENGTH
*   PHI
* The units can be set with setTubeXUnits(...) method.
* @param detid :: A detector id. The miniplot will display data for a component
* containing the detector
*   with this id.
*/
void MiniPlotController::plotTube(size_t detindex) {
  const auto &actor = m_instrWidget->getInstrumentActor();
  const auto &componentInfo = actor.componentInfo();

  // if (!componentInfo.hasParent(detindex)) {
  //  m_miniplot->clear();
  //  return;
  //}

  auto parent = componentInfo.parent(detindex);
  if (componentInfo.detectorsInSubtree(parent).size() > 0) {
    if (m_plotType == PlotType::TubeSum) {
      // plot sums over detectors vs time bins
      plotTubeSums(detindex);
    } else {
      // plot detector integrals vs detID or a function of detector
      // position in the tube
      assert(m_plotType == PlotType::TubeIntegral);
      plotTubeIntegrals(detindex);
    }
  } else {
    m_miniplot->removeActiveCurve();
  }
}

/**
* Plot the accumulated data in a tube against time of flight.
* @param detid :: A detector id. The miniplot will display data for a component
* containing the detector
*   with this id.
* @param instrumentActor The actor giving access to the workspace
* @param A CompAssembly object to be summed
*/
void MiniPlotController::plotTubeSums(size_t detindex) {
  auto plotData = prepareDataForSumsPlot(detindex);
  if (plotData.x.empty() || plotData.y.empty())
    return;
  m_miniplot->setActiveCurve(std::move(plotData));
}

/**
* Plot the data integrated over the time bins. The values are
* plotted against the length of the tube, but the units on the x-axis can be one
* of the following:
*   DETECTOR_ID
*   LENGTH
*   PHI
* The units can be set with setTubeXUnits(...) method.
* @param detid :: A detector id. The miniplot will display data for a component
* containing the detector
*   with this id.
*/
void MiniPlotController::plotTubeIntegrals(size_t detindex) {
  MiniPlotCurveData plotData = prepareDataForIntegralsPlot(detindex);
  if (plotData.x.empty() || plotData.y.empty()) {
    clear();
    return;
  }
  m_miniplot->setActiveCurve(std::move(plotData));
}

/**
* Prepare data for plotting a spectrum of a PlotType::Single detector.
* @param detid :: ID of the detector to be plotted.
* @param includeErrors If true then provide the error data as well
* @return A new MiniPlotCurveData object containing the data to plot
*/
MiniPlotCurveData
MiniPlotController::prepareDataForSinglePlot(size_t detindex,
                                             bool includeErrors) {
  const auto &actor = m_instrWidget->getInstrumentActor();
  auto ws = actor.getWorkspace();

  MiniPlotCurveData data;
  auto wi = actor.getWorkspaceIndex(detindex);
  if (wi == InstrumentActor::INVALID_INDEX)
    return data;

  // find min and max for x
  size_t imin, imax;
  actor.getBinMinMaxIndex(wi, imin, imax);

  // get the data
  const auto &xdet = ws->points(wi);
  const auto &ydet = ws->y(wi);
  data.x.assign(xdet.begin() + imin, xdet.begin() + imax);
  data.y.assign(ydet.begin() + imin, ydet.begin() + imax);
  if (includeErrors) {
    const auto &edet = ws->e(wi);
    data.e.assign(edet.begin() + imin, edet.begin() + imax);
  }
  // metadata
  data.xunit = QString::fromStdString(ws->getAxis(0)->unit()->unitID());
  data.label = "Detector " + QString::number(detindex);
  return data;
}

/**
* Prepare data for plotting accumulated data in a tube against X.
* @param detid :: A detector id. The miniplot will display data for a component
* containing the detector
*   with this id.
* @param instrumentActor The actor giving access to the workspace
* @param A CompAssembly object to be summed
* @param includeErrors If true then provide the error data as well
* @return A new MiniPlotCurveData object containing the data to plot
*/
MiniPlotCurveData
MiniPlotController::prepareDataForSumsPlot(size_t detindex,
                                           bool includeErrors) {
  const auto &actor = m_instrWidget->getInstrumentActor();
  auto ws = actor.getWorkspace();
  const auto &componentInfo = actor.componentInfo();
  auto parent = componentInfo.parent(detindex);
  auto ass = componentInfo.detectorsInSubtree(parent);
  MiniPlotCurveData data;

  auto wi = actor.getWorkspaceIndex(detindex);
  if (wi == InstrumentActor::INVALID_INDEX)
    return data;

  size_t imin, imax;
  actor.getBinMinMaxIndex(wi, imin, imax);

  const auto &xdet = ws->points(wi);
  data.x.assign(xdet.begin() + imin, xdet.begin() + imax);
  data.y.resize(xdet.size(), 0.0);
  if (includeErrors)
    data.e.resize(xdet.size(), 0.0);

  for (auto det : ass) {
    if (componentInfo.isDetector(det)) {
      auto index = actor.getWorkspaceIndex(det);
      if (index == InstrumentActor::INVALID_INDEX)
        continue;
      const auto &ydet = ws->y(index);
      std::transform(data.y.begin(), data.y.end(), ydet.begin() + imin,
                     data.y.begin(), std::plus<double>());
      if (includeErrors) {
        const auto &edet = ws->e(index);
        // sum squares
        std::transform(data.e.begin(), data.e.end(), edet.begin() + imin,
                       data.e.begin(),
                       [](double lhs, double rhs) { return lhs + rhs * rhs; });
      }
    }
  }

  // sqrt errors
  if (includeErrors) {
    std::transform(data.e.begin(), data.e.end(), data.e.begin(),
                   (double (*)(double))std::sqrt);
  }
  return data;
}

/**
* Prepare data for plotting the data integrated over the time bins. The values
* are
* plotted against the length of the tube, but the units on the x-axis can be one
* of the following:
*   DETECTOR_ID
*   LENGTH
*   PHI
*   OUT_OF_PLANE_ANGLE
* The units can be set with setTubeXUnits(...) method.
* @param detid :: A detector id. The miniplot will display data for a component
* containing the detector
*   with this id.
* @param x :: Vector of x coordinates (output)
* @param y :: Vector of y coordinates (output)
* @param err :: Optional pointer to a vector of errors (output)
*/
MiniPlotCurveData
MiniPlotController::prepareDataForIntegralsPlot(size_t detindex,
                                                bool includeErrors) {
  const auto &actor = m_instrWidget->getInstrumentActor();
  const auto &componentInfo = actor.componentInfo();
  auto ws = actor.getWorkspace();
  // Does the instrument definition specify that psi should be offset.
  std::vector<std::string> parameters =
      ws->getInstrument()->getStringParameter("offset-phi");
  const bool bOffsetPsi = (!parameters.empty()) &&
                          std::find(parameters.begin(), parameters.end(),
                                    "Always") != parameters.end();

  MiniPlotCurveData curveData;
  auto parent = componentInfo.parent(detindex);
  auto ass = componentInfo.detectorsInSubtree(parent);
  auto wi = actor.getWorkspaceIndex(detindex);
  if (wi == InstrumentActor::INVALID_INDEX)
    return curveData;
  // imin and imax give the bin integration range
  size_t imin, imax;
  actor.getBinMinMaxIndex(wi, imin, imax);

  auto samplePos = actor.getInstrument()->getSample()->getPos();
  const auto n = ass.size();
  if (n == 0) {
    // don't think it's ever possible but...
    throw std::runtime_error("PickTab miniplot: empty instrument assembly");
  }
  if (n == 1) {
    // if assembly has just one element there is nothing to plot
    return curveData;
  }
  if (!componentInfo.isDetector(ass[0])) {
    // it's not an assembly of detectors,
    // could be a mixture of monitors and other components
    return curveData;
  }
  auto normal = componentInfo.position(ass[1]) - componentInfo.position(ass[0]);
  normal.normalize();
  // collect and sort xye pairs
  using XYEData = std::tuple<double, double, double>;
  std::vector<XYEData> xyeUnsorted;
  xyeUnsorted.reserve(n);
  const auto &detectorInfo = actor.detectorInfo();
  auto pos0 = detectorInfo.position(ass[0]);
  for (auto det : ass) {
    if (componentInfo.isDetector(det)) {
      try {
        auto id = detectorInfo.detectorIDs()[det];
        // get the x-value for detector idet
        double xvalue = 0;
        auto pos = detectorInfo.position(det);
        switch (m_tubeXUnits) {
        case TubeXUnits::LENGTH:
          xvalue = pos.distance(pos0);
          break;
        case TubeXUnits::PHI:
          xvalue = bOffsetPsi ? getPhiOffset(pos, M_PI) : getPhi(pos);
          break;
        case TubeXUnits::OUT_OF_PLANE_ANGLE: {
          xvalue = getOutOfPlaneAngle(pos, samplePos, normal);
          break;
        }
        default:
          xvalue = static_cast<double>(id);
        }
        size_t index = actor.getWorkspaceIndex(id);
        // get the y-value for detector idet
        const auto &ydet = ws->y(index);
        const double sum =
            std::accumulate(ydet.begin() + imin, ydet.begin() + imax, 0.0);
        double errorValue(0.0);
        if (includeErrors) {
          const auto &edet = ws->e(index);
          errorValue = std::accumulate(
              edet.begin() + imin, edet.begin() + imax, 0.0,
              [](double lhs, double rhs) { return lhs + rhs * rhs; });
          errorValue = std::sqrt(errorValue);
        }
        xyeUnsorted.emplace_back(std::make_tuple(xvalue, sum, errorValue));
      } catch (Mantid::Kernel::Exception::NotFoundError &) {
        continue; // Detector doesn't have a workspace index relating to it
      }
    }
  }
  if (!xyeUnsorted.empty()) {
    // sort by increasing x value
    std::sort(std::begin(xyeUnsorted), std::end(xyeUnsorted),
              [](const XYEData &a, const XYEData &b) {
                return std::get<0>(a) < std::get<0>(b);
              });
    curveData.x.reserve(xyeUnsorted.size());
    curveData.y.reserve(xyeUnsorted.size());
    if (includeErrors)
      curveData.e.reserve(xyeUnsorted.size());

    std::for_each(std::begin(xyeUnsorted), std::end(xyeUnsorted),
                  [&curveData, includeErrors](const XYEData &xye) {
                    curveData.x.emplace_back(std::get<0>(xye));
                    curveData.y.emplace_back(std::get<1>(xye));
                    if (includeErrors)
                      curveData.e.emplace_back(std::get<2>(xye));
                  });
  }
  auto xAxisCaption = getTubeXUnitsName();
  curveData.xunit = xAxisCaption;
  auto xAxisUnits = getTubeXUnitsUnits();
  if (!xAxisUnits.isEmpty()) {
    xAxisCaption += " (" + xAxisUnits + ")";
  }
  // auto parent = assembly.getParent();
  //// curve label: "tube_name (detid) Integrals"
  //// detid is included to distiguish tubes with the same name
  // curveData.label = QString::fromStdString(parent->getName()) + " (" +
  //                  QString::number(detid) + ") Integrals/" + xAxisCaption;

  return curveData;
}

/**
* Save data plotted on the miniplot into a MatrixWorkspace.
*/
void MiniPlotController::savePlotToWorkspace() {
  if (!m_miniplot->hasActiveCurve() && !m_miniplot->hasStoredCurves()) {
    // nothing to save
    return;
  }
  const auto &actor = m_instrWidget->getInstrumentActor();
  auto parentWorkspace = actor.getWorkspace();
  // interpret curve labels and reconstruct the data to be saved
  QStringList labels = m_miniplot->storedCurveLabels();
  if (m_miniplot->hasActiveCurve()) {
    labels << m_miniplot->activeCurveLabel();
  }
  std::vector<double> X, Y, E;
  size_t nbins = 0;
  // to keep det ids for spectrum-detector mapping in the output workspace
  std::vector<Mantid::detid_t> detids;
  // unit id for x vector in the created workspace
  std::string unitX;
  const bool includeErrors(true);
  for (QString label : labels) {
    MiniPlotCurveData curveData;
    // split the label to get the detector id and selection type
    QStringList parts = label.split(QRegExp("[()]"));
    if (label == "multiple") {
      if (X.empty()) {
        // label doesn't have any info on how to reproduce the curve:
        // only the current curve can be saved
        std::vector<size_t> dets;
        m_instrWidget->getSurface()->getMaskedDetectors(dets);
        actor.sumDetectors(dets, curveData.x, curveData.y);
        unitX = parentWorkspace->getAxis(0)->unit()->unitID();
      } else {
        QMessageBox::warning(nullptr, "MantidPlot - Warning",
                             "Cannot save the stored curves.\nOnly the "
                             "current curve will be saved.");
      }
    } else if (parts.size() == 3) {
      int detid = parts[1].toInt();
      QString sumOrIntegral = parts[2].trimmed();
      if (sumOrIntegral == "Sum") {
        curveData = prepareDataForSumsPlot(detid, includeErrors);
        unitX = parentWorkspace->getAxis(0)->unit()->unitID();
      } else {
        curveData = prepareDataForIntegralsPlot(detid, includeErrors);
        unitX = sumOrIntegral.split('/')[1].toStdString();
      }
    } else if (parts.size() == 1) {
      // second word is detector id
      int detid = parts[0].split(QRegExp("\\s+"))[1].toInt();
      curveData = prepareDataForSinglePlot(detid, includeErrors);
      unitX = parentWorkspace->getAxis(0)->unit()->unitID();
      // save det ids for the output workspace
      detids.push_back(static_cast<Mantid::detid_t>(detid));
    } else {
      continue;
    }
    if (!curveData.x.empty()) {
      if (nbins > 0 && curveData.x.size() != nbins) {
        QMessageBox::critical(NULL, "MantidPlot - Error",
                              "Curves have different sizes.");
        return;
      } else {
        nbins = curveData.x.size();
      }
      X.insert(X.end(), curveData.x.begin(), curveData.x.end());
      Y.insert(Y.end(), curveData.y.begin(), curveData.y.end());
      E.insert(E.end(), curveData.e.begin(), curveData.e.end());
    }
  }
  // call CreateWorkspace algorithm. Created workspace will have name "Curves"
  if (!X.empty()) {
    if (nbins == 0)
      nbins = 1;
    E.resize(Y.size(), 1.0);
    auto alg =
        Mantid::API::AlgorithmFactory::Instance().create("CreateWorkspace", -1);
    alg->initialize();
    alg->setPropertyValue("OutputWorkspace", "Curves");
    alg->setProperty("DataX", X);
    alg->setProperty("DataY", Y);
    alg->setProperty("DataE", E);
    alg->setProperty("NSpec", static_cast<int>(X.size() / nbins));
    alg->setProperty("UnitX", unitX);
    alg->setPropertyValue("ParentWorkspace", parentWorkspace->getName());
    alg->execute();

    if (!detids.empty()) {
      // set up spectra - detector mapping
      auto ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
          AnalysisDataService::Instance().retrieve("Curves"));
      if (!ws) {
        throw std::runtime_error("Failed to create Curves workspace");
      }

      if (detids.size() == ws->getNumberHistograms()) {
        size_t i = 0;
        for (std::vector<Mantid::detid_t>::const_iterator id = detids.begin();
             id != detids.end(); ++id, ++i) {
          ws->getSpectrum(i).setDetectorID(*id);
        }
      }

    } // !detids.empty()
  }
}

/**
* Calculate the angle between a vector ( == pos - origin ) and a plane (
* orthogonal to normal ).
* The angle is positive if the vector and the normal make an acute angle.
* @param pos :: Vector's end.
* @param origin :: Vector's origin.
* @param normal :: Normal to the plane.
* @return :: Angle between the vector and the plane in radians in [-pi/2,
* pi/2].
*/
double
MiniPlotController::getOutOfPlaneAngle(const Mantid::Kernel::V3D &pos,
                                       const Mantid::Kernel::V3D &origin,
                                       const Mantid::Kernel::V3D &normal) {
  Mantid::Kernel::V3D vec = pos - origin;
  vec.normalize();
  return asin(vec.scalar_prod(normal));
}

/**
* Return symbolic name of current TubeXUnit.
*/
QString MiniPlotController::getTubeXUnitsName() const {
  switch (m_tubeXUnits) {
  case TubeXUnits::LENGTH:
    return "Length";
  case TubeXUnits::PHI:
    return "Phi";
  case TubeXUnits::OUT_OF_PLANE_ANGLE:
    return "Out of plane angle";
  default:
    break;
  }
  return "Detector ID";
}

/**
* Return symbolic name of units of current TubeXUnit.
*/
QString MiniPlotController::getTubeXUnitsUnits() const {
  switch (m_tubeXUnits) {
  case TubeXUnits::LENGTH:
    return "m";
  case TubeXUnits::PHI:
    return "radians";
  case TubeXUnits::OUT_OF_PLANE_ANGLE:
    return "radians";
  default:
    return "";
  }
  return "";
}

/**
* Get the plot caption for the current plot type.
*/
QString MiniPlotController::getPlotCaption() const {

  switch (m_plotType) {
  case PlotType::Single:
    return "Plotting detector spectra";
  case PlotType::DetectorSum:
    return "Plotting multiple detector sum";
  case PlotType::TubeSum:
    return "Plotting sum";
  case PlotType::TubeIntegral:
    return "Plotting integral";
  default:
    throw std::logic_error("getPlotCaption: Unknown plot type.");
  }
}

/**
 * Display the miniplot's context menu.
 */
void MiniPlotController::showContextMenu(QPoint pos) {
  QMenu context(m_miniplot);

  auto plotType = getPlotType();
  if (plotType == PlotType::TubeSum || plotType == PlotType::TubeIntegral) {
    // only for multiple detector selectors
    context.addActions(m_summationType->actions());
    m_sumDetectors->setChecked(plotType == PlotType::TubeSum);
    m_integrateTimeBins->setChecked(plotType != PlotType::TubeSum);
    m_integrateTimeBins->setEnabled(true);
    context.addSeparator();
  }
  if (m_miniplot->hasStoredCurves()) {
    // the remove menu
    QMenu *removeCurves = new QMenu("Remove", &context);
    QSignalMapper *signalMapper = new QSignalMapper(this);
    QStringList labels = m_miniplot->storedCurveLabels();
    for (QString label : labels) {
      QAction *remove = new QAction(label, removeCurves);
      removeCurves->addAction(remove);
      connect(remove, SIGNAL(triggered()), signalMapper, SLOT(map()));
      signalMapper->setMapping(remove, label);
    }
    connect(signalMapper, SIGNAL(mapped(const QString &)), this,
            SLOT(removeCurve(const QString &)));
    context.addMenu(removeCurves);
  }

  // the axes menu
  QMenu *axes = new QMenu("Axes", &context);
  axes->addActions(m_yScaleActions->actions());

  // Tube x units menu options
  if (plotType == PlotType::TubeIntegral) {
    axes->addSeparator();
    axes->addActions(m_unitsGroup->actions());
  }
  context.addMenu(axes);

  // save plot to workspace
  if (m_miniplot->hasStoredCurves() || m_miniplot->hasActiveCurve()) {
    context.addAction(m_savePlotToWorkspace);
  }

  // show menu
  context.exec(pos);
}

/**
 * Switch the plot type to sum detectors
 */
void MiniPlotController::sumDetectors() {
  setPlotType(PlotType::DetectorSum);
  m_miniplot->update();
}

/**
 * Switch the plot type integration over the bins
 */
void MiniPlotController::integrateTimeBins() {
  setPlotType(PlotType::TubeIntegral);
  m_miniplot->update();
}

/**
 * Switch the Y scale to linear
 */
void MiniPlotController::setYScaleLinear() {
  m_miniplot->setYScaleLinear();
  m_linearY->setChecked(true);
  m_logY->setChecked(false);
}

/**
 * Switch the Y scale to logarithmic
 */
void MiniPlotController::setYScaleLog() {
  m_miniplot->setYScaleLog();
  m_logY->setChecked(true);
  m_linearY->setChecked(false);
}

/**
 * Set the unit type basd on an integer
 */
void MiniPlotController::setTubeXUnits(int unit) {
  setTubeXUnits(fromInt<TubeXUnits>(unit));
}

/**
 * Remove a curve based on a label
 * @param label The label "attached" to the curve
 */
void MiniPlotController::removeCurve(const QString &label) {
  m_miniplot->removeCurve(label);
}

/**
* Add a peak to the PlotType::Single crystal peak table.
* @param x :: Time of flight
* @param y :: Peak height (counts)
*/
void MiniPlotController::addPeak(double x, double y) {

  try {
    auto surface = m_instrWidget->getSurface();
    if (!surface)
      return;
    const auto &actor = m_instrWidget->getInstrumentActor();
    auto tw = surface->getEditPeaksWorkspace();
    auto ws = actor.getWorkspace();
    std::string peakTableName;
    bool newPeaksWorkspace = false;
    if (tw) {
      peakTableName = tw->getName();
    } else {
      peakTableName = "SingleCrystalPeakTable";
      // This does need to get the instrument from the workspace as it's doing
      // calculations
      // .....and this method should be an algorithm! Or at least somewhere
      // different to here.
      auto instr = ws->getInstrument();

      auto &mtdDataStore = AnalysisDataService::Instance();
      if (!mtdDataStore.doesExist(peakTableName)) {
        tw = WorkspaceFactory::Instance().createPeaks("PeaksWorkspace");
        tw->setInstrument(instr);
        mtdDataStore.add(peakTableName, tw);
        newPeaksWorkspace = true;
      } else {
        tw = boost::dynamic_pointer_cast<IPeaksWorkspace>(
            mtdDataStore.retrieve(peakTableName));
        if (!tw) {
          QMessageBox::critical(m_miniplot, "Mantid - Error",
                                "Workspace " +
                                    QString::fromStdString(peakTableName) +
                                    " is not a TableWorkspace");
          return;
        }
      }
      auto unwrappedSurface = dynamic_cast<UnwrappedSurface *>(surface.get());
      if (unwrappedSurface) {
        unwrappedSurface->setPeaksWorkspace(
            boost::dynamic_pointer_cast<IPeaksWorkspace>(tw));
      }
    }

    // Run the AddPeak algorithm
    const auto &mtdAlgMgr = AlgorithmManager::Instance();
    auto alg = mtdAlgMgr.createUnmanaged("AddPeak");
    alg->initialize();
    alg->setPropertyValue("RunWorkspace", ws->getName());
    alg->setPropertyValue("PeaksWorkspace", peakTableName);
    alg->setProperty("DetectorID", m_currentDetID);
    alg->setProperty("TOF", x);
    alg->setProperty("Height", actor.getIntegratedCounts(m_currentDetID));
    alg->setProperty("BinCount", y);
    alg->execute();

    // if data WS has UB copy it to the new peaks workspace
    if (newPeaksWorkspace && ws->sample().hasOrientedLattice()) {
      auto UB = ws->sample().getOrientedLattice().getUB();
      auto lattice = new OrientedLattice;
      lattice->setUB(UB);
      tw->mutableSample().setOrientedLattice(lattice);
    }

    // if there is a UB available calculate HKL for the new peak
    if (tw->sample().hasOrientedLattice()) {
      auto alg = mtdAlgMgr.createUnmanaged("CalculatePeaksHKL");
      alg->initialize();
      alg->setPropertyValue("PeaksWorkspace", peakTableName);
      alg->execute();
    }

    if (tw->rowCount() > 0) {
      auto index = tw->rowCount() - 1;
      auto &peak = tw->getPeak(static_cast<int>(index));
      addPeakMarker(peak);
    }

  } catch (std::exception &e) {
    QMessageBox::critical(
        m_miniplot, "MantidPlot -Error",
        "Cannot create a Peak object because of the error:\n" +
            QString(e.what()));
  }
}

/**
 * Create all internal actions
 */
void MiniPlotController::initActions() {
  // summation type
  m_sumDetectors = new QAction("Sum", this);
  connect(m_sumDetectors, SIGNAL(triggered()), this, SLOT(sumDetectors()));
  m_sumDetectors->setCheckable(true);
  m_sumDetectors->setChecked(true);
  m_integrateTimeBins = new QAction("Integrate", this);
  connect(m_integrateTimeBins, SIGNAL(triggered()), this,
          SLOT(integrateTimeBins()));
  m_integrateTimeBins->setCheckable(true);
  m_summationType = new QActionGroup(this);
  m_summationType->addAction(m_sumDetectors);
  m_summationType->addAction(m_integrateTimeBins);

  // scale
  m_linearY = new QAction("Y linear scale", this);
  m_linearY->setCheckable(true);
  m_linearY->setChecked(true);
  connect(m_linearY, SIGNAL(triggered()), m_miniplot, SLOT(setYScaleLinear()));
  m_logY = new QAction("Y log scale", this);
  m_logY->setCheckable(true);
  connect(m_logY, SIGNAL(triggered()), m_miniplot, SLOT(setYScaleLog()));
  m_yScaleActions = new QActionGroup(this);
  m_yScaleActions->addAction(m_linearY);
  m_yScaleActions->addAction(m_logY);

  // units
  m_unitsMapper = new QSignalMapper(this);
  m_detidUnits = new QAction("Detector ID", this);
  m_detidUnits->setCheckable(true);
  m_detidUnits->setChecked(true);
  m_unitsMapper->setMapping(m_detidUnits,
                            toInt<TubeXUnits>(TubeXUnits::DETECTOR_ID));
  connect(m_detidUnits, SIGNAL(triggered()), m_unitsMapper, SLOT(map()));

  m_lengthUnits = new QAction("Tube length", this);
  m_lengthUnits->setCheckable(true);
  m_unitsMapper->setMapping(m_lengthUnits,
                            toInt<TubeXUnits>(TubeXUnits::LENGTH));
  connect(m_lengthUnits, SIGNAL(triggered()), m_unitsMapper, SLOT(map()));

  m_phiUnits = new QAction("Phi", this);
  m_phiUnits->setCheckable(true);
  m_unitsMapper->setMapping(m_phiUnits, toInt<TubeXUnits>(TubeXUnits::PHI));
  connect(m_phiUnits, SIGNAL(triggered()), m_unitsMapper, SLOT(map()));

  m_outOfPlaneAngleUnits = new QAction("Out of plane angle", this);
  m_outOfPlaneAngleUnits->setCheckable(true);
  m_unitsMapper->setMapping(m_outOfPlaneAngleUnits,
                            toInt<TubeXUnits>(TubeXUnits::OUT_OF_PLANE_ANGLE));
  connect(m_outOfPlaneAngleUnits, SIGNAL(triggered()), m_unitsMapper,
          SLOT(map()));

  m_unitsGroup = new QActionGroup(this);
  m_unitsGroup->addAction(m_detidUnits);
  m_unitsGroup->addAction(m_lengthUnits);
  // re #4169 disabled until fixed or removed
  m_unitsGroup->addAction(m_phiUnits);
  m_unitsGroup->addAction(m_outOfPlaneAngleUnits);
  connect(m_unitsMapper, SIGNAL(mapped(int)), this, SLOT(setTubeXUnits(int)));

  // save to workspace
  m_savePlotToWorkspace = new QAction("Save plot to workspace", this);
  connect(m_savePlotToWorkspace, SIGNAL(triggered()), this,
          SLOT(savePlotToWorkspace()));
}
}
}
