// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/Interpolation.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/UnitFactory.h"

#include <algorithm>
#include <iterator>
#include <stdexcept>

namespace Mantid {
namespace Kernel {
namespace {
/// static logger
Logger g_log("Interpolation");
} // namespace

/* Functor used in std::lower_bound to replicate the original behavior.
 */
struct LessOrEqualFunctor {
  bool operator()(const DataXY &lhs, const DataXY &rhs) { return lhs.first <= rhs.first; }
};

/** Constructor default to linear interpolation and x-unit set to TOF
 */
Interpolation::Interpolation() : m_method("linear") {
  m_xUnit = UnitFactory::Instance().create("TOF");
  m_yUnit = UnitFactory::Instance().create("TOF");
}

/** Get iterator of item that is next larger than the supplied x value
 * @param key :: the x value to base the search on
 * @return iterator of the next largest x value
 */
std::vector<DataXY>::const_iterator Interpolation::findIndexOfNextLargerValue(double key) const {
  return std::lower_bound(m_data.begin(), m_data.end(), DataXY(key, 0), LessOrEqualFunctor());
}

std::vector<DataXY>::const_iterator Interpolation::cbegin() const { return m_data.cbegin(); }

std::vector<DataXY>::const_iterator Interpolation::cend() const { return m_data.cend(); }

void Interpolation::setXUnit(const std::string &unit) { m_xUnit = UnitFactory::Instance().create(unit); }

void Interpolation::setYUnit(const std::string &unit) { m_yUnit = UnitFactory::Instance().create(unit); }

/** Get interpolated value at location at
 * @param at :: Location where to get interpolated value
 * @return the value
 */
double Interpolation::value(const double &at) const {
  size_t N = m_data.size();

  if (N == 0) {
    g_log.error() << "Need at least one value for interpolation. Return "
                     "interpolation value zero.";
    return 0.0;
  }

  if (N == 1) {
    return m_data[0].second;
  }

  // check first if at is within the limits of interpolation interval

  if (at < m_data[0].first) {
    return m_data[0].second -
           (m_data[0].first - at) * (m_data[1].second - m_data[0].second) / (m_data[1].first - m_data[0].first);
  }

  if (at > m_data[N - 1].first) {
    return m_data[N - 1].second + (at - m_data[N - 1].first) * (m_data[N - 1].second - m_data[N - 2].second) /
                                      (m_data[N - 1].first - m_data[N - 2].first);
  }

  try {
    // otherwise
    // General case. Find index of next largest value by std::lower_bound.
    auto pos = findIndexOfNextLargerValue(at);

    auto posBefore = std::prev(pos);
    if (posBefore->first == at) {
      return posBefore->second;
    } else {
      double interpolatedY = posBefore->second + (at - posBefore->first) * (pos->second - posBefore->second) /
                                                     (pos->first - posBefore->first);
      return interpolatedY;
    }
  } catch (const std::range_error &) {
    return 0.0;
  }
}

/** Add point in the interpolation.
 *
 * @param xx :: x-value
 * @param yy :: y-value
 */
void Interpolation::addPoint(const double &xx, const double &yy) {
  size_t N = m_data.size();
  std::vector<DataXY>::const_iterator it;

  if (N == 0) {
    DataXY newpair(xx, yy);
    m_data.emplace_back(newpair);
    return;
  }

  // check first if xx is within the limits of interpolation interval

  if (xx < m_data[0].first) {
    it = m_data.begin();
    it = m_data.insert(it, DataXY(xx, yy));
    return;
  }

  if (xx > m_data[N - 1].first) {
    m_data.emplace_back(xx, yy);
    return;
  }

  // otherwise

  it = findIndexOfNextLargerValue(xx);

  auto posBefore = std::prev(it);
  if (posBefore->first != xx) {
    m_data.insert(it, DataXY(xx, yy));
  }
}

/**
  Prints object to stream
  @param os :: the Stream to output to
*/
void Interpolation::printSelf(std::ostream &os) const {
  os << m_method << " ; " << m_xUnit->unitID() << " ; " << m_yUnit->unitID();

  for (unsigned int i = 0; i < m_data.size(); i++) {
    os << " ; " << m_data[i].first << " " << m_data[i].second;
  }
}

/**
  Resets interpolation data by clearing the internal storage for x- and y-values
*/
void Interpolation::resetData() { m_data.clear(); }

/**
  Prints the value of parameter
  @param os :: the Stream to output to
  @param f :: the FitParameter to output
  @return the output stream
  */
std::ostream &operator<<(std::ostream &os, const Interpolation &f) {
  f.printSelf(os);
  return os;
}

/**
  Reads in parameter value
  @param in :: Input Stream
  @param f :: FitParameter to write to
  @return Current state of stream
*/
std::istream &operator>>(std::istream &in, Interpolation &f) {

  using tokenizer = Mantid::Kernel::StringTokenizer;
  std::string str;
  getline(in, str);
  tokenizer values(str, ";", tokenizer::TOK_TRIM);

  f.setMethod(values[0]);
  f.setXUnit(values[1]);
  f.setYUnit(values[2]);
  f.resetData(); // Reset data, in case the interpolation table is not empty

  for (unsigned int i = 3; i < values.count(); i++) {
    std::stringstream strstream(values[i]);
    double x, y;
    strstream >> x >> y;
    f.addPoint(x, y);
  }

  return in;
}

} // namespace Kernel
} // namespace Mantid
