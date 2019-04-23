// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Qwt/MantidQwtIMDWorkspaceData.h"
#include "MantidAPI/CoordTransform.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/NullCoordTransform.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidQtWidgets/Common/QStringUtils.h"
#include <QStringBuilder>

using MantidQt::API::toQStringInternal;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using Mantid::API::CoordTransform;
using Mantid::API::IMDWorkspace;
using Mantid::API::IMDWorkspace_const_sptr;
using Mantid::API::NullCoordTransform;
using Mantid::coord_t;

/** Constructor
 *
 * @param workspace :: IMDWorkspace to plot
 * @param logScaleY :: true to plot Y in log scale
 * @param start :: start point in N-dimensions of the line
 * @param end :: end point in N-dimensions of the line
 * @param normalize :: method for normalizing the line
 * @param isDistribution :: is this a distribution (divide by bin width?)
 * @return
 */
MantidQwtIMDWorkspaceData::MantidQwtIMDWorkspaceData(
    Mantid::API::IMDWorkspace_const_sptr workspace, const bool logScaleY,
    Mantid::Kernel::VMD start, Mantid::Kernel::VMD end,
    Mantid::API::MDNormalization normalize, bool isDistribution)
    : MantidQwtWorkspaceData(logScaleY), m_workspace(workspace),
      m_preview(false), m_start(start), m_end(end), m_normalization(normalize),
      m_isDistribution(isDistribution), m_transform(nullptr),
      m_plotAxis(PlotDistance), m_currentPlotAxis(PlotDistance) {
  if (start.getNumDims() == 1 && end.getNumDims() == 1) {
    if (start[0] == 0.0 && end[0] == 0.0) {
      // Default start and end. Find the limits
      auto nonIntegDims = m_workspace->getNonIntegratedDimensions();
      std::string alongDim;
      if (!nonIntegDims.empty())
        alongDim = nonIntegDims[0]->getName();
      else
        alongDim = m_workspace->getDimension(0)->getName();
      size_t nd = m_workspace->getNumDims();
      m_start = VMD(nd);
      m_end = VMD(nd);
      for (size_t d = 0; d < nd; d++) {
        IMDDimension_const_sptr dim = m_workspace->getDimension(d);
        if (dim->getName() == alongDim) {
          // All the way through in the single dimension
          m_start[d] = dim->getMinimum();
          m_end[d] = dim->getMaximum();
        } else {
          // Mid point along each dimension
          m_start[d] = (dim->getMaximum() + dim->getMinimum()) / 2.0f;
          m_end[d] = m_start[d];
        }
      }
    }
  }
  // Unit direction of the line
  m_dir = m_end - m_start;
  if (m_end != m_start) {
    m_dir.normalize();
  } else {
    m_dir[0] = 1.0;
  }
  // And cache the X/Y values
  this->cacheLinePlot();

  this->calculateMinMax();
}

//-----------------------------------------------------------------------------
/// Copy constructor
MantidQwtIMDWorkspaceData::MantidQwtIMDWorkspaceData(
    const MantidQwtIMDWorkspaceData &data)
    : MantidQwtWorkspaceData(data) {
  copyData(data);
}

//-----------------------------------------------------------------------------
/** Assignment operator
 *
 * @param data :: copy into this
 */
MantidQwtIMDWorkspaceData &MantidQwtIMDWorkspaceData::
operator=(const MantidQwtIMDWorkspaceData &data) {
  copyData(data);
  return *this;
}

/// Destructor
MantidQwtIMDWorkspaceData::~MantidQwtIMDWorkspaceData() { delete m_transform; }

//-----------------------------------------------------------------------------
/** Cloner/virtual copy constructor
 * @return a copy of this
 */
QwtData *MantidQwtIMDWorkspaceData::copy() const {
  return new MantidQwtIMDWorkspaceData(*this);
}

/// Return a new data object of the same type but with a new workspace
MantidQwtIMDWorkspaceData *MantidQwtIMDWorkspaceData::copy(
    Mantid::API::IMDWorkspace_sptr workspace) const {
  MantidQwtIMDWorkspaceData *out;
  out = new MantidQwtIMDWorkspaceData(workspace, logScaleY(), m_start, m_end,
                                      m_normalization, m_isDistribution);
  out->m_plotAxis = this->m_plotAxis;
  out->m_currentPlotAxis = this->m_currentPlotAxis;
  out->setPreviewMode(m_preview);
  return out;
}

/**
 * Handles copying member variables for the copy constructor and
 * assignment operator
 *
 * @param data The pointer to the object to copy from
 */
void MantidQwtIMDWorkspaceData::copyData(
    const MantidQwtIMDWorkspaceData &data) {
  if (this != &data) {
    static_cast<MantidQwtWorkspaceData &>(*this) = data;
    m_workspace = data.m_workspace;
    m_preview = data.m_preview;
    m_start = data.m_start;
    m_end = data.m_end;
    m_dir = data.m_dir;
    m_normalization = data.m_normalization;
    m_isDistribution = data.m_isDistribution;
    m_originalWorkspace = data.m_originalWorkspace;
    m_plotAxis = data.m_plotAxis;
    m_currentPlotAxis = data.m_currentPlotAxis;
    m_transform = nullptr;
    if (data.m_transform)
      m_transform = data.m_transform->clone();
    this->cacheLinePlot();
  }
}

//-----------------------------------------------------------------------------
/** Cache the X/Y line plot data from this workspace and start/end points */
void MantidQwtIMDWorkspaceData::cacheLinePlot() {
  auto line = m_workspace->getLinePlot(m_start, m_end, m_normalization);
  m_lineX = line.x;
  m_Y = line.y;
  m_E = line.e;
}

//-----------------------------------------------------------------------------
/// Calculate the MinY and MaxY values
void MantidQwtIMDWorkspaceData::calculateMinMax() { calculateYMinAndMax(); }

//-----------------------------------------------------------------------------
/** Size of the data set
 */
size_t MantidQwtIMDWorkspaceData::size() const {
  if (!isPlottable()) {
    return 0;
  }
  return m_Y.size();
}

/** Return the x value of data point i
@param i :: Index
@return x X value of data point i
*/
double MantidQwtIMDWorkspaceData::getX(size_t i) const {
  double x = m_lineX[i];
  if (m_currentPlotAxis != PlotDistance && m_transform) {
    // Coordinates in the workspace being plotted
    VMD wsCoord = m_start + m_dir * x;
    // Transform to the original workspace's coordinates
    VMD originalCoord = m_transform->applyVMD(wsCoord);
    // And pick only that coordinate
    x = originalCoord[m_currentPlotAxis];
    // std::cout << wsCoord << " -> " << originalCoord << " at index " << i << "
    // is read as " << x << ". m_dimensionIndex is " << m_dimensionIndex <<
    // '\n';
  }
  return x;
}

/** Return the y value of data point i
@param i :: Index
@return y Y value of data point i
*/
double MantidQwtIMDWorkspaceData::getY(size_t i) const {
  Mantid::signal_t val = m_Y[i];
  return val;
}

/// Returns the x position of the error bar for the i-th data point (bin)
double MantidQwtIMDWorkspaceData::getEX(size_t i) const {
  return this->getX(i);
}

/// Returns the error of the i-th data point
double MantidQwtIMDWorkspaceData::getE(size_t i) const { return m_E[i]; }

/// Number of error bars to plot
size_t MantidQwtIMDWorkspaceData::esize() const {
  if (!isPlottable()) {
    return 0;
  }
  return m_E.size();
}

bool MantidQwtIMDWorkspaceData::setAsDistribution(bool on) {
  m_isDistribution = on;
  return m_isDistribution;
}

//-----------------------------------------------------------------------------
/** Set which axis to plot as the X of the line plot
 *
 * @param choice :: int, -2 = auto, -1 = distance,
 */
void MantidQwtIMDWorkspaceData::setPlotAxisChoice(int choice) {
  m_plotAxis = choice;
  this->choosePlotAxis();
}

//-----------------------------------------------------------------------------
/** Set the signal normalization to use.
 * This recalculates the line plot.
 *
 * @param choice :: one of MDNormalization enum
 */
void MantidQwtIMDWorkspaceData::setNormalization(
    Mantid::API::MDNormalization choice) {
  m_normalization = choice;
  this->cacheLinePlot();
}

//-----------------------------------------------------------------------------
/** Are we in Preview mode?
 *
 * Preview means that we are visualizing the workspace directly, i.e.,
 * while dragging the line around; Therefore there is no "original" workspace
 * to change coordinates to.
 *
 * If NOT in preview mode, then we get a reference to the original workspace,
 * which we use to display the right X axis coordinate.
 *
 * @param preview :: true for Preview mode.
 */
void MantidQwtIMDWorkspaceData::setPreviewMode(bool preview) {
  m_preview = preview;
  // If the workspace has no original, then we MUST be in preview mode.
  const size_t nOriginalWorkspaces = m_workspace->numOriginalWorkspaces();
  if (preview || (nOriginalWorkspaces == 0)) {
    // Preview mode. No transformation.
    m_originalWorkspace = m_workspace;
  } else {
    // Refer to the last workspace = the intermediate in the case of MDHisto
    // binning
    const size_t indexOfWS = nOriginalWorkspaces - 1; // Get the last workspace
    m_originalWorkspace = boost::dynamic_pointer_cast<IMDWorkspace>(
        m_workspace->getOriginalWorkspace(indexOfWS));
  }

  const size_t nTransformsToOriginal =
      m_workspace->getNumberTransformsToOriginal();
  if (preview || (nTransformsToOriginal == 0)) {
    m_transform = new NullCoordTransform(m_workspace->getNumDims());
  } else {
    const size_t indexOfTransform =
        nTransformsToOriginal - 1; // Get the last transform
    CoordTransform const *temp =
        m_workspace->getTransformToOriginal(indexOfTransform);
    if (temp)
      m_transform = temp->clone();
  }

  this->choosePlotAxis();
}

//-----------------------------------------------------------------------------
/** Automatically choose which coordinate to use as the X axis,
 * if we selected it to be automatic
 */
void MantidQwtIMDWorkspaceData::choosePlotAxis() {
  if (m_plotAxis == MantidQwtIMDWorkspaceData::PlotAuto) {
    if (m_transform) {
      // Find the start and end points in the original workspace
      VMD originalStart = m_transform->applyVMD(m_start);
      VMD originalEnd = m_transform->applyVMD(m_end);
      VMD diff = originalEnd - originalStart;

      // Now we find the dimension with the biggest change
      double largest = -1e30;
      // Default to 0
      m_currentPlotAxis = 0;
      IMDWorkspace_const_sptr originalWS = m_originalWorkspace.lock();

      bool regularBinnedMDWorkspace = false;
      if (auto mdew =
              boost::dynamic_pointer_cast<const Mantid::API::IMDEventWorkspace>(
                  m_workspace)) {
        Mantid::API::BoxController_const_sptr controller =
            mdew->getBoxController();
        bool atLeastOneDimNotIntegrated = false;
        for (size_t i = 0; i < mdew->getNumDims(); ++i) {
          if (!mdew->getDimension(i)->getIsIntegrated()) {
            atLeastOneDimNotIntegrated = true;
          }
        }
        regularBinnedMDWorkspace = atLeastOneDimNotIntegrated;
      }

      if (nullptr !=
              boost::dynamic_pointer_cast<const Mantid::API::IMDHistoWorkspace>(
                  originalWS) ||
          regularBinnedMDWorkspace) {
        for (size_t d = 0; d < diff.getNumDims(); d++) {
          if (fabs(diff[d]) > largest ||
              (originalWS && originalWS->getDimension(m_currentPlotAxis)
                                 ->getIsIntegrated())) {
            // Skip over any integrated dimensions
            if (originalWS && !originalWS->getDimension(d)->getIsIntegrated()) {
              largest = fabs(diff[d]);
              m_currentPlotAxis = int(d);
            }
          }
        }
      } else {
        for (size_t d = 0; d < diff.getNumDims(); d++) {
          if (fabs(diff[d]) > largest) {
            largest = fabs(diff[d]);
            m_currentPlotAxis = int(d);
          }
        }
      }
    } else
      // Drop to distance if the transform does not exist
      m_currentPlotAxis = MantidQwtIMDWorkspaceData::PlotDistance;
  } else {
    // Pass-through the value.
    m_currentPlotAxis = m_plotAxis;
  }
}

/**
@return the dimension index corresponding to the current plot X Axis.
*/
int MantidQwtIMDWorkspaceData::currentPlotXAxis() const {
  return m_currentPlotAxis;
}

//-----------------------------------------------------------------------------
/// @return the label for the X axis
QString MantidQwtIMDWorkspaceData::getXAxisLabel() const {
  QString xLabel;
  if (m_originalWorkspace.expired())
    return xLabel; // Empty string
  if (m_currentPlotAxis >= 0) {
    // One of the dimensions of the original
    IMDDimension_const_sptr dim =
        m_originalWorkspace.lock()->getDimension(m_currentPlotAxis);
    xLabel = QString::fromStdString(dim->getName()) + " (" +
             toQStringInternal(dim->getUnits().utf8()) + ")";
  } else {
    // Distance
    // Distance, or not set.
    xLabel = "Distance from start";
    //    if (dimX->getUnits() == dimY->getUnits())
    //      xLabel += " (" + dimX->getUnits() + ")";
    //    else
    //      xLabel += " (undefined units)";
    //    break;
  }

  return xLabel;
}

//-----------------------------------------------------------------------------
/// @return the label for the Y axis, based on the selected normalization
QString MantidQwtIMDWorkspaceData::getYAxisLabel() const {
  switch (m_normalization) {
  case Mantid::API::NoNormalization:
    return "Signal";
  case Mantid::API::VolumeNormalization:
    return "Signal/volume";
  case Mantid::API::NumEventsNormalization:
    return "Signal/num. events";
  }
  return "Unknown";
}
