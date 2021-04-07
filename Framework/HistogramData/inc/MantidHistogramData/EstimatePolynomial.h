// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/Histogram.h"
#include <vector>

namespace Mantid {
namespace HistogramData {

/** EstimatePolynomial : TODO: DESCRIPTION
 */

/**
 * @brief estimateBackground Estimate a polynomial using Gauss-Markov.
 *
 * Uses Gauss-Markov to find the best linear unbiased estimator (BLUE). This
 * will select the functional
 * form that has the smallest reduced chisq (divided by degrees of freedom) that
 * is less than or equal
 * to the polynomial order requested.
 * https://en.wikipedia.org/wiki/Gauss%E2%80%93Markov_theorem
 *
 * @param order Maximum order of the polynomial to fit
 * @param i_min Left boundary of window (inclusive)
 * @param i_max Right boundary of window (exclusive)
 * @param p_min Left boundary of data in window to skip (inclusive)
 * @param p_max Right boundary of data in window to skip (exclusive)
 * @param out_bg0 constant term
 * @param out_bg1 linear term
 * @param out_bg2 quadratic term
 * @param out_chisq_red reduced chisq (chisq normalized by degrees of freedom)
 */
MANTID_HISTOGRAMDATA_DLL void estimateBackground(const size_t order, const Mantid::HistogramData::Histogram &histo,
                                                 const size_t i_min, const size_t i_max, const size_t p_min,
                                                 const size_t p_max, double &out_bg0, double &out_bg1, double &out_bg2,
                                                 double &out_chisq_red);

MANTID_HISTOGRAMDATA_DLL void estimatePolynomial(const size_t order, const Mantid::HistogramData::Histogram &histo,
                                                 const size_t i_min, const size_t i_max, double &out_bg0,
                                                 double &out_bg1, double &out_bg2, double &out_chisq_red);

} // namespace HistogramData
} // namespace Mantid
