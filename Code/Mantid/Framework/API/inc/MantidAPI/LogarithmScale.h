#ifndef MANTID_API_LOGARITHMSCALE_H_
#define MANTID_API_LOGARITHMSCALE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include <cmath>

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ITransformScale.h"

namespace Mantid
{
namespace API
{
/*Base class  representing a logarithm scaling transformation acting on a one-dimensional grid domain

  @author Jose Borreguero
  @date Aug/28/2012

  Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  Code Documentation is available at: <http://doxygen.mantidproject.org>.
*/

class MANTID_API_DLL LogarithmScale : public ITransformScale
{
public:
  LogarithmScale() : m_base(M_E) {};
  ~LogarithmScale() {};
  const std::string name() const { return "LogarithmScale"; }
  void transform( std::vector<double> &gd );
  void setBase( double &base);
  /// The scaling transformation. First and last elements of the grid remain unchanged

private:
  double m_base; //base of the logarithm

}; // class LogarithmScale


} // namespace API
} // namespace Mantid

#endif /*MANTID_API_LOGARITHMSCALE_H_*/
