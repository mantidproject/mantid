#ifndef MANTID_ALGORITHMS_DETECTOREFFICIENCYCORUSER_H_
#define MANTID_ALGORITHMS_DETECTOREFFICIENCYCORUSER_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/cow_ptr.h"

// Forward declarations
namespace mu {
class Parser;
} // namespace mu

namespace Mantid {

namespace HistogramData {
class Histogram;
class HistogramE;
class HistogramY;
class Points;
} // namespace HistogramData

namespace Algorithms {

/** DetectorEfficiencyCorUser :

 This algorithm will calculate the detector efficiency according to the ILL INX
 program for time-of-flight
 data reduction.

 Formula_eff must be defined in the instrument parameters file.


 Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport DetectorEfficiencyCorUser : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Corrects for detector efficiency. The correction factor is "
           "calculated using an instrument specific formula as a function "
           "of the final neutron energy E_f=E_i-E. Note that the formula "
           "is implemented only for a limited number of TOF instruments.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"DetectorEfficiencyCor"};
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
  void retrieveProperties();
  void correctHistogram(const size_t index, const double eff0, double &e,
                        mu::Parser &parser);

  double evaluate(const mu::Parser &parser) const;

  mu::Parser generateParser(const std::string &formula, double *e) const;

  std::string retrieveFormula(const size_t workspaceIndex);

  /// The user selected (input) workspace
  API::MatrixWorkspace_const_sptr m_inputWS;
  /// The output workspace, maybe the same as the input one
  API::MatrixWorkspace_sptr m_outputWS;
  /// stores the user selected value for incidient energy of the neutrons
  double m_Ei = 0.0;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_DETECTOREFFICIENCYCORUSER_H_ */
