#ifndef MANTID_ISOROTDIFF_H_
#define MANTID_ISOROTDIFF_H_
// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Mantid Headers from the same project
#include "MantidCurveFitting/Functions/ElasticIsoRotDiff.h"
#include "MantidCurveFitting/Functions/InelasticIsoRotDiff.h"
// Mantid headers from other projects
#include "MantidAPI/IFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ImmutableCompositeFunction.h"
// third party library headers (N/A)
// standard library headers (N/A)

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
@author Jose Borreguero, NScD
@date 25/09/2016

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

class DLLExport IsoRotDiff : public API::ImmutableCompositeFunction {

public:
  /// overwrite IFunction base class methods
  std::string name() const override { return "IsoRotDiff"; }

  /// overwrite IFunction base class methods
  const std::string category() const override { return "QuasiElastic"; }

  /// overwrite IFunction base class methods
  virtual int version() const { return 1; }

  /// Propagate an attribute to member functions
  virtual void trickleDownAttribute(const std::string &name);

  /// Override parent definition
  virtual void declareAttribute(const std::string &name,
                                const API::IFunction::Attribute &defaultValue);

  /// Override parent definition
  void setAttribute(const std::string &name,
                    const API::IFunction::Attribute &att) override;

  /// overwrite IFunction base class method, which declare function parameters
  void init() override;

private:
  boost::shared_ptr<Mantid::CurveFitting::Functions::ElasticIsoRotDiff>
      m_elastic; // elastic intensity of the DiffSphere structure factor
  boost::shared_ptr<Mantid::CurveFitting::Functions::InelasticIsoRotDiff>
      m_inelastic; // inelastic intensity of the DiffSphere structure factor
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_ISOROTDIFF_H_*/
