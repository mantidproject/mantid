#ifndef MANTID_ALGORITHM_MUONASYMMETRYHELPER_H_
#define MANTID_ALGORITHM_MUONASYMMETRYHELPER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
/*
A set of helper functions for calculating asymmetry. Including:
 Calculating the normalised counts, estimating the normalisation constant and
finding the range
 of data to use in the analysis.


@author
@date 03/03/2017

Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
} // namespace Mantid

#endif /*MANTID_MUONASYMMETRYHELPER_H_*/
