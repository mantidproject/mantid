//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/SpectraAxis.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/ISpectraDetectorMap.h"

#include <boost/lexical_cast.hpp>
#include <iostream>

namespace Mantid
{
namespace API
{

using std::size_t;

/**
 * Constructor taking a length and optional flag for initialization
 * @param length :: The length of the axis
 * @param initWithDefaults :: If true the axis values will be initialized 
 * with values from 1->length
 */
SpectraAxis::SpectraAxis(const std::size_t& length, const bool initWithDefaults ): Axis()
{
  m_values.resize(length);
  if( initWithDefaults )
  {
    // For small axes there is no point in the additional thread overhead
    PARALLEL_FOR_IF((length > 1000)) 
    for(specid_t i = 0; i < specid_t(length); ++i)
    {
      m_values[i] = i + 1;
    }
  }
}

/**
 * Constructor taking a reference to an ISpectraDetectorMap implementation. The 
 * axis is initialized to first length unique spectra values provided by the map
 * @param length :: The length of the axis
 * @param spectramap :: A reference to an ISpectraDetectorMap implementation.
 */
SpectraAxis::SpectraAxis(const std::size_t length, const Geometry::ISpectraDetectorMap & spectramap) :
  Axis()
{
  m_values.resize(length);
  if( length == 0 ) return;
  Geometry::ISpectraDetectorMap::const_iterator itr = spectramap.cbegin();
  Geometry::ISpectraDetectorMap::const_iterator iend = spectramap.cend();
  if( itr == iend ) 
  {
      m_values.resize(0);
      return;
  }
  // it->first = the spectrum number
  specid_t previous = itr->first;
  m_values[0] = previous;
  ++itr;
  size_t index(1);
  for(; itr != iend; ++itr)
  {
    if( index == length ) break;
    const specid_t current = itr->first;
    if( current != previous )
    {
      // the spectrum number (in the iterator) just changed.
      // save this new spectrum number in the SpectraAxis
      m_values[index] = current;
      previous = current;
      // go to the next workspace index
      ++index;
    }
  }
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

Axis* SpectraAxis::clone(const std::size_t length, const MatrixWorkspace* const parentWorkspace)
{
  UNUSED_ARG(parentWorkspace)
    SpectraAxis * newAxis = new SpectraAxis(*this);
  newAxis->m_values.clear();
  newAxis->m_values.resize(length);
  return newAxis;
}

/** Get the axis value at the position given
 *  @param  index The position along the axis for which the value is required
 *  @param  verticalIndex Needed for the subclass (RefAxis) method, but ignored (and defaulted) here
 *  @return The value of the axis as a double
 *  @throw  IndexError If the index requested is not in the range of this axis
 */
double SpectraAxis::operator()(const std::size_t& index, const std::size_t& verticalIndex) const
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
void SpectraAxis::setValue(const std::size_t& index, const double& value)
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
const specid_t& SpectraAxis::spectraNo(const std::size_t& index) const
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
specid_t& SpectraAxis::spectraNo(const std::size_t& index)
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
    map.insert(std::make_pair(i, m_values[i]));
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
    map.insert(std::make_pair(m_values[i],i));
  }
}

/** 
 * Populate the SpectraAxis with a 1:1 map from start to end (inclusive).
 */
void SpectraAxis::populateOneToOne(int64_t start, int64_t end)
{
  if( start > end )
  {
    throw std::invalid_argument("SpectraAxis::populateOneToOne - start > end");
  }
  const size_t nvalues(end-start+1);
  m_values.resize(nvalues, 0);
  for (size_t i=0; i < nvalues; i++)
  {
    m_values[i] = static_cast<specid_t>(start+i);
  }
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
std::string SpectraAxis::label(const std::size_t& index)const
{
  return "sp-" + boost::lexical_cast<std::string>(spectraNo(index));
}

} // namespace API
} // namespace Mantid
