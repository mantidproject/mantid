//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/SpectraAxis.h"
#include "MantidKernel/Exception.h"

#include <boost/lexical_cast.hpp>

namespace Mantid
{
namespace API
{

/// Constructor
SpectraAxis::SpectraAxis(const int& length): Axis()
{
  m_values.resize(length);
}

/** Virtual constructor
 *  @param parentWorkspace not used in this implementation
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
double SpectraAxis::operator()(const int& index, const int& verticalIndex) const
{
  (void) verticalIndex; //Avoid compiler warning
  if (index < 0 || index >= length())
  {
    throw Kernel::Exception::IndexError(index, length()-1, "SpectraAxis: Index out of range.");
  }

  return static_cast<double>(m_values[index]);
}

/** Sets the axis value at a given position
 *  @param index The position along the axis for which to set the value
 *  @param value The new value
 *  @throw  IndexError If the index requested is not in the range of this axis
 */
void SpectraAxis::setValue(const int& index, const double& value)
{
  if (index < 0 || index >= length())
  {
    throw Kernel::Exception::IndexError(index, length()-1, "SpectraAxis: Index out of range.");
  }

  m_values[index] = static_cast<int>(value);
}

/** Returns the spectrum number at the position given (Spectra axis only)
 *  @param  index The position for which the value is required
 *  @return The spectrum number as an int
 *  @throw  domain_error If this method is called on a numeric axis
 *  @throw  IndexError If the index requested is not in the range of this axis
 */
const int& SpectraAxis::spectraNo(const int& index) const
{
  if (index < 0 || index >= length())
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
int& SpectraAxis::spectraNo(const int& index)
{
  if (index < 0 || index >= length())
  {
    throw Kernel::Exception::IndexError(index, length()-1, "SpectraAxis: Index out of range.");
  }

  return m_values[index];
}

/** Returns a map where index is the key and spectra is the value
 *  This is used for efficient search of spectra number within a workspace
 *  @param  map Reference to the map
 */
void SpectraAxis::getIndexSpectraMap(spec2index_map& map)const
{
  std::size_t nel=m_values.size();

  if (nel==0)
    throw std::runtime_error("getSpectraIndexMap(),  zero elements");
  map.clear();
  for (int i=nel-1;i>=0;--i)
  {
    map.insert(std::make_pair<int,int>(i, m_values[i]));
  }
}


/** Returns a map where spectra is the key and index is the value
 *  This is used for efficient search of spectra number within a workspace
 *  @param  map Reference to the map
 */
void SpectraAxis::getSpectraIndexMap(spec2index_map& map)const
{
  std::size_t nel=m_values.size();

  if (nel==0)
    throw std::runtime_error("getSpectraIndexMap(),  zero elements");
  map.clear();
  for (int i=nel-1;i>=0;--i)
  {
    map.insert(std::make_pair<int,int>(m_values[i],i));
  }
}

/** Populate the SpectraAxis with a simple 1:1 map from 0 to end-1.
 */
void SpectraAxis::populateSimple(int end)
{
  m_values.resize(static_cast<size_t>(end), 0);
  for (int i=0; i < end; i++)
    m_values[i] = i;
}


/** Check if two axis defined as spectra or numeric axis are equivalent
 *  @param axis2 Reference to the axis to compare to
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
 *  @param index The index of an axis value
 */
std::string SpectraAxis::label(const int& index)const
{
  return "sp-" + boost::lexical_cast<std::string>(spectraNo(index));
}

} // namespace API
} // namespace Mantid
