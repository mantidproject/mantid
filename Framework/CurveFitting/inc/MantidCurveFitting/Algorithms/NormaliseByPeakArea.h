#ifndef MANTID_CURVEFITTING_NORMALISEBYPEAKAREA_H_
#define MANTID_CURVEFITTING_NORMALISEBYPEAKAREA_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {

namespace HistogramData {
class HistogramY;
class HistogramE;
} // namespace HistogramData

namespace CurveFitting {
namespace Algorithms {

/**

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
class DLLExport NormaliseByPeakArea : public API::Algorithm {
public:
  NormaliseByPeakArea();

  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Normalises the input data by the area of of peak defined by the "
           "input mass value.";
  }

  const std::vector<std::string> seeAlso() const override {
    return {"MonitorEfficiencyCorUser", "Divide"};
  }

  int version() const override;
  const std::string category() const override;

private:
  void init() override;
  void exec() override;

  /// Check and store appropriate input data
  void retrieveInputs();
  /// Create the output workspaces
  void createOutputWorkspaces(const API::MatrixWorkspace_sptr &yspaceIn);
  /// Set the units meta-data
  void setUnitsToMomentum(const API::MatrixWorkspace_sptr &workspace);

  /// Convert input workspace to Y coordinates for fitting
  API::MatrixWorkspace_sptr convertInputToY();
  /// Fit the mass peak & find the area value
  double fitToMassPeak(const API::MatrixWorkspace_sptr &yspace,
                       const size_t index);
  /// Normalise given TOF spectrum
  void normaliseTOFData(const double area, const size_t index);
  /// Stores/accumulates the results
  void saveToOutput(const API::MatrixWorkspace_sptr &accumWS,
                    const Kernel::cow_ptr<HistogramData::HistogramY> &yValues,
                    const Kernel::cow_ptr<HistogramData::HistogramE> &eValues,
                    const size_t index);
  /// Symmetrises the data in yspace about the origin
  void symmetriseYSpace();

  API::MatrixWorkspace_sptr m_inputWS;
  /// The input mass in AMU
  double m_mass;
  /// Flag to indicate if results are to be summed
  bool m_sumResults;
  /// Normalised output in TOF
  API::MatrixWorkspace_sptr m_normalisedWS;
  /// Input data converted (and possible summed) to Y space
  API::MatrixWorkspace_sptr m_yspaceWS;
  /// Fitted output
  API::MatrixWorkspace_sptr m_fittedWS;
  /// Fitted output
  API::MatrixWorkspace_sptr m_symmetrisedWS;

  /// Reporting
  API::Progress *m_progress;
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_NORMALISEBYPEAKAREA_H_ */
