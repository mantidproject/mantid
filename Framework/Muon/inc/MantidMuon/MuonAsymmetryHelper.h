// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHM_MUONASYMMETRYHELPER_H_
#define MANTID_ALGORITHM_MUONASYMMETRYHELPER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ITableWorkspace.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidMuon/DllConfig.h"

namespace Mantid {
/*
A set of helper functions for calculating asymmetry. Including:
 Calculating the normalised counts, estimating the normalisation constant and
finding the range
 of data to use in the analysis.


@author
@date 03/03/2017
*/
HistogramData::Histogram
normaliseCounts(const HistogramData::Histogram &histogram,
                const double numGoodFrames);
// calculate Muon normalisation constant
double estimateNormalisationConst(const HistogramData::Histogram &histogram,
                                  const double numGoodFrames,
                                  const double startX, const double endX);
size_t startIndexFromTime(const HistogramData::BinEdges &xData,
                          const double startX);
size_t endIndexFromTime(const HistogramData::BinEdges &xData,
                        const double endX);

void updateNormalizationTable(Mantid::API::ITableWorkspace_sptr &table,
                              const std::vector<std::string> &wsNamse,
                              const std::vector<double> &norms,
                              const std::vector<std::string> &methods);

} // namespace Mantid

#endif /*MANTID_MUONASYMMETRYHELPER_H_*/
