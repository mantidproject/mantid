// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_HISTOGRAMDATA_HISTOGRAM_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAM_H_

#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/CountStandardDeviations.h"
#include "MantidHistogramData/CountVariances.h"
#include "MantidHistogramData/Counts.h"
#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/Frequencies.h"
#include "MantidHistogramData/FrequencyStandardDeviations.h"
#include "MantidHistogramData/FrequencyVariances.h"
#include "MantidHistogramData/HistogramDx.h"
#include "MantidHistogramData/HistogramE.h"
#include "MantidHistogramData/HistogramX.h"
#include "MantidHistogramData/HistogramY.h"
#include "MantidHistogramData/PointStandardDeviations.h"
#include "MantidHistogramData/PointVariances.h"
#include "MantidHistogramData/Points.h"
#include "MantidKernel/cow_ptr.h"

#include <vector>

namespace Mantid {
namespace HistogramData {

class HistogramIterator;

/** Histogram

  Histogram is a container for objects that together represent a histogram. In
  the simpest case this are bin edges, counts (e.g., neutron counts from an
  experiment), and statistical uncertainties for these counts. These three
  objects are also referred to as X, Y, and E data of the histogram.

  More generally, the X data in the Histogram can be accessed as bin edges or as
  points (which basically correspond to the bin centers). The Y data in the
  Histogram can be accessed as counts or frequencies (which correspond to the
  counts divided by the bin widths). The E data in the Histogram can be accessed
  as standard deviations or variances (with two cases, uncertainties for counts
  and for frequencies).

  Histogram currently provides a legacy interface alongside with the new and
  recommended interface.

  @author Simon Heybrock
  @date 2016
*/
class MANTID_HISTOGRAMDATA_DLL Histogram {
private:
  Kernel::cow_ptr<HistogramX> m_x;
  Kernel::cow_ptr<HistogramY> m_y{nullptr};
  Kernel::cow_ptr<HistogramE> m_e{nullptr};
  // Mutable until Dx legacy interface is removed.
  mutable Kernel::cow_ptr<HistogramDx> m_dx{nullptr};

public:
  enum class XMode { BinEdges, Points };
  enum class YMode { Uninitialized, Counts, Frequencies };
  /// Construct with given storage mode for X data (BinEdges or Points) and Y
  /// data (Counts or Frequencies).
  explicit Histogram(XMode xmode, YMode ymode)
      : m_x(Kernel::make_cow<HistogramX>(0)), m_xMode(xmode), m_yMode(ymode) {}

  template <class TX, class TY = Counts, class TE = CountVariances>
  explicit Histogram(const TX &x, const TY &y = Counts(),
                     const TE &e = CountVariances());

  // Copy and move need to be declared and defaulted, since we need to have the
  // lvalue reference qualifier on the assignment operators.
  Histogram(const Histogram &) = default;
  Histogram(Histogram &&) = default;
  Histogram &operator=(const Histogram &) & = default;
  Histogram &operator=(Histogram &&) & = default;

  /// Returns the storage mode of the X data (BinEdges or Points).
  XMode xMode() const noexcept { return m_xMode; }

  /// Returns the storage mode of the Y data (Counts or Frequencies).
  YMode yMode() const noexcept { return m_yMode; }

  BinEdges binEdges() const;
  Points points() const;
  PointVariances pointVariances() const;
  PointStandardDeviations pointStandardDeviations() const;
  template <typename... T> void setBinEdges(T &&... data) &;
  template <typename... T> void setPoints(T &&... data) &;
  template <typename... T> void setPointVariances(T &&... data) &;
  template <typename... T> void setPointStandardDeviations(T &&... data) &;

  Counts counts() const;
  CountVariances countVariances() const;
  CountStandardDeviations countStandardDeviations() const;
  Frequencies frequencies() const;
  FrequencyVariances frequencyVariances() const;
  FrequencyStandardDeviations frequencyStandardDeviations() const;
  template <typename... T> void setCounts(T &&... data) &;
  template <typename... T> void setCountVariances(T &&... data) &;
  template <typename... T> void setCountStandardDeviations(T &&... data) &;
  template <typename... T> void setFrequencies(T &&... data) &;
  template <typename... T> void setFrequencyVariances(T &&... data) &;
  template <typename... T> void setFrequencyStandardDeviations(T &&... data) &;

  const HistogramX &x() const { return *m_x; }
  const HistogramY &y() const { return *m_y; }
  const HistogramE &e() const { return *m_e; }
  const HistogramDx &dx() const { return *m_dx; }
  HistogramX &mutableX() & { return m_x.access(); }
  HistogramY &mutableY() & { return m_y.access(); }
  HistogramE &mutableE() & { return m_e.access(); }
  HistogramDx &mutableDx() & { return m_dx.access(); }

  Kernel::cow_ptr<HistogramX> sharedX() const { return m_x; }
  Kernel::cow_ptr<HistogramY> sharedY() const { return m_y; }
  Kernel::cow_ptr<HistogramE> sharedE() const { return m_e; }
  Kernel::cow_ptr<HistogramDx> sharedDx() const { return m_dx; }
  void setSharedX(const Kernel::cow_ptr<HistogramX> &x) &;
  void setSharedY(const Kernel::cow_ptr<HistogramY> &y) &;
  void setSharedE(const Kernel::cow_ptr<HistogramE> &e) &;
  void setSharedDx(const Kernel::cow_ptr<HistogramDx> &Dx) &;

  /// Returns the size of the histogram, i.e., the number of Y data points.
  size_t size() const {
    if (!m_x->empty() && xMode() == XMode::BinEdges)
      return m_x->size() - 1;
    return m_x->size();
  }

  void resize(size_t n);

  // Temporary legacy interface to X
  void setX(const Kernel::cow_ptr<HistogramX> &X) & { m_x = X; }
  MantidVec &dataX() & { return m_x.access().mutableRawData(); }
  const MantidVec &dataX() const & { return m_x->rawData(); }
  const MantidVec &readX() const { return m_x->rawData(); }
  Kernel::cow_ptr<HistogramX> ptrX() const { return m_x; }

  // Temporary legacy interface to Y
  void setY(const Kernel::cow_ptr<HistogramY> &Y) & { m_y = Y; }
  MantidVec &dataY() & { return m_y.access().mutableRawData(); }
  const MantidVec &dataY() const & { return m_y->rawData(); }
  const MantidVec &readY() const { return m_y->rawData(); }
  Kernel::cow_ptr<HistogramY> ptrY() const { return m_y; }

  // Temporary legacy interface to E
  void setE(const Kernel::cow_ptr<HistogramE> &E) & { m_e = E; }
  MantidVec &dataE() & { return m_e.access().mutableRawData(); }
  const MantidVec &dataE() const & { return m_e->rawData(); }
  const MantidVec &readE() const { return m_e->rawData(); }
  Kernel::cow_ptr<HistogramE> ptrE() const { return m_e; }

  // Temporary legacy interface to Dx. Note that accessors mimic the current
  // behavior which always has Dx allocated.
  MantidVec &dataDx() & {
    if (!m_dx)
      m_dx = Kernel::make_cow<HistogramDx>(size(), 0.0);
    return m_dx.access().mutableRawData();
  }
  const MantidVec &dataDx() const & {
    if (!m_dx)
      m_dx = Kernel::make_cow<HistogramDx>(size(), 0.0);
    return m_dx->rawData();
  }
  const MantidVec &readDx() const {
    if (!m_dx)
      m_dx = Kernel::make_cow<HistogramDx>(size(), 0.0);
    return m_dx->rawData();
  }

  // TODO This is a temporary helper function for refactoring, must be removed!
  void setYMode(YMode ymode) { m_yMode = ymode; }
  void convertToCounts();
  void convertToFrequencies();

  HistogramIterator begin() const &;
  HistogramIterator end() const &;
  // Calling begin/end on a temporary is not allowed as the iterator
  // reference will be immediately invalidated.
  HistogramIterator begin() const && = delete;
  HistogramIterator end() const && = delete;

private:
  template <class TX> void initX(const TX &x);
  template <class TY> void initY(const TY &y);
  template <class TE> void initE(const TE &e);
  template <class TY> void setValues(const TY &y);
  template <class TE> void setUncertainties(const TE &e);
  void checkAndSetYModeCounts();
  void checkAndSetYModeFrequencies();
  template <class T> void checkSize(const T &data) const;
  template <class... T> bool selfAssignmentX(const T &...) { return false; }
  template <class... T> bool selfAssignmentDx(const T &...) { return false; }
  template <class... T> bool selfAssignmentY(const T &...) { return false; }
  template <class... T> bool selfAssignmentE(const T &...) { return false; }

  XMode m_xMode;
  YMode m_yMode{YMode::Uninitialized};
};

template <> MANTID_HISTOGRAMDATA_DLL void Histogram::initX(const Points &x);
template <> MANTID_HISTOGRAMDATA_DLL void Histogram::initX(const BinEdges &x);
template <> MANTID_HISTOGRAMDATA_DLL void Histogram::setValues(const Counts &y);
template <>
MANTID_HISTOGRAMDATA_DLL void Histogram::setValues(const Frequencies &y);
template <>
MANTID_HISTOGRAMDATA_DLL void
Histogram::setUncertainties(const CountVariances &e);
template <>
MANTID_HISTOGRAMDATA_DLL void
Histogram::setUncertainties(const CountStandardDeviations &e);
template <>
MANTID_HISTOGRAMDATA_DLL void
Histogram::setUncertainties(const FrequencyVariances &e);
template <>
void MANTID_HISTOGRAMDATA_DLL
Histogram::setUncertainties(const FrequencyStandardDeviations &e);

/** Construct from X data, (optionally) Y data, and (optionally) E data.

  @param x X data for the Histogram. Can be BinEdges or Points.
  @param y Optional Y data for the Histogram. Can be Counts or Frequencies.
  @param e Optional E data for the Histogram. Can be Variances or
  StandardDeviations for Counts or Frequencies. If not specified or null, the
  standard deviations will be set as the square root of the Y data.
  */
template <class TX, class TY, class TE>
Histogram::Histogram(const TX &x, const TY &y, const TE &e) {
  initX(x);
  initY(y);
  initE(e);
}

template <class TY> void Histogram::initY(const TY &y) {
  if (y)
    setValues(y);
}

template <class TE> void Histogram::initE(const TE &e) {
  if (e) {
    if (!m_y)
      throw std::logic_error("Histogram: attempted to set uncertainties for "
                             "histogram without data");
    setUncertainties(e);
  } else if (m_y) {
    if (yMode() == YMode::Counts)
      setCountVariances(m_y->rawData());
    if (yMode() == YMode::Frequencies)
      setFrequencyVariances(m_y->rawData());
  }
}

/** Sets the Histogram's bin edges.

 Any arguments that can be used for constructing a BinEdges object are allowed,
 however, a size check ensures that the Histogram stays valid, i.e., that x and
 y lengths are consistent. */
template <typename... T> void Histogram::setBinEdges(T &&... data) & {
  BinEdges edges(std::forward<T>(data)...);
  checkSize(edges);
  if (selfAssignmentX(data...))
    return;
  m_xMode = XMode::BinEdges;
  m_x = edges.cowData();
}

/** Sets the Histogram's points.

 Any arguments that can be used for constructing a Points object are allowed,
 however, a size check ensures that the Histogram stays valid, i.e., that x and
 y lengths are consistent. */
template <typename... T> void Histogram::setPoints(T &&... data) & {
  Points points(std::forward<T>(data)...);
  checkSize(points);
  if (selfAssignmentX(data...))
    return;
  m_xMode = XMode::Points;
  m_x = points.cowData();
}

/// Sets the Histogram's point variances.
template <typename... T> void Histogram::setPointVariances(T &&... data) & {
  PointVariances points(std::forward<T>(data)...);
  if (points)
    checkSize(points);
  // No sensible self assignment is possible, we do not store variances, so if
  // anyone tries to set our current data as variances it must be an error.
  if (selfAssignmentDx(data...))
    throw std::logic_error("Histogram::setPointVariances: Attempt to "
                           "self-assign standard deviations as variance.");
  // Convert variances to standard deviations before storing it.
  m_dx = PointStandardDeviations(std::move(points)).cowData();
}

/// Sets the Histogram's point standard deviations.
template <typename... T>
void Histogram::setPointStandardDeviations(T &&... data) & {
  PointStandardDeviations points(std::forward<T>(data)...);
  if (points)
    checkSize(points);
  if (selfAssignmentDx(data...))
    return;
  m_dx = points.cowData();
}

/** Sets the Histogram's counts.

 Any arguments that can be used for constructing a Counts object are allowed,
 however, a size check ensures that the Histogram stays valid, i.e., that x and
 y lengths are consistent. */
template <typename... T> void Histogram::setCounts(T &&... data) & {
  checkAndSetYModeCounts();
  Counts counts(std::forward<T>(data)...);
  checkSize(counts);
  if (selfAssignmentY(data...))
    return;
  m_y = counts.cowData();
}

/// Sets the Histogram's count variances.
template <typename... T> void Histogram::setCountVariances(T &&... data) & {
  checkAndSetYModeCounts();
  CountVariances counts(std::forward<T>(data)...);
  checkSize(counts);
  // No sensible self assignment is possible, we do not store variances, so if
  // anyone tries to set our current data as variances it must be an error.
  if (selfAssignmentE(data...))
    throw std::logic_error("Histogram::setCountVariances: Attempt to "
                           "self-assign standard deviations as variance.");
  // Convert variances to standard deviations before storing it.
  m_e = CountStandardDeviations(std::move(counts)).cowData();
}

/// Sets the Histogram's count standard deviations.
template <typename... T>
void Histogram::setCountStandardDeviations(T &&... data) & {
  checkAndSetYModeCounts();
  CountStandardDeviations counts(std::forward<T>(data)...);
  checkSize(counts);
  if (selfAssignmentE(data...))
    return;
  m_e = counts.cowData();
}

/** Sets the Histogram's frequencies.

 Any arguments that can be used for constructing a Frequencies object are
 allowed, however, a size check ensures that the Histogram stays valid, i.e.,
 that x and y lengths are consistent. */
template <typename... T> void Histogram::setFrequencies(T &&... data) & {
  checkAndSetYModeFrequencies();
  Frequencies frequencies(std::forward<T>(data)...);
  checkSize(frequencies);
  if (selfAssignmentY(data...))
    return;
  m_y = frequencies.cowData();
}

/// Sets the Histogram's frequency variances.
template <typename... T> void Histogram::setFrequencyVariances(T &&... data) & {
  checkAndSetYModeFrequencies();
  FrequencyVariances frequencies(std::forward<T>(data)...);
  checkSize(frequencies);
  if (selfAssignmentE(data...))
    return;
  m_e = FrequencyStandardDeviations(std::move(frequencies)).cowData();
}

/// Sets the Histogram's frequency standard deviations.
template <typename... T>
void Histogram::setFrequencyStandardDeviations(T &&... data) & {
  checkAndSetYModeFrequencies();
  FrequencyStandardDeviations frequencies(std::forward<T>(data)...);
  checkSize(frequencies);
  if (selfAssignmentE(data...))
    return;
  m_e = frequencies.cowData();
}

template <>
MANTID_HISTOGRAMDATA_DLL void Histogram::checkSize(const BinEdges &data) const;

template <class T> void Histogram::checkSize(const T &data) const {
  size_t target = m_x->size();
  // 0 edges -> 0 points, otherwise points are 1 less than edges.
  if (xMode() == XMode::BinEdges && target > 0)
    target--;
  if (target != data.size())
    throw std::logic_error("Histogram: size mismatch of data.\n");
}

template <> inline bool Histogram::selfAssignmentX(const HistogramX &data) {
  return &data == m_x.get();
}

template <>
inline bool Histogram::selfAssignmentX(const std::vector<double> &data) {
  return static_cast<bool>(m_x) && &data == &(m_x->rawData());
}

template <> inline bool Histogram::selfAssignmentDx(const HistogramDx &data) {
  return &data == m_dx.get();
}

template <>
inline bool Histogram::selfAssignmentDx(const std::vector<double> &data) {
  return static_cast<bool>(m_dx) && &data == &(m_dx->rawData());
}

template <> inline bool Histogram::selfAssignmentY(const HistogramY &data) {
  return &data == m_y.get();
}

template <>
inline bool Histogram::selfAssignmentY(const std::vector<double> &data) {
  return static_cast<bool>(m_y) && &data == &(m_y->rawData());
}

template <> inline bool Histogram::selfAssignmentE(const HistogramE &data) {
  return &data == m_e.get();
}

template <>
inline bool Histogram::selfAssignmentE(const std::vector<double> &data) {
  return static_cast<bool>(m_e) && &data == &(m_e->rawData());
}

MANTID_HISTOGRAMDATA_DLL Histogram::XMode getHistogramXMode(size_t xLength,
                                                            size_t yLength);

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAM_H_ */
