//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Axis.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{
namespace API
{

/// Constructor
Axis::Axis(const bool type, const int length) :
  m_size(length), m_title(), m_unit(Kernel::Unit_sptr()),
  m_isSpectra(type), m_spectraValues(), m_numericValues()
{
  if (m_isSpectra)
  {
    m_spectraValues.resize(m_size);
  }
  else
  {
    m_numericValues.resize(m_size);
  }
}

/// Protected copy constructor
Axis::Axis(const Axis& right) :
  m_size(right.m_size), m_title(right.m_title), m_unit(right.m_unit), m_isSpectra(right.m_isSpectra),
  m_spectraValues(right.m_spectraValues), m_numericValues(right.m_numericValues)
{}

/// Virtual destructor
Axis::~Axis()
{}

/** Virtual constructor
 *  @param parentWorkspace Needed for subclass's (RefAxis) clone method
 *  @return A pointer to a copy of the Axis on which the method is called
 */
Axis* Axis::clone(const MatrixWorkspace* const parentWorkspace)
{
  return new Axis(*this);
}

/// Returns the user-defined title for this axis
const std::string& Axis::title() const
{
  return m_title;
}

/// Returns a reference to the user-defined title for this axis
std::string& Axis::title()
{
  return m_title;
}

/** The unit for this axis
 *  @return A shared pointer to the unit object
 */
const Kernel::Unit_sptr& Axis::unit() const
{
  return m_unit;
}

/** The unit object for this workspace (non const version)
 *  @return A shared pointer to the unit object
 */
Kernel::Unit_sptr& Axis::unit()
{
  return m_unit;
}

/// Returns true if this axis hold spectra numbers
const bool Axis::isSpectra() const
{
  return m_isSpectra;
}

/// Returns true if this axis is of numeric type
const bool Axis::isNumeric() const
{
  return (!m_isSpectra);
}

/** Get the axis value at the position given
 *  @param  index The position along the axis for which the value is required
 *  @param  verticalIndex Needed for the subclass (RefAxis) method, but ignored (and defaulted) here
 *  @return The value of the axis as a double
 *  @throw  IndexError If the index requested is not in the range of this axis
 */
const double Axis::operator()(const int index, const int verticalIndex) const
{
  if (index < 0 || index >= m_size)
  {
    throw Kernel::Exception::IndexError(index, m_size-1, "Axis: Index out of range.");
  }

  if (m_isSpectra)
  {
    return static_cast<double>(m_spectraValues[index]);
  }
  else
  {
    return m_numericValues[index];
  }
}

/** Sets the axis value at a given position
 *  @param index The position along the axis for which to set the value
 *  @param value The new value
 *  @throw  IndexError If the index requested is not in the range of this axis
 */
void Axis::setValue(const int index, const double value)
{
  if (index < 0 || index >= m_size)
  {
    throw Kernel::Exception::IndexError(index, m_size-1, "Axis: Index out of range.");
  }

  if (m_isSpectra)
  {
    m_spectraValues[index] = static_cast<int>(value);
  }
  else
  {
    m_numericValues[index] = value;
  }
}

/** Returns the spectrum number at the position given (Spectra axis only)
 *  @param  index The position for which the value is required
 *  @return The spectrum number as an int
 *  @throw  domain_error If this method is called on a numeric axis
 *  @throw  IndexError If the index requested is not in the range of this axis
 */
const int& Axis::spectraNo(const int index) const
{
  if (!m_isSpectra) throw std::domain_error("Cannot call spectraNo() on numeric axes");
  if (index < 0 || index >= m_size)
  {
    throw Kernel::Exception::IndexError(index, m_size-1, "Axis: Index out of range.");
  }

  return m_spectraValues[index];
}

/** Returns a non-const reference to the spectrum number at the position given (Spectra axis only)
 *  @param  index The position for which the value is required
 *  @return The spectrum number as an int
 *  @throw  domain_error If this method is called on a numeric axis
 *  @throw  IndexError If the index requested is not in the range of this axis
 */
int& Axis::spectraNo(const int index)
{
  if (!m_isSpectra) throw std::domain_error("Cannot call spectraNo() on numeric axes");
  if (index < 0 || index >= m_size)
  {
    throw Kernel::Exception::IndexError(index, m_size-1, "Axis: Index out of range.");
  }

  return m_spectraValues[index];
}
/** Returns a map where spectra is the key and index is the value
 * This is used for efficient search of spectra number within a workspace
 *  @param  map Reference to the map
 */
void Axis::getSpectraIndexMap(spec2index_map& map)
{
	if (!isSpectra())
		throw std::runtime_error("getSpectraIndexMap(), not a spectra axis");

	std::size_t nel=m_spectraValues.size();

	if (nel==0)
		throw std::runtime_error("getSpectraIndexMap(),  zero elements");
	map.clear();
	for (int i=nel-1;i>=0;--i)
	{
		map.insert(std::make_pair<int,int>(m_spectraValues[i],i));
	}
}

} // namespace API
} // namespace Mantid
