// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/MDLeanEvent.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"

namespace Mantid {
namespace DataObjects {
/** MDBin : Class describing a single bin in a dense, Multidimensional
 *histogram.
 * This object will get passed around by MDBox'es and accumulate the total
 * signal of events contained in it.
 * The signal (and error) of each bin will be used to make the big dense
 *histogram.
 *
 * NOTE: For now, will only support bins that are aligned with the workspace
 *axes (no rotation!),
 * but in future it will be extended.
 *
 * @tparam nd :: the number of dimensions **in the workspace being binned, not
 *the output workspace **
 *
 * @author Janik Zikovsky, SNS
 * @date 2011-03-23 17:04:02.621325
 */
TMDE_CLASS
class MANTID_DATAOBJECTS_DLL MDBin {
public:
  MDBin();

  /** The accumulated signal in this bin.
   * This is public so as to avoid the need (and slowdown) of getters/setters
   */
  signal_t m_signal;

  /** The accumulated error (squared) in this bin.
   * This is public so as to avoid the need (and slowdown) of getters/setters
   */
  signal_t m_errorSquared;

  /// The minimum edge of the bin for each dimension in the workspace
  coord_t m_min[nd];

  /// The maximum edge of the bin for each dimension in the workspace
  coord_t m_max[nd];

  /// Index of where this bin lands into the broader histogrammed workspace.
  size_t m_index;
};

} // namespace DataObjects
} // namespace Mantid
