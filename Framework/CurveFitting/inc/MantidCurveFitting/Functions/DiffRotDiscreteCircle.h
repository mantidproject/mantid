#ifndef MANTID_DIFFROTDISCRETECIRCLE_H_
#define MANTID_DIFFROTDISCRETECIRCLE_H_

// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Mantid Headers from the same project
#include "MantidCurveFitting/Functions/ElasticDiffRotDiscreteCircle.h"
#include "MantidCurveFitting/Functions/InelasticDiffRotDiscreteCircle.h"
// Mantid headers from other projects
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ImmutableCompositeFunction.h"
// 3rd party library headers (N/A)
// standard library (N/A)

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
@author Jose Borreguero, NScD
@date December 02 2013

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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

/* Class representing the dynamics structure factor of a particle undergoing
 * discrete jumps on N-sites evenly distributed in a circle. The particle can
 * only
 * jump to neighboring sites. This is the most common type of discrete
 * rotational diffusion in a circle.
 */
class DLLExport DiffRotDiscreteCircle : public API::ImmutableCompositeFunction {
public:
  std::string name() const override { return "DiffRotDiscreteCircle"; }

  const std::string category() const override { return "QuasiElastic"; }

  virtual int version() const { return 1; }

  void init() override;

  /// Propagate an attribute to member functions
  virtual void trickleDownAttribute(const std::string &name);

  /// Override parent definition
  virtual void declareAttribute(const std::string &name,
                                const API::IFunction::Attribute &defaultValue);

  /// Override parent definition
  void setAttribute(const std::string &name,
                    const API::IFunction::Attribute &att) override;

private:
  boost::shared_ptr<ElasticDiffRotDiscreteCircle> m_elastic;

  boost::shared_ptr<InelasticDiffRotDiscreteCircle> m_inelastic;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_DIFFROTDISCRETECIRCLE_H_*/
