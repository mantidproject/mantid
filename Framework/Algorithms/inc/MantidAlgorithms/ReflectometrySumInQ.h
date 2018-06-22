#ifndef MANTID_ALGORITHMS_REFLECTOMETRYSUMINQ_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYSUMINQ_H_

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace API {
class SpectrumInfo;
}

namespace HistogramData {
class BinEdges;
class Counts;
class CountStandardDeviations;
}
namespace Algorithms {

/** ReflectometrySumInQ : Sum counts from the input workspace in lambda
  along lines of constant Q by projecting to "virtual lambda" at a
  reference angle.

  Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_ALGORITHMS_DLL ReflectometrySumInQ : public API::Algorithm {
public:
  struct Angles {
    double horizon{std::nan("")};
    double twoTheta{std::nan("")};
    double delta{std::nan("")};
  };

  struct MinMax {
    double min{std::numeric_limits<double>::max()};
    double max{std::numeric_limits<double>::lowest()};
    MinMax() noexcept = default;
    MinMax(const double a, const double b) noexcept;
    void testAndSet(const double a) noexcept;
  };

  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
  API::MatrixWorkspace_sptr
  constructIvsLamWS(const API::MatrixWorkspace &detectorWS,
                    const Indexing::SpectrumIndexSet &indices,
                    const Angles &refAngles);
  MinMax findWavelengthMinMax(const API::MatrixWorkspace &detectorWS,
                              const Indexing::SpectrumIndexSet &indices,
                              const Angles &refAngles);
  void
  processValue(const int inputIdx, const MinMax &twoThetaRange,
               const Angles &refAngles,
               const Mantid::HistogramData::BinEdges &inputX,
               const Mantid::HistogramData::Counts &inputY,
               const Mantid::HistogramData::CountStandardDeviations &inputE,
               API::MatrixWorkspace &IvsLam, std::vector<double> &outputE);
  MinMax projectedLambdaRange(const MinMax &wavelengthRange,
                              const MinMax &twoThetaRange,
                              const Angles &refAngles);
  Angles referenceAngles(const API::SpectrumInfo &spectrumInfo);
  API::MatrixWorkspace_sptr sumInQ(const API::MatrixWorkspace &detectorWS,
                                   const Indexing::SpectrumIndexSet &indices);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_REFLECTOMETRYSUMINQ_H_ */
