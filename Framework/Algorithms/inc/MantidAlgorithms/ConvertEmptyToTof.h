#ifndef MANTID_ALGORITHMS_CONVERTEMPTYTOTOF_H_
#define MANTID_ALGORITHMS_CONVERTEMPTYTOTOF_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/System.h"

#include <utility> // std::pair

namespace Mantid {
namespace API {
class SpectrumInfo;
}
namespace Algorithms {

/** ConvertEmptyToTof :

 At the ILL the data is loaded in raw format : no units used. The X-axis
 represent the time channel number.
 This algorithm converts the channel number to time of flight

 Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport ConvertEmptyToTof : public API::Algorithm,
                                    public API::DeprecatedAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Converts the channel number to time of flight.";
  }

private:
  void init() override;
  void exec() override;

  void validateWorkspaceIndices(std::vector<int> &v);
  void validateChannelIndices(std::vector<int> &v);

  std::map<int, int> findElasticPeakPositions(const std::vector<int> &,
                                              const std::vector<int> &);

  void estimateFWHM(const Mantid::HistogramData::HistogramY &, double &,
                    double &, double &, double &, double &);

  bool doFitGaussianPeak(int, double &, double &, double &, double, double);
  std::pair<int, double> findAverageEppAndEpTof(const std::map<int, int> &);

  double calculateTOF(double, double);
  bool areEqual(double, double, double);
  int roundUp(double);
  std::vector<double> makeTofAxis(int, double, size_t, double);
  void setTofInWS(const std::vector<double> &, API::MatrixWorkspace_sptr);

  DataObjects::Workspace2D_sptr m_inputWS;
  API::MatrixWorkspace_sptr m_outputWS;
  // Provide hint to compiler that this should be default initialized to nullptr
  const API::SpectrumInfo *m_spectrumInfo = nullptr;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CONVERTEMPTYTOTOF_H_ */
