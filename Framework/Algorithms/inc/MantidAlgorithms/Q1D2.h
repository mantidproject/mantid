// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
namespace API {
class SpectrumInfo;
}
namespace Algorithms {
/** Takes account of the effects of gravity for instruments where the y-axis
   points upwards, for
    example SANS instruments

    @author Steve Williams ISIS Rutherford Appleton Laboratory
    @date 10/12/2010
*/
class MANTID_ALGORITHMS_DLL Q1D2 : public API::Algorithm {
public:
  /// Default constructor
  Q1D2();
  /// Algorithm's name
  const std::string name() const override { return "Q1D"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "SANS 1D reduction. Converts a workspace in wavelength into a "
           "workspace of momentum transfer, assuming elastic scattering";
  }

  /// Algorithm's version
  int version() const override { return (2); }
  const std::vector<std::string> seeAlso() const override { return {"Q1DWeighted", "Qxy"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "SANS"; }

protected:
  Parallel::ExecutionMode
  getParallelExecutionMode(const std::map<std::string, Parallel::StorageMode> &storageModes) const override;

private:
  /// the experimental workspace with counts across the detector
  API::MatrixWorkspace_const_sptr m_dataWS;
  bool m_doSolidAngle;

  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  API::MatrixWorkspace_sptr setUpOutputWorkspace(const std::vector<double> &binParams) const;
  // these are the steps that are run on each individual spectrum
  void calculateNormalization(const size_t wavStart, const size_t wsIndex,
                              const API::MatrixWorkspace_const_sptr &pixelAdj,
                              const API::MatrixWorkspace_const_sptr &wavePixelAdj, double const *const binNorms,
                              double const *const binNormEs, HistogramData::HistogramY::iterator norm,
                              HistogramData::HistogramY::iterator normETo2) const;
  void pixelWeight(const API::MatrixWorkspace_const_sptr &pixelAdj, const size_t wsIndex, double &weight,
                   double &error) const;
  void addWaveAdj(const double *c, const double *Dc, HistogramData::HistogramY::iterator bInOut,
                  HistogramData::HistogramY::iterator e2InOut) const;
  void addWaveAdj(const double *c, const double *Dc, HistogramData::HistogramY::iterator bInOut,
                  HistogramData::HistogramY::iterator e2InOut,
                  HistogramData::HistogramY::const_iterator wavePixelAdjData,
                  HistogramData::HistogramE::const_iterator wavePixelAdjError) const;
  void normToMask(const size_t offSet, const size_t wsIndex, const HistogramData::HistogramY::iterator theNorms,
                  const HistogramData::HistogramY::iterator errorSquared) const;
  void convertWavetoQ(const API::SpectrumInfo &spectrumInfo, const size_t wsInd, const bool doGravity,
                      const size_t offset, HistogramData::HistogramY::iterator Qs, const double extraLength) const;
  void getQBinPlus1(const HistogramData::HistogramX &OutQs, const double QToFind,
                    HistogramData::HistogramY::const_iterator &loc) const;
  void normalize(const HistogramData::HistogramY &normSum, const HistogramData::HistogramE &normError2,
                 HistogramData::HistogramY &counts, HistogramData::HistogramE &errors) const;
};

} // namespace Algorithms
} // namespace Mantid
