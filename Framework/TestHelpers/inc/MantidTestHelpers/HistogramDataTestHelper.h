/*********************************************************************************
*  PLEASE READ THIS!!!!!!!
*
*  This header MAY ONLY be included from a test using Histogram types.
*********************************************************************************/

#ifndef HISTOGRAMDATATESTHELPER_H
#define HISTOGRAMDATATESTHELPER_H

#include <MantidHistogramData/FixedLengthVector.h>

namespace HistogramDataTestHelper {
	template <class T>
	bool operator==(
		const Mantid::HistogramData::detail::FixedLengthVector<T> &lhs,
		const Mantid::HistogramData::detail::FixedLengthVector<T> &rhs) {
		return lhs.rawData() == rhs.rawData();
	}
} // namespace HistogramDataTestHelper
#endif // HISTGRAMDATATESTHELPER_H