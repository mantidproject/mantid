#ifndef MANTID_DIFFSPHERE_H_
#define MANTID_DIFFSPHERE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/Jacobian.h"
#include "MantidAPI/ImmutableCompositeFunction.h"
#include "DeltaFunction.h"
#include "MantidCurveFitting/ElasticDiffSphere.h"
#include "MantidCurveFitting/InelasticDiffSphere.h"

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

class DLLExport DiffSphere : public API::ImmutableCompositeFunction
{
public:
  /// Constructor
  DiffSphere();

  /// Destructor
  ~DiffSphere() {};

  /// overwrite IFunction base class methods
  std::string name()const{return "DiffSphere";}

  virtual const std::string category() const { return "QuasiElastic";}

  virtual int version() const { return 1;}

  /// Propagate an attribute to member functions
  virtual void trickleDownAttribute( const std::string& name );

  /// Override parent definition
  virtual void declareAttribute(const std::string & name,const API::IFunction::Attribute & defaultValue);

  /// Override parent definition
  virtual void setAttribute(const std::string& attName,const Attribute& att);

private:
  //API::IFunctionMW* m_elastic;    //elastic intensity of the DiffSphere structure factor
  boost::shared_ptr<ElasticDiffSphere> m_elastic;
  boost::shared_ptr<InelasticDiffSphere> m_inelastic;  //inelastic intensity of the DiffSphere structure factor
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_DIFFSPHERE_H_*/
