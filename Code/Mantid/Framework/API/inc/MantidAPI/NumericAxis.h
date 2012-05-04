#ifndef MANTID_API_NUMERICAXIS_H_
#define MANTID_API_NUMERICAXIS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/Unit.h"
#include "MantidAPI/Axis.h"

#include <boost/shared_ptr.hpp>
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

/** Class to represent a numeric axis of a workspace.

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
class MANTID_API_DLL NumericAxis: public Axis
{
public:
  NumericAxis(const std::size_t& length);
  virtual ~NumericAxis(){}
  virtual Axis* clone(const MatrixWorkspace* const parentWorkspace = NULL);
  virtual Axis* clone(const std::size_t length, const MatrixWorkspace* const parentWorkspace = NULL);
  ///Is the axis numeric - always true for this class
  virtual bool isNumeric() const{return true;}
  virtual std::size_t length() const{return m_values.size();}
  /// Get a value at the specified index
  virtual double operator()(const std::size_t& index, const std::size_t& verticalIndex = 0) const;
  /// Set the value at a specific index
  virtual void setValue(const std::size_t& index, const double& value);
  virtual bool operator==(const Axis&) const;
  std::string label(const std::size_t& index)const;
  /// Create bin boundaries from the point values
  std::vector<double> createBinBoundaries() const;
  const std::vector<double> & getValues() const;
private:
  /// Private, undefined copy assignment operator
  const NumericAxis& operator=(const NumericAxis&);
  /// A vector holding the axis values for the axis.
  std::vector<double> m_values;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_NUMERICAXIS_H_ */
