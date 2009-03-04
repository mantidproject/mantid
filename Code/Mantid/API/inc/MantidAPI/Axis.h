#ifndef MANTID_API_AXIS_H_
#define MANTID_API_AXIS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidKernel/Unit.h"
#include "boost/shared_ptr.hpp"
#include <string>
#include <vector>

#ifndef HAS_UNORDERED_MAP_H
#include <map>
#else
#include <tr1/unordered_map>
#endif

namespace Mantid
{
namespace API
{
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class MatrixWorkspace;

/** Class to represent the axis of a workspace.

    @author Russell Taylor, Tessella Support Services plc
    @date 16/05/2008

    Copyright &copy; 2008 STFC Rutherford Appleton Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport Axis
{
public:
	#ifndef HAS_UNORDERED_MAP_H
	typedef std::map<int,int> spec2index_map;
	#else
	typedef std::tr1::unordered_map<int,int> spec2index_map;
	#endif
  Axis(const bool type, const int length);
  virtual ~Axis();

  virtual Axis* clone(const MatrixWorkspace* const parentWorkspace = NULL);

  const std::string& title() const;
  std::string& title();

  const Kernel::Unit_sptr& unit() const;
  Kernel::Unit_sptr& unit();

  const bool isSpectra() const;
  const bool isNumeric() const;

  virtual const double operator()(const int index, const int verticalIndex = 0) const;
  virtual void setValue(const int index, const double value);

  const int& spectraNo(const int index) const;
  int& spectraNo(const int index);
  //< Get the length of the axis
  int getLength() const;
  //< Get a map that contains the spectra index as the key and the index in the array as teh value
  void getSpectraIndexMap(spec2index_map&);
  //< Check whether two axis are the same, i.e same length and same spectra_values for all elements in the axis
  bool operator==(const Axis&) const;

protected:
  Axis(const Axis& right);

  /// The length of this axis
  int m_size;

private:
  /// Private, undefined copy assignment operator
  const Axis& operator=(const Axis&);

  /// The user-defined title for this axis
  std::string m_title;
  /// The unit for this axis
  Kernel::Unit_sptr m_unit;
  /// Is this axis of spectra or numeric type? if true it's spectra
  const bool m_isSpectra;
  /// A vector holding the axis values for a spectra axis. Empty otherwise.
  std::vector<int> m_spectraValues;
  /// A vector holding the axis values for a numeric axis. Empty otherwise.
  std::vector<double> m_numericValues;
};

/// Gives the type of an Axis (numeric or spectra)
struct AxisType
{
  enum
  {
    Numeric=0,
    Spectra
  };
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_AXIS_H_*/
