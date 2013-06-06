#ifndef MANTID_ELASTICDIFFSPHERE_H_
#define MANTID_ELASTICDIFFSPHERE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/Jacobian.h"
#include "MantidAPI/ImmutableCompositeFunction.h"
#include "DeltaFunction.h"

namespace Mantid
{
namespace CurveFitting
{
  /**
  @author Jose Borreguero, NScD
  @date 11/14/2011

  Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
  */

class DLLExport ElasticDiffSphere : public DeltaFunction
{
public:

  /// Constructor
  ElasticDiffSphere();

  /// Destructor
  virtual ~ElasticDiffSphere() {};

  /// overwrite IFunction base class methods
  virtual std::string name()const{return "ElasticDiffSphere";}

  /// A rescaling of the peak intensity
  double HeightPrefactor() const;

};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_ELASTICDIFFSPHERE_H_*/
