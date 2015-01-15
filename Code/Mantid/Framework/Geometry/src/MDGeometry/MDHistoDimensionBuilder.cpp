#include "MantidGeometry/MDGeometry/MDHistoDimensionBuilder.h"
#include "MantidKernel/Strings.h"

namespace Mantid {
namespace Geometry {

/// Constructor
MDHistoDimensionBuilder::MDHistoDimensionBuilder()
    : m_min(0), m_max(0), m_nbins(0), m_minSet(false), m_maxSet(false) {}

/// Destructor
MDHistoDimensionBuilder::~MDHistoDimensionBuilder() {}

/*
Copy constructor
@param other : source of the copy
*/
MDHistoDimensionBuilder::MDHistoDimensionBuilder(
    const MDHistoDimensionBuilder &other)
    : m_name(other.m_name), m_id(other.m_id), m_units(other.m_units),
      m_min(other.m_min), m_max(other.m_max), m_nbins(other.m_nbins),
      m_minSet(other.m_minSet), m_maxSet(other.m_maxSet) {}

/*
Assignment
@param other : source of the assignment
*/
MDHistoDimensionBuilder &MDHistoDimensionBuilder::
operator=(const MDHistoDimensionBuilder &other) {
  if (&other != this) {
    m_name = other.m_name;
    m_id = other.m_id;
    m_units = other.m_units;
    m_max = other.m_max;
    m_min = other.m_min;
    m_nbins = other.m_nbins;
    m_maxSet = other.m_maxSet;
    m_minSet = other.m_minSet;
  }
  return *this;
}

/*
Setter for the dimension name
@param name : friendly name of dimension
*/
void MDHistoDimensionBuilder::setName(std::string name) {
  // String any spaces
  m_name = Kernel::Strings::strip(name);
}

/*
Setter for the dimension id
@param id : id of the dimension
*/
void MDHistoDimensionBuilder::setId(std::string id) { m_id = id; }

/*
Setter for the dimension units
@param units : unit type of dimension
*/
void MDHistoDimensionBuilder::setUnits(std::string units) { m_units = units; }

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

/*
Creational method
@return fully constructed MDHistoDimension instance.
*/
MDHistoDimension *MDHistoDimensionBuilder::createRaw() {
  if (m_name.empty()) {
    throw std::invalid_argument(
        "Cannot create MDHistogramDimension without setting a name.");
  }
  if (m_id.empty()) {
    throw std::invalid_argument(
        "Cannot create MDHistogramDimension without setting a id.");
  }
  if (m_units.empty()) {
    throw std::invalid_argument(
        "Cannot create MDHistogramDimension without setting a unit type.");
  }
  if (!m_minSet) {
    throw std::invalid_argument(
        "Cannot create MDHistogramDimension without setting min.");
  }
  if (!m_maxSet) {
    throw std::invalid_argument(
        "Cannot create MDHistogramDimension without setting max.");
  }
  if (m_min >= m_max) {
    throw std::invalid_argument(
        "Cannot create MDHistogramDimension with min >= max.");
  }
  if (m_nbins <= 0) {
    throw std::invalid_argument(
        "Cannot create MDHistogramDimension without setting a n bins.");
  }
  return new MDHistoDimension(m_name, m_id, m_units, coord_t(m_min),
                              coord_t(m_max), m_nbins);
}

/*
Creational method
@return fully constructed MDHistoDimension instance wrapped in a boost shared
pointer.
*/
IMDDimension_sptr MDHistoDimensionBuilder::create() {
  return IMDDimension_sptr(createRaw());
}
}
}
