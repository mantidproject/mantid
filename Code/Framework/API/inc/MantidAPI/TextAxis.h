#ifndef MANTID_API_TEXTAXIS_H_
#define MANTID_API_TEXTAXIS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidKernel/Unit.h"
#include "MantidAPI/Axis.h"

#include "boost/shared_ptr.hpp"
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

/** Class to represent a text axis of a workspace.

    @author Roman Tolchenov, Tessella plc
    @date 06/07/2010

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
class DLLExport TextAxis: public Axis
{
public:
  TextAxis(const int& length);
  virtual ~TextAxis(){}
  virtual Axis* clone(const MatrixWorkspace* const parentWorkspace = NULL);
  virtual int length() const{return static_cast<int>(m_values.size());}
  /// If this is a TextAxis, always return true for this class
  virtual bool isText() const{return true;}
  virtual double operator()(const int& index, const int& verticalIndex = 0) const;
  virtual void setValue(const int& index, const double& value);
  virtual bool operator==(const Axis&) const;
  std::string label(const int& index)const;
  void setLabel(const int& index, const std::string& lbl);
private:
  /// Private, undefined copy assignment operator
  const TextAxis& operator=(const TextAxis&);
  /// A vector holding the axis values for the axis.
  std::vector<std::string> m_values;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_TEXTAXIS_H_ */
