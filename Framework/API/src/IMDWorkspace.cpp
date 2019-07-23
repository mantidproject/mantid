// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/VMD.h"

#include <sstream>

using Mantid::Kernel::VMD;

namespace Mantid {
namespace API {
//-----------------------------------------------------------------------------------------------
/** Default constructor */
IMDWorkspace::IMDWorkspace(const Parallel::StorageMode storageMode)
    : Workspace(storageMode), Mantid::API::MDGeometry() {
  m_convention = Kernel::ConfigService::Instance().getString("Q.convention");
}

/** Creates a single iterator and returns it.
 *
 * This calls createIterators(), a pure virtual method on IMDWorkspace which
 * has custom implementations for other workspaces.
 *
 * @param function :: Implicit function limiting space to look at
 * @return a single IMDIterator pointer
 */
std::unique_ptr<IMDIterator> IMDWorkspace::createIterator(
    Mantid::Geometry::MDImplicitFunction *function) const {
  std::vector<std::unique_ptr<IMDIterator>> iterators =
      this->createIterators(1, function);
  if (iterators.empty())
    throw std::runtime_error("IMDWorkspace::createIterator(): iterator "
                             "creation was not successful. No iterators "
                             "returned by " +
                             this->id());
  return std::move(iterators[0]);
}

//---------------------------------------------------------------------------------------------
/** @return the convention
 */
std::string IMDWorkspace::getConvention() const { return m_convention; }

//---------------------------------------------------------------------------------------------
/** @return the convention
 */
void IMDWorkspace::setConvention(std::string convention) {
  m_convention = convention;
}

//---------------------------------------------------------------------------------------------
/** @return the convention
 */
std::string IMDWorkspace::changeQConvention() {
  if (this->getConvention() == "Crystallography")
    m_convention = "Inelastic";
  else
    m_convention = "Crystallography";
  return m_convention;
}

//-------------------------------------------------------------------------------------------
/** Returns the signal (normalized by volume) at a given coordinates
 *
 * @param coords :: coordinate as a VMD vector
 * @param normalization :: how to normalize the signal returned
 * @return normalized signal
 */
signal_t IMDWorkspace::getSignalAtVMD(
    const Mantid::Kernel::VMD &coords,
    const Mantid::API::MDNormalization &normalization) const {
  return this->getSignalAtCoord(coords.getBareArray(), normalization);
}

//-------------------------------------------------------------------------------------------
/** Returns the signal (normalized by volume) at a given coordinates
 * or 0 if masked
 *
 * @param coords :: coordinate as a VMD vector
 * @param normalization :: how to normalize the signal returned
 * @return normalized signal
 */
signal_t IMDWorkspace::getSignalWithMaskAtVMD(
    const Mantid::Kernel::VMD &coords,
    const Mantid::API::MDNormalization &normalization) const {
  return this->getSignalWithMaskAtCoord(coords.getBareArray(), normalization);
}

//-----------------------------------------------------------------------------------------------

/**
 */
const std::string IMDWorkspace::toString() const {
  std::ostringstream os;
  os << id() << "\n"
     << "Title: " + getTitle() << "\n";
  for (size_t i = 0; i < getNumDims(); i++) {
    const auto &dim = getDimension(i);
    os << "Dim " << i << ": (" << dim->getName() << ") " << dim->getMinimum()
       << " to " << dim->getMaximum() << " in " << dim->getNBins() << " bins";
    // Also show the dimension ID string, if different than name
    if (dim->getDimensionId() != dim->getName())
      os << ". Id=" << dim->getDimensionId();
    os << "\n";
  }
  if (hasOriginalWorkspace()) {
    os << "Binned from '" << getOriginalWorkspace()->getName();
  }
  os << "\n";
  if (this->getConvention() == "Crystallography")
    os << "Crystallography: kf-ki";
  else
    os << "Inelastic: ki-kf";
  os << "\n";

  return os.str();
}

//----------------------------------------------------------------------------------------------
/**
 * Make a single point with NaN as the signal and error
 * This can be returned when there would otherwise be nothing to plot
 * @param x :: position on the line
 * @param y :: signal value
 * @param e :: error value
 */
void IMDWorkspace::makeSinglePointWithNaN(std::vector<coord_t> &x,
                                          std::vector<signal_t> &y,
                                          std::vector<signal_t> &e) const {
  x.push_back(0);
  y.push_back(std::numeric_limits<signal_t>::quiet_NaN());
  e.push_back(std::numeric_limits<signal_t>::quiet_NaN());
}

//-----------------------------------------------------------------------------------------------
/** Obtain coordinates for a line plot through a IMDWorkspace.
 * Cross the workspace from start to end points, recording the signal along the
 *line.
 * Sets the x,y vectors to the histogram bin boundaries and counts
 *
 * @param start :: coordinates of the start point of the line
 * @param end :: coordinates of the end point of the line
 * @param normalize :: how to normalize the signal
 * @returns :: a LinePlot in which x is set to the boundaries of the bins,
 * relative to start of the line, y is set to the normalized signal for
 * each bin with Length = length(x) - 1 and e is set to the normalized
 * errors for each bin with Length = length(x) - 1.
 */
IMDWorkspace::LinePlot
IMDWorkspace::getLinePlot(const Mantid::Kernel::VMD &start,
                          const Mantid::Kernel::VMD &end,
                          Mantid::API::MDNormalization normalize) const {
  // TODO: Don't use a fixed number of points later
  size_t numPoints = 200;

  VMD step = (end - start) / double(numPoints);
  double stepLength = step.norm();

  // This will be the curve as plotted
  LinePlot line;
  for (size_t i = 0; i < numPoints; i++) {
    // Coordinate along the line
    VMD coord = start + step * double(i);
    // Record the position along the line
    line.x.push_back(static_cast<coord_t>(stepLength * double(i)));

    signal_t yVal = this->getSignalAtCoord(coord.getBareArray(), normalize);
    line.y.push_back(yVal);
    line.e.push_back(0.0);
  }
  // And the last point
  line.x.push_back((end - start).norm());
  return line;
}

/**
@return normalization preferred for visualization. Set to none for the generic
case, but overriden elsewhere.
*/
MDNormalization IMDWorkspace::displayNormalization() const {
  return NoNormalization;
}

/**
@return normalization preferred for visualization of histo workspaces. Set to
none for the generic case, but overriden elsewhere.
*/
MDNormalization IMDWorkspace::displayNormalizationHisto() const {
  return NoNormalization;
}
} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
/** In order to be able to cast PropertyWithValue classes correctly a definition
 * for the PropertyWithValue<IMDEventWorkspace> is required */
template <>
MANTID_API_DLL Mantid::API::IMDWorkspace_sptr
IPropertyManager::getValue<Mantid::API::IMDWorkspace_sptr>(
    const std::string &name) const {
  auto *prop =
      dynamic_cast<PropertyWithValue<Mantid::API::IMDWorkspace_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected shared_ptr<IMDWorkspace>.";
    throw std::runtime_error(message);
  }
}

/** In order to be able to cast PropertyWithValue classes correctly a definition
 * for the PropertyWithValue<IMDWorkspace_const_sptr> is required */
template <>
MANTID_API_DLL Mantid::API::IMDWorkspace_const_sptr
IPropertyManager::getValue<Mantid::API::IMDWorkspace_const_sptr>(
    const std::string &name) const {
  auto *prop =
      dynamic_cast<PropertyWithValue<Mantid::API::IMDWorkspace_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected const shared_ptr<IMDWorkspace>.";
    throw std::runtime_error(message);
  }
}

} // namespace Kernel
} // namespace Mantid
