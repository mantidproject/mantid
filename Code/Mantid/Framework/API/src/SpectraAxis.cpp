//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/SpectraAxis.h"
#include "MantidKernel/Exception.h"

#include <boost/lexical_cast.hpp>
#include "MantidAPI/SpectraDetectorMap.h"

#include <iostream>

namespace Mantid
{
namespace API
{

using std::size_t;

/// Constructor
SpectraAxis::SpectraAxis(const size_t& length): Axis()
{
  m_values.resize(length);
}

/** Virtual constructor
 *  @param parentWorkspace :: not used in this implementation
 *  @return A pointer to a copy of the SpectraAxis on which the method is called
 */
Axis* SpectraAxis::clone(const MatrixWorkspace* const parentWorkspace)
{
  (void) parentWorkspace; //Avoid compiler warning
  return new SpectraAxis(*this);
}

/** Get the axis value at the position given
 *  @param  index The position along the axis for which the value is required
 *  @param  verticalIndex Needed for the subclass (RefAxis) method, but ignored (and defaulted) here
 *  @return The value of the axis as a double
 *  @throw  IndexError If the index requested is not in the range of this axis
 */
double SpectraAxis::operator()(const size_t& index, const size_t& verticalIndex) const
{
  UNUSED_ARG(verticalIndex)
  if (index >= length())
  {
    throw Kernel::Exception::IndexError(index, length()-1, "SpectraAxis: Index out of range.");
  }

  return static_cast<double>(m_values[index]);
}

/** Sets the axis value at a given position
 *  @param index :: The position along the axis for which to set the value
 *  @param value :: The new value
 *  @throw  IndexError If the index requested is not in the range of this axis
 */
void SpectraAxis::setValue(const size_t& index, const double& value)
{
  if (index >= length())
  {
    throw Kernel::Exception::IndexError(index, length()-1, "SpectraAxis: Index out of range.");
  }

  m_values[index] = static_cast<specid_t>(value);
}

/** Returns the spectrum number at the position given (Spectra axis only)
 *  @param  index The position for which the value is required
 *  @return The spectrum number as an int
 *  @throw  domain_error If this method is called on a numeric axis
 *  @throw  IndexError If the index requested is not in the range of this axis
 */
const specid_t& SpectraAxis::spectraNo(const size_t& index) const
{
  if (index >= length())
  {
    throw Kernel::Exception::IndexError(index, length()-1, "SpectraAxis: Index out of range.");
  }

  return m_values[index];
}

/** Returns a non-const reference to the spectrum number at the position given (Spectra axis only)
 *  @param  index The position for which the value is required
 *  @return The spectrum number as an int
 *  @throw  domain_error If this method is called on a numeric axis
 *  @throw  IndexError If the index requested is not in the range of this axis
 */
specid_t& SpectraAxis::spectraNo(const size_t& index)
{
  if (index >= length())
  {
    throw Kernel::Exception::IndexError(index, length()-1, "SpectraAxis: Index out of range.");
  }

  return m_values[index];
}

/** Returns a map where index is the key and spectra is the value
 *  This is used for efficient search of spectra number within a workspace
 *  @param  map Reference to the map
 */
void SpectraAxis::getIndexSpectraMap(index2spec_map& map)const
{
  size_t nel=m_values.size();

  if (nel==0)
    throw std::runtime_error("getSpectraIndexMap(),  zero elements");
  map.clear();
  for (size_t i=0; i < nel; ++i )
  {
    map.insert(std::make_pair<int64_t,int64_t>(i, m_values[i]));
  }
}


/** Returns a map where spectra is the key and index is the value
 *  This is used for efficient search of spectra number within a workspace
 *  @param  map Reference to the map
 */
void SpectraAxis::getSpectraIndexMap(spec2index_map& map)const
{
  size_t nel=m_values.size();
  
  if (nel==0)
    throw std::runtime_error("getSpectraIndexMap(),  zero elements");
  map.clear();
  for (size_t i=0; i < nel; ++i )
  {
    map.insert(std::make_pair<int64_t,int64_t>(m_values[i],i));
  }
}

/** Populate the SpectraAxis with a simple 1:1 map from 0 to end-1.
 */
void SpectraAxis::populateSimple(int64_t end)
{
  m_values.resize(static_cast<size_t>(end), 0);
  for (int64_t i=0; i < end; i++)
    m_values[i] = static_cast<specid_t>(i);
}


/** Check if two axis defined as spectra or numeric axis are equivalent
 *  @param axis2 :: Reference to the axis to compare to
 *  @return true is self and second axis are equal
 */
bool SpectraAxis::operator==(const Axis& axis2) const
{
	if (length()!=axis2.length())
  {
		return false;
  }
  const SpectraAxis* spec2 = dynamic_cast<const SpectraAxis*>(&axis2);
  if (!spec2)
  {
    return false;
  }
  return std::equal(m_values.begin(),m_values.end(),spec2->m_values.begin());
}

/** Returns a text label which shows the value at index and identifies the
 *  type of the axis.
 *  @param index :: The index of an axis value
 *  @return label of requested axis index
 */
std::string SpectraAxis::label(const size_t& index)const
{
  return "sp-" + boost::lexical_cast<std::string>(spectraNo(index));
}

} // namespace API
} // namespace Mantid
