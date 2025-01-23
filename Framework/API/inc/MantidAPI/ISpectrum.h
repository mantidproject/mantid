// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/cow_ptr.h"

#include <set>

class SpectrumTester;
namespace Mantid {
namespace DataObjects {
class Histogram1D;
class EventList;
} // namespace DataObjects
namespace API {
class MatrixWorkspace;

/** A "spectrum" is an object that holds the data for a particular spectrum,
 * in particular:
 *  - The X/Y/E arrays
 *  - The spectrum number
 *  - A list of detector ids associated with it.
 *
 * This is an interface that can be used for both Workspace2D's Spectrum
 objects,
 * and EventWorkspace's EventList objects

  @author Janik Zikovsky
  @date 2011-07-01
*/
class MANTID_API_DLL ISpectrum {
public:
  ISpectrum() = default;
  ISpectrum(const specnum_t specNo);
  virtual ~ISpectrum() = default;

  void copyInfoFrom(const ISpectrum &other);

  /// Copy data from another ISpectrum with double-dynamic dispatch.
  virtual void copyDataFrom(const ISpectrum &source) = 0;

  virtual void setX(const Kernel::cow_ptr<HistogramData::HistogramX> &X) = 0;
  virtual MantidVec &dataX() = 0;
  virtual const MantidVec &dataX() const = 0;
  virtual const MantidVec &readX() const = 0;
  virtual Kernel::cow_ptr<HistogramData::HistogramX> ptrX() const = 0;

  virtual MantidVec &dataDx() = 0;
  virtual const MantidVec &dataDx() const = 0;
  virtual const MantidVec &readDx() const = 0;

  virtual void clearData() = 0;

  virtual MantidVec &dataY() = 0;
  virtual MantidVec &dataE() = 0;

  virtual const MantidVec &dataY() const = 0;
  virtual const MantidVec &dataE() const = 0;
  virtual const MantidVec &readY() const;
  virtual const MantidVec &readE() const;

  virtual size_t getMemorySize() const = 0;

  virtual std::pair<double, double> getXDataRange() const;
  // ---------------------------------------------------------
  void addDetectorID(const detid_t detID);
  void addDetectorIDs(const std::set<detid_t> &detIDs);
  void addDetectorIDs(const std::vector<detid_t> &detIDs);
  void setDetectorID(const detid_t detID);
  void setDetectorIDs(const std::set<detid_t> &detIDs);
  void setDetectorIDs(std::set<detid_t> &&detIDs);

  bool hasDetectorID(const detid_t detID) const;
  const std::set<detid_t> &getDetectorIDs() const;

  void clearDetectorIDs();

  // ---------------------------------------------------------
  specnum_t getSpectrumNo() const;

  void setSpectrumNo(specnum_t num);

  bool hasDx() const;
  void resetHasDx();

  /// Returns the Histogram associated with this spectrum.
  virtual HistogramData::Histogram histogram() const { return histogramRef(); }
  /// Sets the Histogram associated with this spectrum.
  template <typename... T> void setHistogram(T &&...data) {
    HistogramData::Histogram hist(std::forward<T>(data)...);
    // Check for the special case EventList, it only accepts histograms without
    // Y and E data.
    checkAndSanitizeHistogram(hist);
    mutableHistogramRef() = std::move(hist);
  }

  HistogramData::Histogram::YMode yMode() const { return histogramRef().yMode(); }
  void setYMode(HistogramData::Histogram::YMode ymode) { mutableHistogramRef().setYMode(ymode); }
  void convertToCounts() {
    checkIsYAndEWritable();
    mutableHistogramRef().convertToCounts();
  }
  void convertToFrequencies() {
    checkIsYAndEWritable();
    mutableHistogramRef().convertToFrequencies();
  }

  HistogramData::BinEdges binEdges() const { return histogramRef().binEdges(); }
  HistogramData::Points points() const { return histogramRef().points(); }
  HistogramData::PointStandardDeviations pointStandardDeviations() const {
    return histogramRef().pointStandardDeviations();
  }
  template <typename... T> void setBinEdges(T &&...data) & {
    mutableHistogramRef().setBinEdges(std::forward<T>(data)...);
  }
  template <typename... T> void setPoints(T &&...data) & {
    // Check for the special case EventList, it only works with BinEdges.
    checkWorksWithPoints();
    mutableHistogramRef().setPoints(std::forward<T>(data)...);
  }
  template <typename... T> void setPointVariances(T &&...data) & {
    // Note that we can set point variances even if storage mode is BinEdges, Dx
    // is *always* one value *per bin*.
    mutableHistogramRef().setPointVariances(std::forward<T>(data)...);
  }
  template <typename... T> void setPointStandardDeviations(T &&...data) & {
    mutableHistogramRef().setPointStandardDeviations(std::forward<T>(data)...);
  }
  virtual HistogramData::Counts counts() const { return histogramRef().counts(); }
  virtual HistogramData::CountVariances countVariances() const { return histogramRef().countVariances(); }
  virtual HistogramData::CountStandardDeviations countStandardDeviations() const {
    return histogramRef().countStandardDeviations();
  }
  virtual HistogramData::Frequencies frequencies() const { return histogramRef().frequencies(); }
  virtual HistogramData::FrequencyVariances frequencyVariances() const { return histogramRef().frequencyVariances(); }
  virtual HistogramData::FrequencyStandardDeviations frequencyStandardDeviations() const {
    return histogramRef().frequencyStandardDeviations();
  }
  template <typename... T> void setCounts(T &&...data) & {
    // Check for the special case EventList, cannot set Y and E there.
    checkIsYAndEWritable();
    mutableHistogramRef().setCounts(std::forward<T>(data)...);
  }
  template <typename... T> void setCountVariances(T &&...data) & {
    checkIsYAndEWritable();
    mutableHistogramRef().setCountVariances(std::forward<T>(data)...);
  }
  template <typename... T> void setCountStandardDeviations(T &&...data) & {
    checkIsYAndEWritable();
    mutableHistogramRef().setCountStandardDeviations(std::forward<T>(data)...);
  }
  template <typename... T> void setFrequencies(T &&...data) & {
    checkIsYAndEWritable();
    mutableHistogramRef().setFrequencies(std::forward<T>(data)...);
  }
  template <typename... T> void setFrequencyVariances(T &&...data) & {
    checkIsYAndEWritable();
    mutableHistogramRef().setFrequencyVariances(std::forward<T>(data)...);
  }
  template <typename... T> void setFrequencyStandardDeviations(T &&...data) & {
    checkIsYAndEWritable();
    mutableHistogramRef().setFrequencyStandardDeviations(std::forward<T>(data)...);
  }
  const HistogramData::HistogramX &x() const { return histogramRef().x(); }
  virtual const HistogramData::HistogramY &y() const { return histogramRef().y(); }
  virtual const HistogramData::HistogramE &e() const { return histogramRef().e(); }
  const HistogramData::HistogramDx &dx() const { return histogramRef().dx(); }
  HistogramData::HistogramX &mutableX() & { return mutableHistogramRef().mutableX(); }
  HistogramData::HistogramDx &mutableDx() & { return mutableHistogramRef().mutableDx(); }
  HistogramData::HistogramY &mutableY() & {
    checkIsYAndEWritable();
    return mutableHistogramRef().mutableY();
  }
  HistogramData::HistogramE &mutableE() & {
    checkIsYAndEWritable();
    return mutableHistogramRef().mutableE();
  }
  Kernel::cow_ptr<HistogramData::HistogramX> sharedX() const { return histogramRef().sharedX(); }
  virtual Kernel::cow_ptr<HistogramData::HistogramY> sharedY() const { return histogramRef().sharedY(); }
  virtual Kernel::cow_ptr<HistogramData::HistogramE> sharedE() const { return histogramRef().sharedE(); }
  Kernel::cow_ptr<HistogramData::HistogramDx> sharedDx() const { return histogramRef().sharedDx(); }
  void setSharedX(const Kernel::cow_ptr<HistogramData::HistogramX> &x) & { mutableHistogramRef().setSharedX(x); }
  void setSharedDx(const Kernel::cow_ptr<HistogramData::HistogramDx> &dx) & { mutableHistogramRef().setSharedDx(dx); }
  void setSharedY(const Kernel::cow_ptr<HistogramData::HistogramY> &y) & {
    checkIsYAndEWritable();
    mutableHistogramRef().setSharedY(y);
  }
  void setSharedE(const Kernel::cow_ptr<HistogramData::HistogramE> &e) & {
    checkIsYAndEWritable();
    mutableHistogramRef().setSharedE(e);
  }

  void resize(size_t n) { mutableHistogramRef().resize(n); }
  size_t size() const { return histogramRef().size(); }

  void setMatrixWorkspace(MatrixWorkspace *matrixWorkspace, const size_t index);

  virtual void copyDataInto(DataObjects::EventList &) const;
  virtual void copyDataInto(DataObjects::Histogram1D &) const;
  virtual void copyDataInto(SpectrumTester &) const;

protected:
  virtual void checkAndSanitizeHistogram(HistogramData::Histogram &) {};
  virtual void checkWorksWithPoints() const {}
  virtual void checkIsYAndEWritable() const {}

  // Copy and move are not public since this is an abstract class, but protected
  // such that derived classes can implement copy and move.
  ISpectrum(const ISpectrum &other);
  ISpectrum(ISpectrum &&other);
  ISpectrum &operator=(const ISpectrum &other);
  ISpectrum &operator=(ISpectrum &&other);

private:
  virtual const HistogramData::Histogram &histogramRef() const = 0;
  virtual HistogramData::Histogram &mutableHistogramRef() = 0;

  void invalidateCachedSpectrumNumbers() const;
  void invalidateSpectrumDefinition() const;
  MatrixWorkspace *m_matrixWorkspace{nullptr};
  // The default value is meaningless. This will always be set before use.
  size_t m_index{0};

  /// The spectrum number of this spectrum
  specnum_t m_specNo{0};

  /// Set of the detector IDs associated with this spectrum
  std::set<detid_t> detectorIDs;
};

} // namespace API
} // namespace Mantid
