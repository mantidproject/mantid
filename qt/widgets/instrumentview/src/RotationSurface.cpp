// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/RotationSurface.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/InstrumentView/InstrumentRenderer.h"
#include "MantidQtWidgets/InstrumentView/UnwrappedDetector.h"

#include <QApplication>
#include <QCursor>
#include <QMessageBox>

using namespace Mantid::Geometry;
using Mantid::Beamline::ComponentType;
namespace {
// The logger object
Mantid::Kernel::Logger g_log("RotationSurface");
} // namespace

namespace MantidQt {
namespace MantidWidgets {

RotationSurface::RotationSurface(const InstrumentActor *rootActor,
                                 const Mantid::Kernel::V3D &origin,
                                 const Mantid::Kernel::V3D &axis)
    : UnwrappedSurface(rootActor), m_pos(origin), m_zaxis(axis),
      m_manual_u_correction(false) {}

void RotationSurface::findAxes() {
  // First detector defines the surface's x axis
  if (m_xaxis.nullVector()) {
    Mantid::Kernel::V3D pos = m_instrActor->getDetPos(0) - m_pos;
    double z = pos.scalar_prod(m_zaxis);
    if (z == 0.0 || fabs(z) == pos.norm()) {
      // find the shortest projection of m_zaxis and direct m_xaxis along it
      bool isY = false;
      bool isZ = false;
      if (fabs(m_zaxis.Y()) < fabs(m_zaxis.X()))
        isY = true;
      if (fabs(m_zaxis.Z()) < fabs(m_zaxis.Y()))
        isZ = true;
      if (isZ) {
        m_xaxis = Mantid::Kernel::V3D(0, 0, 1);
      } else if (isY) {
        m_xaxis = Mantid::Kernel::V3D(0, 1, 0);
      } else {
        m_xaxis = Mantid::Kernel::V3D(1, 0, 0);
      }
    } else {
      m_xaxis = normalize(pos - m_zaxis * z);
    }
    m_yaxis = m_zaxis.cross_prod(m_xaxis);
  }
}

std::vector<size_t> RotationSurface::retrieveSurfaceDetectors() const {
  const auto &componentInfo = m_instrActor->componentInfo();
  const auto &renderer = m_instrActor->getInstrumentRenderer();
  const auto &components = m_instrActor->components();
  std::vector<size_t> detectors;

  auto root = componentInfo.root();
  if (renderer.isUsingLayers()) { // handle voxel detectors
    for (const auto &component : components) {
      auto parent = componentInfo.parent(component);
      auto grandparent = componentInfo.parent(parent);
      auto parentType = componentInfo.componentType(parent);
      auto grandparentType = componentInfo.componentType(grandparent);
      auto componentType = componentInfo.componentType(component);
      if (componentType == ComponentType::Grid) {
        // Select detectors in layer and add to list for display
        const auto &layers = componentInfo.children(component);
        auto layer = layers[renderer.selectedLayer()];
        auto dets = componentInfo.detectorsInSubtree(layer);
        detectors.insert(detectors.end(), dets.begin(), dets.end());
      } else if (component != root && parentType != ComponentType::Grid &&
                 grandparentType != ComponentType::Grid) {
        // Add detectors not in any way related to a grid
        auto dets = componentInfo.detectorsInSubtree(component);
        detectors.insert(detectors.end(), dets.begin(), dets.end());
      }
    }
  } else // use all instrument detectors otherwise
    detectors = componentInfo.detectorsInSubtree(root);

  return detectors;
}

void RotationSurface::correctUCoords(double manual_u_min, double manual_u_max) {
  // apply a shift in u-coord either found automatically
  // or set manually
  if (!m_manual_u_correction) {
    // automatic gap correction
    findAndCorrectUGap();
  } else {
    // apply manually set shift
    m_u_min = manual_u_min;
    m_u_max = manual_u_max;
    for (auto &udet : m_unwrappedDetectors) {
      udet.u = applyUCorrection(udet.u);
    }
  }
  updateViewRectForUCorrection();
}

void RotationSurface::createUnwrappedDetectors() {
  const auto &detectorInfo = m_instrActor->detectorInfo();
  const auto &detIds = detectorInfo.detectorIDs();
  auto detectors = retrieveSurfaceDetectors();
  bool exceptionThrown = false;
  // For each detector in the order of actors
  // cppcheck-suppress syntaxError
  PRAGMA_OMP(parallel for)
  for (int ii = 0; ii < int(detectors.size()); ++ii) {
    if (!exceptionThrown) {
      auto i = detectors[size_t(ii)];
      try {
        if (detectorInfo.isMonitor(i) || detIds[i] < 0) {
          m_unwrappedDetectors[i] = UnwrappedDetector();
        } else {
          // A real detector.
          // Position, relative to origin
          auto rpos = detectorInfo.position(i) - m_pos;
          // Create the unwrapped shape
          UnwrappedDetector udet(m_instrActor->getColor(i), i);
          // Calculate its position/size in UV
          // coordinates
          this->calcUV(udet, rpos);

          m_unwrappedDetectors[i] = udet;
        } // is a real detector
      } catch (Mantid::Kernel::Exception::NotFoundError &) {
        // do nothing
      } catch (...) {
        // stop executing the body of the loop
        exceptionThrown = true;
        g_log.error("Unknown exception thrown.");
      }
    }
  } // for each detector in pick order

  if (exceptionThrown)
    throw std::exception();
}

/**
 * Initialize the surface.
 */
void RotationSurface::init() {
  // the actor calls this->callback for each detector
  m_unwrappedDetectors.clear();

  // if u-correction is applied manually then m_u_min and m_u_max
  // have valid values and have to be saved
  double manual_u_min = m_u_min;
  double manual_u_max = m_u_max;

  size_t ndet = m_instrActor->ndetectors();
  m_unwrappedDetectors.resize(ndet);
  if (ndet == 0)
    return;

  findAxes();

  // give some valid values to u bounds in case some code checks
  // on u to be within them
  m_u_min = -DBL_MAX;
  m_u_max = DBL_MAX;

  // Set if one of the threads in the following loop
  // throws an exception
  bool exceptionThrown = false;

  try {
    createUnwrappedDetectors();
  } catch (std::exception &) {
    exceptionThrown = true;
  }

  // if the loop above has thrown stop execution
  if (exceptionThrown) {
    throw std::runtime_error("An exception was thrown. See log for detail.");
  }

  // find the overall edges in u and v coords
  findUVBounds();

  correctUCoords(manual_u_min, manual_u_max);
}

/** Update the view rect to account for the U correction
 */
void RotationSurface::updateViewRectForUCorrection() {
  const auto offsets = calculateViewRectOffsets();
  const auto min = QPointF(m_u_min - offsets.first, m_v_min - offsets.second);
  const auto max = QPointF(m_u_max + offsets.first, m_v_max + offsets.second);
  m_viewRect = RectF(min, max);
}

/** Calculate UV offsets to the view rect
 *
 * @return a std::pair containing the u & v offsets for the view rect
 */
std::pair<double, double> RotationSurface::calculateViewRectOffsets() {
  const auto dU = fabs(m_u_max - m_u_min);
  const auto dV = fabs(m_v_max - m_v_min);
  auto du = dU * 0.05;
  auto dv = dV * 0.05;

  if (m_width_max > du && std::isfinite(m_width_max)) {
    if (du > 0 && !(dU >= m_width_max)) {
      m_width_max = dU;
    }
    du = m_width_max;
  }

  if (m_height_max > dv && std::isfinite(m_height_max)) {
    if (dv > 0 && !(dV >= m_height_max)) {
      m_height_max = dV;
    }
    dv = m_height_max;
  }

  return std::make_pair(du, dv);
}

void RotationSurface::findUVBounds() {
  m_u_min = DBL_MAX;
  m_u_max = -DBL_MAX;
  m_v_min = DBL_MAX;
  m_v_max = -DBL_MAX;
  for (size_t i = 0; i < m_unwrappedDetectors.size(); ++i) {
    const UnwrappedDetector &udet = m_unwrappedDetectors[i];
    if (udet.empty() ||
        !m_instrActor->componentInfo().hasValidShape(udet.detIndex))
      continue;
    if (udet.u < m_u_min)
      m_u_min = udet.u;
    if (udet.u > m_u_max)
      m_u_max = udet.u;
    if (udet.v < m_v_min)
      m_v_min = udet.v;
    if (udet.v > m_v_max)
      m_v_max = udet.v;
  }
}

void RotationSurface::findAndCorrectUGap() {
  double period = uPeriod();
  if (period == 0.0)
    return;
  const int nbins = 1000;
  std::vector<bool> ubins(nbins);
  double bin_width = fabs(m_u_max - m_u_min) / (nbins - 1);
  if (bin_width == 0.0) {
    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
    QMessageBox::warning(
        nullptr, tr("MantidPlot - Instrument view warning"),
        tr("Rotation surface: failed to build unwrapped surface"));
    QApplication::restoreOverrideCursor();
    m_u_min = 0.0;
    m_u_max = 1.0;
    return;
  }

  for (const auto &udet : m_unwrappedDetectors) {
    if (udet.empty() ||
        !m_instrActor->componentInfo().hasValidShape(udet.detIndex))
      continue;
    double u = udet.u;
    int i = int((u - m_u_min) / bin_width);
    ubins[i] = true;
  }

  int iFrom = 0; // marks gap start
  int iTo = 0;   // marks gap end
  int i0 = 0;
  bool inGap = false;
  for (int i = 0; i < int(ubins.size()) - 1; ++i) {
    if (!ubins[i]) {
      if (!inGap) {
        i0 = i;
      }
      inGap = true;
    } else {
      if (inGap && iTo - iFrom < i - i0) {
        iFrom = i0; // first bin in the gap
        iTo = i;    // first bin after the gap
      }
      inGap = false;
    }
  }

  double uFrom = m_u_min + iFrom * bin_width;
  double uTo = m_u_min + iTo * bin_width;
  if (uTo - uFrom > period - (m_u_max - m_u_min)) {

    m_u_max = uFrom;
    m_u_min = uTo;
    if (m_u_min > m_u_max) {
      m_u_max += period;
    }

    for (auto &udet : m_unwrappedDetectors) {
      if (udet.empty() ||
          !m_instrActor->componentInfo().hasValidShape(udet.detIndex))
        continue;
      double &u = udet.u;
      u = applyUCorrection(u);
    }
  }
}

/**
 * Apply a correction to u value of a projected point due to
 * change of u-scale by findAndCorrectUGap()
 * @param u :: u-coordinate to be corrected
 * @return :: Corrected u-coordinate.
 */
double RotationSurface::applyUCorrection(double u) const {
  double period = uPeriod();
  if (period == 0.0)
    return u;
  if (u < m_u_min) {
    double periods = floor((m_u_max - u) / period) * period;
    u += periods;
  }
  if (u > m_u_max) {
    double periods = floor((u - m_u_min) / period) * period;
    u -= periods;
  }
  return u;
}

/**
 * Set new value for the u-correction.
 * Correct all uv corrdinates of detectors.
 */
void RotationSurface::setUCorrection(double umin, double umax) {
  m_u_min = umin;
  m_u_max = umax;
  double period = uPeriod();
  double du = m_u_max - m_u_min;
  if (du > period * 1.1) {
    m_u_max -= floor(du / period) * period;
  }
  while (m_u_min >= m_u_max) {
    m_u_max += period;
  }
  m_manual_u_correction = true;
  updateDetectors();
  updateViewRectForUCorrection();
}

/**
 * Set automatic u-correction
 */
void RotationSurface::setAutomaticUCorrection() {
  m_manual_u_correction = false;
  updateDetectors();
  updateViewRectForUCorrection();
}

} // namespace MantidWidgets
} // namespace MantidQt
