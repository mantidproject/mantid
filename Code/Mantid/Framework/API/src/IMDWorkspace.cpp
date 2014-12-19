#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/VMD.h"

#include <sstream>

using Mantid::Kernel::VMD;

namespace Mantid {
namespace API {
//-----------------------------------------------------------------------------------------------
/** Default constructor */
IMDWorkspace::IMDWorkspace() : Workspace(), Mantid::API::MDGeometry() {}

//-----------------------------------------------------------------------------------------------
/** Copy constructor */
IMDWorkspace::IMDWorkspace(const IMDWorkspace &other)
    : Workspace(other), Mantid::API::MDGeometry(other) {}

/// Destructor
IMDWorkspace::~IMDWorkspace() {}

/** Creates a single iterator and returns it.
 *
 * This calls createIterators(), a pure virtual method on IMDWorkspace which
 * has custom implementations for other workspaces.
 *
 * @param function :: Implicit function limiting space to look at
 * @return a single IMDIterator pointer
 */
IMDIterator *IMDWorkspace::createIterator(
    Mantid::Geometry::MDImplicitFunction *function) const {
  std::vector<IMDIterator *> iterators = this->createIterators(1, function);
  if (iterators.empty())
    throw std::runtime_error("IMDWorkspace::createIterator(): iterator "
                             "creation was not successful. No iterators "
                             "returned by " +
                             this->id());
  return iterators[0];
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

//-----------------------------------------------------------------------------------------------

/**
 */
const std::string IMDWorkspace::toString() const {
  std::ostringstream os;
  os << id() << "\n"
     << "Title: " + getTitle() << "\n";
  for (size_t i = 0; i < getNumDims(); i++) {
    Geometry::IMDDimension_const_sptr dim = getDimension(i);
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
  return os.str();
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
 * @param x :: is set to the boundaries of the bins, relative to start of the
 *line.
 * @param y :: is set to the normalized signal for each bin. Length = length(x)
 *- 1
 * @param e :: is set to the normalized errors for each bin. Length = length(x)
 *- 1
 */
void IMDWorkspace::getLinePlot(const Mantid::Kernel::VMD &start,
                               const Mantid::Kernel::VMD &end,
                               Mantid::API::MDNormalization normalize,
                               std::vector<coord_t> &x,
                               std::vector<signal_t> &y,
                               std::vector<signal_t> &e) const {
  // TODO: Don't use a fixed number of points later
  size_t numPoints = 200;

  VMD step = (end - start) / double(numPoints);
  double stepLength = step.norm();

  // These will be the curve as plotted
  x.clear();
  y.clear();
  e.clear();
  for (size_t i = 0; i < numPoints; i++) {
    // Coordinate along the line
    VMD coord = start + step * double(i);
    // Record the position along the line
    x.push_back(static_cast<coord_t>(stepLength * double(i)));

    signal_t yVal = this->getSignalAtCoord(coord.getBareArray(), normalize);
    y.push_back(yVal);
    e.push_back(0.0);
  }
  // And the last point
  x.push_back((end - start).norm());
}
}
}

namespace Mantid {
namespace Kernel {
/** In order to be able to cast PropertyWithValue classes correctly a definition
 * for the PropertyWithValue<IMDEventWorkspace> is required */
template <>
MANTID_API_DLL Mantid::API::IMDWorkspace_sptr
IPropertyManager::getValue<Mantid::API::IMDWorkspace_sptr>(
    const std::string &name) const {
  PropertyWithValue<Mantid::API::IMDWorkspace_sptr> *prop =
      dynamic_cast<PropertyWithValue<Mantid::API::IMDWorkspace_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message = "Attempt to assign property " + name +
                          " to incorrect type. Expected IMDWorkspace.";
    throw std::runtime_error(message);
  }
}

/** In order to be able to cast PropertyWithValue classes correctly a definition
 * for the PropertyWithValue<IMDWorkspace_const_sptr> is required */
template <>
MANTID_API_DLL Mantid::API::IMDWorkspace_const_sptr
IPropertyManager::getValue<Mantid::API::IMDWorkspace_const_sptr>(
    const std::string &name) const {
  PropertyWithValue<Mantid::API::IMDWorkspace_sptr> *prop =
      dynamic_cast<PropertyWithValue<Mantid::API::IMDWorkspace_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message = "Attempt to assign property " + name +
                          " to incorrect type. Expected const IMDWorkspace.";
    throw std::runtime_error(message);
  }
}

} // namespace Kernel
} // namespace Mantid
