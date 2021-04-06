// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidParallel/IO/EventParser.h"

namespace Mantid {
namespace Parallel {
namespace IO {
namespace detail {

/** Transform event IDs to global spectrum numbers using the bankOffsets stored
 * at object creation.
 *
 * The transformation is in-place to save memory bandwidth and modifies the
 * range pointed to by `event_id_start`.
 * @param event_id_start Starting position of chunk of data containing event
 * IDs.
 * @param count Number of items in data chunk
 * @param bankOffset Offset to subtract from the array `event_id_start`.
 */
void eventIdToGlobalSpectrumIndex(int32_t *event_id_start, size_t count, const int32_t bankOffset) {
  for (size_t i = 0; i < count; ++i)
    event_id_start[i] -= bankOffset;
}

} // namespace detail
} // namespace IO
} // namespace Parallel
} // namespace Mantid
