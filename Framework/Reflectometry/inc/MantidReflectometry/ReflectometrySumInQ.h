// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidReflectometry/DllConfig.h"
#include <cmath>

namespace Mantid {
namespace API {
class SpectrumInfo;
}

namespace HistogramData {
class BinEdges;
class Counts;
class CountStandardDeviations;
} // namespace HistogramData
namespace Reflectometry {

/** ReflectometrySumInQ : Sum counts from the input workspace in lambda
  along lines of constant Q by projecting to "virtual lambda" at a
  reference angle.
*/
class MANTID_REFLECTOMETRY_DLL ReflectometrySumInQ : public API::Algorithm {
public:
  struct Angles {
    double horizon{std::numeric_limits<double>::quiet_NaN()};
    double twoTheta{std::numeric_limits<double>::quiet_NaN()};
    double delta{std::numeric_limits<double>::quiet_NaN()};
    size_t referenceWSIndex{0};
  };

  struct MinMax {
    double min{std::numeric_limits<double>::max()};
    double max{std::numeric_limits<double>::lowest()};
    // Do not add noexcept to defaulted constructor here as this
    // causes the constructor to be deleted in clang 6.0.0
    // For more see:
    // https://stackoverflow.com/questions/46866686/default-member-initializer-needed-within-definition-of-enclosing-class-outside
    MinMax() = default;
    MinMax(const double a, const double b) noexcept;
    void testAndSet(const double a) noexcept;
    void testAndSetMax(const double a) noexcept;
    void testAndSetMin(const double a) noexcept;
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
  void processValue(const int inputIdx, const MinMax &twoThetaRange,
                    const Angles &refAngles,
                    const HistogramData::BinEdges &edges,
                    const HistogramData::Counts &counts,
                    const HistogramData::CountStandardDeviations &stdDevs,
                    API::MatrixWorkspace &IvsLam, std::vector<double> &outputE);
  MinMax projectedLambdaRange(const MinMax &wavelengthRange,
                              const MinMax &twoThetaRange,
                              const Angles &refAngles);
  Angles referenceAngles(const API::SpectrumInfo &spectrumInfo);
  API::MatrixWorkspace_sptr sumInQ(const API::MatrixWorkspace &detectorWS,
                                   const Indexing::SpectrumIndexSet &indices);
};

} // namespace Reflectometry
} // namespace Mantid
