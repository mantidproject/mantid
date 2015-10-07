#include "MantidAPI/BinEdgeAxis.h"

namespace Mantid {
namespace API {

//----------------------------------------------------------------------------------------------
/**
 * Constructor taking a length
 * @param length The length of the axis
 */
BinEdgeAxis::BinEdgeAxis(const std::size_t &length)
    : NumericAxis() // default constructor
{
  m_values.resize(length);
}

/**
 * Constructor taking a list of edge values
 * @param edges A list of bin boundaries
 */
BinEdgeAxis::BinEdgeAxis(const std::vector<double> &edges)
    : NumericAxis() // default constructor
{
  m_values = edges;
}

/** Virtual constructor
 *  @param parentWorkspace :: The workspace is not used in this implementation
 *  @returns A pointer to a copy of the NumericAxis on which the method is
 * called
 */
Axis *BinEdgeAxis::clone(const MatrixWorkspace *const parentWorkspace) {
  UNUSED_ARG(parentWorkspace)
  return new BinEdgeAxis(*this);
}

/** Virtual constructor
 * @param length A new length for the axis. The values are cleared.
 * @param parentWorkspace The workspace is not used in this implementation
 * @returns A pointer to a copy of the NumericAxis on which the method is called
 */
Axis *BinEdgeAxis::clone(const std::size_t length,
                         const MatrixWorkspace *const parentWorkspace) {
  UNUSED_ARG(parentWorkspace)
  auto *newAxis = new BinEdgeAxis(*this);
  newAxis->m_values.clear();
  newAxis->m_values.resize(length);
  return newAxis;
}

/**
 * Return the values axis as they are
 * @return A vector containing the bin boundaries
 */
std::vector<double> BinEdgeAxis::createBinBoundaries() const {
  return this->getValues();
}

/** Sets the axis value at a given position
 *  @param index :: The position along the axis for which to set the value
 *  @param value :: The new value
 *  @throw  IndexError If the index requested is not in the range of this axis
 */
void BinEdgeAxis::setValue(const std::size_t &index, const double &value) {
  // Avoids setting edge information
  if (index >= length()) {
    throw Kernel::Exception::IndexError(index, length() - 1,
                                        "BinEdgeAxis: Index out of range.");
  }
  m_values[index] = value;
}

/**
 * Treats values as bin edges and returns the index of the bin, which
 * the value falls into. The maximum value will always be length() - 1
 * @param value A value on the axis
 * @return The index closest to given value
 * @throws std::out_of_range if the value is out of range of the axis
 */
size_t BinEdgeAxis::indexOfValue(const double value) const {
  return NumericAxis::indexOfValue(value, m_values);
}

} // namespace API
} // namespace Mantid
