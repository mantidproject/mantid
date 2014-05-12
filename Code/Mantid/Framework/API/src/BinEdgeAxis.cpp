#include "MantidAPI/BinEdgeAxis.h"

namespace Mantid
{
  namespace API
  {


    //----------------------------------------------------------------------------------------------
    /**
     */
    BinEdgeAxis::BinEdgeAxis(const std::size_t & length) : NumericAxis(length)
    {
    }

    /** Virtual constructor
     *  @param parentWorkspace :: The workspace is not used in this implementation
     *  @returns A pointer to a copy of the NumericAxis on which the method is called
     */
    Axis* BinEdgeAxis::clone(const MatrixWorkspace* const parentWorkspace)
    {
      UNUSED_ARG(parentWorkspace)
      return new BinEdgeAxis(*this);
    }

    /** Virtual constructor
     * @param length A new length for the axis. The values are cleared.
     * @param parentWorkspace The workspace is not used in this implementation
     * @returns A pointer to a copy of the NumericAxis on which the method is called
     */
    Axis* BinEdgeAxis::clone(const std::size_t length, const MatrixWorkspace* const parentWorkspace)
    {
      UNUSED_ARG(parentWorkspace)
      auto * newAxis = new BinEdgeAxis(*this);
      newAxis->m_values.clear();
      newAxis->m_values.resize(length);
      return newAxis;
    }

    /**
     * Return the values axis as they are
     * @return A vector containing the bin boundaries
     */
    std::vector<double> BinEdgeAxis::createBinBoundaries() const
    {
      return this->getValues();
    }

    /**
     * Treats values as bin edges and returns the index of the bin, which
     * the value falls into. The maximum value will always be length() - 1
     * @param value A value on the axis
     * @return The index closest to given value
     * @throws std::out_of_range if the value is out of range of the axis
     */
    size_t BinEdgeAxis::indexOfValue(const double value) const
    {
      size_t edgeIndex = NumericAxis::indexOfValue(value);
      // index of bin centre is one less since the first boundary offsets the whole range
      // need to protect for case where value equals lowest bin boundary as that will return &
      // not 1
      if(edgeIndex > 0) return edgeIndex - 1;
      else return edgeIndex;
    }

  } // namespace API
} // namespace Mantid
