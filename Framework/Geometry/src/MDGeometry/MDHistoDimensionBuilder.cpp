// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <utility>

#include "MantidGeometry/MDGeometry/MDFrameFactory.h"
#include "MantidGeometry/MDGeometry/MDHistoDimensionBuilder.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/UnitLabelTypes.h"

namespace Mantid {
namespace Geometry {

/// Constructor
MDHistoDimensionBuilder::MDHistoDimensionBuilder()
    : m_units(Kernel::Units::Symbol::EmptyLabel), m_min(0), m_max(0), m_nbins(0), m_minSet(false), m_maxSet(false),
      m_frameName("") {}

/*
Setter for the dimension name
@param name : friendly name of dimension
*/
void MDHistoDimensionBuilder::setName(const std::string &name) {
  // String any spaces
  m_name = Kernel::Strings::strip(name);
}

/*
Setter for the dimension id
@param id : id of the dimension
*/
void MDHistoDimensionBuilder::setId(std::string id) { m_id = std::move(id); }

/*
Setter for the dimension units
@param units : unit type of dimension
*/
void MDHistoDimensionBuilder::setUnits(const Kernel::UnitLabel &units) { m_units = units; }

/*
Setter for the dimension min
@param min : min boundary of dimension
*/
void MDHistoDimensionBuilder::setMin(double min) {
  m_min = min;
  m_minSet = true;
}

/*
Setter for the dimension max
@param max : max boundary of dimension
*/
void MDHistoDimensionBuilder::setMax(double max) {
  m_max = max;
  m_maxSet = true;
}

/*
Setter for the dimension nbins
@param nbins : number of bins of dimension
*/
void MDHistoDimensionBuilder::setNumBins(size_t nbins) { m_nbins = nbins; }

/**
 * Setter for the frame name
 * @param frameName: the frame name
 */
void MDHistoDimensionBuilder::setFrameName(std::string frameName) { m_frameName = std::move(frameName); }

/*
Creational method
@return fully constructed MDHistoDimension instance.
*/
MDHistoDimension *MDHistoDimensionBuilder::createRaw() {
  if (m_name.empty()) {
    throw std::invalid_argument("Cannot create MDHistogramDimension without setting a name.");
  }
  if (m_id.empty()) {
    throw std::invalid_argument("Cannot create MDHistogramDimension without setting a id.");
  }
  if (m_units.ascii().empty()) {
    throw std::invalid_argument("Cannot create MDHistogramDimension without setting a unit type.");
  }
  if (!m_minSet) {
    throw std::invalid_argument("Cannot create MDHistogramDimension without setting min.");
  }
  if (!m_maxSet) {
    throw std::invalid_argument("Cannot create MDHistogramDimension without setting max.");
  }
  if (m_min >= m_max) {
    throw std::invalid_argument("Cannot create MDHistogramDimension with min >= max.");
  }
  if (m_nbins <= 0) {
    throw std::invalid_argument("Cannot create MDHistogramDimension without setting a n bins.");
  }

  // Select a Mantid Frame. Use FrameName if available else just use name.
  auto frameFactory = Mantid::Geometry::makeMDFrameFactoryChain();
  std::string frameNameForFactory = m_frameName.empty() ? m_name : m_frameName;
  Mantid::Geometry::MDFrameArgument frameArgument(frameNameForFactory, m_units);
  auto frame = frameFactory->create(frameArgument);

  return new MDHistoDimension(m_name, m_id, *frame, coord_t(m_min), coord_t(m_max), m_nbins);
}

/*
Creational method
@return fully constructed MDHistoDimension instance wrapped in a boost shared
pointer.
*/
IMDDimension_sptr MDHistoDimensionBuilder::create() { return IMDDimension_sptr(createRaw()); }
} // namespace Geometry
} // namespace Mantid
