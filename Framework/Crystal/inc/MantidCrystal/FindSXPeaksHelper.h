// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_FINDSXPEAKSHELPER_H_
#define MANTID_CRYSTAL_FINDSXPEAKSHELPER_H_

#include "MantidAPI/SpectrumInfo.h"
#include "MantidHistogramData/HistogramX.h"
#include "MantidHistogramData/HistogramY.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Unit.h"

#include <boost/optional.hpp>
#include <iterator>
#include <vector>

namespace Mantid {
namespace Kernel {
class ProgressBase;
}
} // namespace Mantid

namespace Mantid {
namespace Crystal {
namespace FindSXPeaksHelper {

/// enum to determine the units of the workspaces X axis we are searching in
enum class XAxisUnit { TOF, DSPACING };

/* ------------------------------------------------------------------------------------------
 * Single Crystal peak representation
 * ------------------------------------------------------------------------------------------
 */
struct DLLExport SXPeak {
public:
  SXPeak(double t, double phi, double intensity,
         const std::vector<int> &spectral, const size_t wsIndex,
         const Mantid::API::SpectrumInfo &spectrumInfo);

  /// Object comparison. Note that the tolerance is relative and used for all
  /// three traits.
  bool compare(const SXPeak &rhs, double tolerance) const;

  /// Object comparison. Note that the tolerances are absolute and there is one
  /// per trait.
  bool compare(const SXPeak &rhs, const double xTolerance,
               const double phiTolerance, const double thetaTolerance,
               const XAxisUnit tofUnits = XAxisUnit::TOF) const;

  /// Getter for LabQ
  Mantid::Kernel::V3D getQ() const;

  /// Operator addition overload
  SXPeak &operator+=(const SXPeak &rhs);

  /// Normalise by number of pixels
  void reduce();

  friend std::ostream &operator<<(std::ostream &os, const SXPeak &rhs) {
    os << rhs.m_tof << "," << rhs.m_twoTheta << "," << rhs.m_phi << ","
       << rhs.m_intensity << "\n";
    os << " Spectra";
    std::copy(rhs.m_spectra.begin(), rhs.m_spectra.end(),
              std::ostream_iterator<int>(os, ","));
    return os;
  }

  const double &getIntensity() const;

  detid_t getDetectorId() const;

private:
  /// TOF for the peak centre
  double m_tof;
  /// d-spacing at the peak centre
  double m_dSpacing;
  /// 2 * theta angle for then centre detector of the peak
  double m_twoTheta;
  /// Phi angle for the centre detector of the peak
  double m_phi;
  /// Measured intensity of centre of the peak
  double m_intensity;
  /// Contributing spectra to this peak
  std::vector<int> m_spectra;
  /// Detector-sample distance
  double m_LTotal;
  /// Detector workspace index
  size_t m_wsIndex;
  /// Detector ID
  detid_t m_detId;
  /// Number of contributing pixels
  int m_nPixels;
  /// Unit vector in the direction of the wavevector
  Kernel::V3D m_unitWaveVector;
  /// Q Convention
  std::string m_qConvention;
};

using yIt = Mantid::HistogramData::HistogramY::const_iterator;
using Bound = HistogramData::HistogramX::const_iterator;
using BoundsIterator = std::pair<Bound, Bound>;
using PeakList = boost::optional<std::vector<SXPeak>>;

class PeakContainer {
public:
  PeakContainer(const HistogramData::HistogramY &y);
  void startRecord(yIt item);
  void stopRecord(yIt item);
  void record(yIt item);
  size_t getNumberOfPointsInPeak() const;
  yIt getMaxIterator() const;

private:
  const HistogramData::HistogramY &m_y;
  size_t m_startIndex;
  size_t m_stopIndex;
  size_t m_maxIndex;
  double m_maxSignal = -1.;
};

/* ------------------------------------------------------------------------------------------
 * Background strategy
 * ------------------------------------------------------------------------------------------
 */
struct DLLExport BackgroundStrategy {
  virtual ~BackgroundStrategy() = default;
  virtual bool isBelowBackground(const double intensity,
                                 const HistogramData::HistogramY &y) const = 0;
};

struct DLLExport AbsoluteBackgroundStrategy : public BackgroundStrategy {
  AbsoluteBackgroundStrategy(const double background);
  bool isBelowBackground(const double intensity,
                         const HistogramData::HistogramY &) const override;

private:
  double m_background = 0.;
};

struct DLLExport PerSpectrumBackgroundStrategy : public BackgroundStrategy {
  PerSpectrumBackgroundStrategy(const double backgroundMultiplier);
  bool isBelowBackground(const double intensity,
                         const HistogramData::HistogramY &y) const override;

private:
  double m_backgroundMultiplier = 1.;
};

/* ------------------------------------------------------------------------------------------
 * Peak Finding Strategy
 * ------------------------------------------------------------------------------------------
 */
class DLLExport PeakFindingStrategy {
public:
  PeakFindingStrategy(const BackgroundStrategy *backgroundStrategy,
                      const API::SpectrumInfo &spectrumInfo,
                      const double minValue = EMPTY_DBL(),
                      const double maxValue = EMPTY_DBL(),
                      const XAxisUnit units = XAxisUnit::TOF);
  virtual ~PeakFindingStrategy() = default;
  PeakList findSXPeaks(const HistogramData::HistogramX &x,
                       const HistogramData::HistogramY &y,
                       const int workspaceIndex) const;

protected:
  BoundsIterator getBounds(const HistogramData::HistogramX &x) const;
  double calculatePhi(size_t workspaceIndex) const;
  double getXValue(const HistogramData::HistogramX &x,
                   const size_t peakLocation) const;
  double convertToTOF(const double xValue, const size_t workspaceIndex) const;
  virtual PeakList dofindSXPeaks(const HistogramData::HistogramX &x,
                                 const HistogramData::HistogramY &y, Bound low,
                                 Bound high,
                                 const int workspaceIndex) const = 0;

  const BackgroundStrategy *m_backgroundStrategy;
  const double m_minValue = EMPTY_DBL();
  const double m_maxValue = EMPTY_DBL();
  const API::SpectrumInfo &m_spectrumInfo;
  const XAxisUnit m_units;
};

class DLLExport StrongestPeaksStrategy : public PeakFindingStrategy {
public:
  StrongestPeaksStrategy(const BackgroundStrategy *backgroundStrategy,
                         const API::SpectrumInfo &spectrumInfo,
                         const double minValue = EMPTY_DBL(),
                         const double maxValue = EMPTY_DBL(),
                         const XAxisUnit units = XAxisUnit::TOF);
  PeakList dofindSXPeaks(const HistogramData::HistogramX &x,
                         const HistogramData::HistogramY &y, Bound low,
                         Bound high, const int workspaceIndex) const override;
};

class DLLExport AllPeaksStrategy : public PeakFindingStrategy {
public:
  AllPeaksStrategy(const BackgroundStrategy *backgroundStrategy,
                   const API::SpectrumInfo &spectrumInfo,
                   const double minValue = EMPTY_DBL(),
                   const double maxValue = EMPTY_DBL(),
                   const XAxisUnit units = XAxisUnit::TOF);
  PeakList dofindSXPeaks(const HistogramData::HistogramX &x,
                         const HistogramData::HistogramY &y, Bound low,
                         Bound high, const int workspaceIndex) const override;

private:
  std::vector<std::unique_ptr<PeakContainer>>
  getAllPeaks(const HistogramData::HistogramX &x,
              const HistogramData::HistogramY &y, Bound low, Bound high,
              const Mantid::Crystal::FindSXPeaksHelper::BackgroundStrategy
                  *backgroundStrategy) const;
  PeakList
  convertToSXPeaks(const HistogramData::HistogramX &x,
                   const HistogramData::HistogramY &y,
                   const std::vector<std::unique_ptr<PeakContainer>> &peaks,
                   const int workspaceIndex) const;
};

/* ------------------------------------------------------------------------------------------
 * Comparison Strategy
 * ------------------------------------------------------------------------------------------
 */
class DLLExport CompareStrategy {
public:
  virtual ~CompareStrategy() = default;
  virtual bool compare(const SXPeak &lhs, const SXPeak &rhs) const = 0;
};

class DLLExport RelativeCompareStrategy : public CompareStrategy {
public:
  RelativeCompareStrategy(const double resolution);
  bool compare(const SXPeak &lhs, const SXPeak &rhs) const override;

private:
  const double m_resolution;
};

class DLLExport AbsoluteCompareStrategy : public CompareStrategy {
public:
  AbsoluteCompareStrategy(const double tofResolution,
                          const double phiResolution,
                          const double twoThetaResolution,
                          const XAxisUnit units = XAxisUnit::TOF);
  bool compare(const SXPeak &lhs, const SXPeak &rhs) const override;

private:
  const double m_xUnitResolution;
  double m_phiResolution;
  double m_twoThetaResolution;
  const XAxisUnit m_units;
};

/* ------------------------------------------------------------------------------------------
 * PeakListReduction Strategy
 * ------------------------------------------------------------------------------------------
 */
class DLLExport ReducePeakListStrategy {
public:
  ReducePeakListStrategy(const CompareStrategy *compareStrategy);
  virtual ~ReducePeakListStrategy() = default;
  virtual std::vector<SXPeak>
  reduce(const std::vector<SXPeak> &peaks,
         Mantid::Kernel::ProgressBase &progress) const = 0;

protected:
  const CompareStrategy *m_compareStrategy;
};

class DLLExport SimpleReduceStrategy : public ReducePeakListStrategy {
public:
  SimpleReduceStrategy(const CompareStrategy *compareStrategy);
  std::vector<SXPeak>
  reduce(const std::vector<SXPeak> &peaks,
         Mantid::Kernel::ProgressBase &progress) const override;
};

class DLLExport FindMaxReduceStrategy : public ReducePeakListStrategy {
public:
  FindMaxReduceStrategy(const CompareStrategy *compareStrategy);
  std::vector<SXPeak>
  reduce(const std::vector<SXPeak> &peaks,
         Mantid::Kernel::ProgressBase &progress) const override;

private:
  std::vector<std::vector<SXPeak *>>
  getPeakGroups(const std::vector<SXPeak> &peakList,
                Mantid::Kernel::ProgressBase &progress) const;
  std::vector<SXPeak>
  getFinalPeaks(const std::vector<std::vector<SXPeak *>> &peakGroups) const;
};

} // namespace FindSXPeaksHelper
} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_FINDSXPEAKSHELPER_H_ */
