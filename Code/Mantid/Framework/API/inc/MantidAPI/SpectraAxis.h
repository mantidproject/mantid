#ifndef MANTID_API_SPECTRAAXIS_H_
#define MANTID_API_SPECTRAAXIS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Axis.h"
#include "MantidAPI/SpectraDetectorTypes.h"
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/Unit.h"
#include <string>
#include <vector>

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

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_API_DLL SpectraAxis: public Axis
{
public:
  explicit SpectraAxis(const MatrixWorkspace* const parentWorkspace);
  virtual ~SpectraAxis(){}
  virtual Axis* clone(const MatrixWorkspace* const parentWorkspace);
  virtual Axis* clone(const std::size_t length, const MatrixWorkspace* const parentWorkspace);
  virtual std::size_t length() const;
  /// If this is a spectra Axis - always true for this class
  virtual bool isSpectra() const{return true;}
  virtual double operator()(const std::size_t& index, const std::size_t& verticalIndex = 0) const;
  virtual void setValue(const std::size_t& index, const double& value);
  size_t indexOfValue(const double value) const;
  virtual bool operator==(const Axis&) const;
  std::string label(const std::size_t& index)const;

  specid_t spectraNo(const std::size_t& index) const;
  // Get a map that contains the spectra index as the key and the index in the array as teh value
  void getSpectraIndexMap(spec2index_map&) const;

  double getMin() const;
  double getMax() const;

private:
  /// Default constructor
  SpectraAxis();
  /// Private, undefined copy constructor
  SpectraAxis(const SpectraAxis&);
  /// Private, undefined copy assignment operator
  const SpectraAxis& operator=(const SpectraAxis&);
  /// A pointer to the workspace holding the axis
  const MatrixWorkspace* const m_parentWS;
  /// List of edge values for quick searching of values as if this is binned data
  mutable std::vector<double> m_edges;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_SpectraAxis_H_ */
