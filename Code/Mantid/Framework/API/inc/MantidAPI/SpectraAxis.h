#ifndef MANTID_API_SpectraAxis_H_
#define MANTID_API_SpectraAxis_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidKernel/Unit.h"
#include "MantidAPI/Axis.h"

#include <boost/shared_ptr.hpp>
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

/** Class to represent the spectra axis of a workspace.

    @author Roman Tolchenov, Tessella plc
    @date 05/07/2010

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport SpectraAxis: public Axis
{
public:
	#ifndef HAS_UNORDERED_MAP_H
    typedef std::map<int64_t,int64_t> spec2index_map; ///< The storage for the spectrum number to index map
	#else
    typedef std::tr1::unordered_map<int64_t,int64_t> spec2index_map;
	#endif
  SpectraAxis(const std::size_t& length);
  virtual ~SpectraAxis(){}
  virtual Axis* clone(const MatrixWorkspace* const parentWorkspace = NULL);
  virtual std::size_t length() const{return m_values.size();}
  /// If this is a spectra Axis - always true for this class
  virtual bool isSpectra() const{return true;}
  virtual double operator()(const std::size_t& index, const std::size_t& verticalIndex = 0) const;
  virtual void setValue(const std::size_t& index, const double& value);
  virtual bool operator==(const Axis&) const;
  std::string label(const std::size_t& index)const;

  const int64_t& spectraNo(const std::size_t& index) const;
  int64_t& spectraNo(const std::size_t& index);
  // Get a map that contains the spectra index as the key and the index in the array as teh value
  void getSpectraIndexMap(spec2index_map&) const;
  void getIndexSpectraMap(spec2index_map& map) const;

  void populateSimple(int64_t end);

private:
  /// Private, undefined copy assignment operator
  const SpectraAxis& operator=(const SpectraAxis&);
  /// A vector holding the axis values for the axis.
  std::vector<int64_t> m_values;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_SpectraAxis_H_ */
