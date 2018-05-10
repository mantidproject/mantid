#include "MantidHistogramData/Rebin.h"
#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/Exception.h"
#include "MantidHistogramData/Histogram.h"
#include <algorithm>
#include <numeric>

using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::CountStandardDeviations;
using Mantid::HistogramData::CountVariances;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::Exception::InvalidBinEdgesError;
using Mantid::HistogramData::Frequencies;
using Mantid::HistogramData::FrequencyStandardDeviations;
using Mantid::HistogramData::Histogram;

namespace {
Histogram rebinCounts(const Histogram &input, const BinEdges &binEdges) {
  auto &xold = input.x();
  auto &yold = input.y();
  auto &eold = input.e();

  auto &xnew = binEdges.rawData();
  Counts newCounts(xnew.size() - 1);
  CountVariances newCountVariances(xnew.size() - 1);
  auto &ynew = newCounts.mutableData();
  auto &enew = newCountVariances.mutableData();

  auto size_yold = yold.size();
  auto size_ynew = ynew.size();
  size_t iold = 0;
  size_t inew = 0;

  while ((inew < size_ynew) && (iold < size_yold)) {
    auto xo_low = xold[iold];
    auto xo_high = xold[iold + 1];
    auto xn_low = xnew[inew];
    auto xn_high = xnew[inew + 1];
    auto owidth = xo_high - xo_low;
    auto nwidth = xn_high - xn_low;

    if (owidth <= 0.0 || nwidth <= 0.0) {
      if (xo_high == -DBL_MAX && xo_low == -DBL_MAX) {
        throw InvalidBinEdgesError(
            "One or more x-values was unusually low "
            "(below -1e100). This usually occurs when a "
            "monitor spectrum has not been masked after "
            "ConvertUnits has been run on the workspace");
      } else {
        throw InvalidBinEdgesError("Negative or zero bin widths not allowed.");
      }
    }

    if (xn_high <= xo_low)
      inew++; /* old and new bins do not overlap */
    else if (xo_high <= xn_low)
      iold++; /* old and new bins do not overlap */
    else {
      // delta is the overlap of the bins on the x axis
      auto delta = xo_high < xn_high ? xo_high : xn_high;
      delta -= xo_low > xn_low ? xo_low : xn_low;

      ynew[inew] += yold[iold] * delta / owidth;
      enew[inew] += eold[iold] * eold[iold] * delta / owidth;

      if (xn_high > xo_high) {
        iold++;
      } else {
        inew++;
      }
    }
  }

  return Histogram(binEdges, newCounts,
                   CountStandardDeviations(std::move(newCountVariances)));
}

Histogram rebinFrequencies(const Histogram &input, const BinEdges &binEdges) {
  auto &xold = input.x();
  auto &yold = input.y();
  auto &eold = input.e();

  auto &xnew = binEdges.rawData();
  Frequencies newFrequencies(xnew.size() - 1);
  FrequencyStandardDeviations newFrequencyStdDev(xnew.size() - 1);
  auto &ynew = newFrequencies.mutableData();
  auto &enew = newFrequencyStdDev.mutableData();

  auto size_yold = yold.size();
  auto size_ynew = ynew.size();
  size_t iold = 0;
  size_t inew = 0;

  while ((inew < size_ynew) && (iold < size_yold)) {
    auto xo_low = xold[iold];
    auto xo_high = xold[iold + 1];
    auto xn_low = xnew[inew];
    auto xn_high = xnew[inew + 1];

    auto owidth = xo_high - xo_low;
    auto nwidth = xn_high - xn_low;

    if (owidth <= 0.0 || nwidth <= 0.0)
      throw InvalidBinEdgesError("Negative or zero bin widths not allowed.");

    if (xn_high <= xo_low)
      inew++; /* old and new bins do not overlap */
    else if (xo_high <= xn_low)
      iold++; /* old and new bins do not overlap */
    else {
      //        delta is the overlap of the bins on the x axis
      auto delta = xo_high < xn_high ? xo_high : xn_high;
      delta -= xo_low > xn_low ? xo_low : xn_low;

      ynew[inew] += yold[iold] * delta;
      enew[inew] += eold[iold] * eold[iold] * delta * owidth;

      if (xn_high > xo_high) {
        iold++;
      } else {
        inew++;
      }
    }
  }

  for (size_t i = 0; i < size_ynew; ++i) {
    auto width = xnew[i + 1] - xnew[i];
    auto factor = 1 / width;
    ynew[i] *= factor;
    enew[i] = sqrt(enew[i]) * factor;
  }

  return Histogram(binEdges, newFrequencies, newFrequencyStdDev);
}
} // anonymous namespace

namespace Mantid {
namespace HistogramData {

/** Rebins data according to a new set of bin edges.
 * @param input :: input histogram data to be rebinned.
 * @param binEdges :: input will be rebinned according to this set of bin edges.
 * @returns The rebinned histogram.
 * @throws std::runtime_error if the input histogram xmode is not BinEdges,
 * the input yMode is undefined, or for non-positive input/output bin widths
 */
Histogram rebin(const Histogram &input, const BinEdges &binEdges) {
  if (input.xMode() != Histogram::XMode::BinEdges)
    throw std::runtime_error(
        "XMode must be Histogram::XMode::BinEdges for input histogram");
  if (input.yMode() == Histogram::YMode::Counts)
    return rebinCounts(input, binEdges);
  else if (input.yMode() == Histogram::YMode::Frequencies)
    return rebinFrequencies(input, binEdges);
  else
    throw std::runtime_error("YMode must be defined for input histogram.");
}

} // namespace HistogramData
} // namespace Mantid
