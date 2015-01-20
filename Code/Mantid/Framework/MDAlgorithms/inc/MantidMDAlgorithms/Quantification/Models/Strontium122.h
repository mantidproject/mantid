#ifndef MANTID_MDALGORITHMS_STRONTIUM122_H_
#define MANTID_MDALGORITHMS_STRONTIUM122_H_
/**
  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

  File change history is stored at: <https://github.com/mantidproject/mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#include "MantidMDAlgorithms/Quantification/ForegroundModel.h"

namespace Mantid {
namespace MDAlgorithms {

/**
 * Defines the Strontium-122 model of Ewings et al.
 * This is model 207 in TobyFit.
 */
class DLLExport Strontium122 : public ForegroundModel {
private:
  /// String name of the model
  std::string name() const { return "Strontium122"; }

  /// Setup the model
  void init();
  /// Called when an attribute is set
  void setAttribute(const std::string &name,
                    const API::IFunction::Attribute &attr);

  /// Returns the type of model
  ModelType modelType() const { return Broad; }
  /// Calculates the intensity for the model for the current parameters.
  double scatteringIntensity(const API::ExperimentInfo &exptDescr,
                             const std::vector<double> &point) const;

  /// Twin type attribute
  int m_twinType;
  /// MultEps attribute
  bool m_multEps;

public:
  Strontium122();
};
}
}
#endif /* MANTID_MDALGORITHMS_STRONTIUM122_H_ */
