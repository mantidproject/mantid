// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataObjects {

/** MDEventInserter : Helper class that provides a generic interface for adding
  events to an MDWorkspace without knowing whether the workspace is storing
  MDLeanEvents or full MDEvents

  Uses LOKI techniques for choosing the overload addition operation based on
  embedded type arguments in the respective MDLeanEventTypes. This solution is
  nice because depending upon the workspace type, only one of the private
  addEvent funtions is instantiated. For usage, you only need to know the
  dimensionality of the workspace,
  not the underlying type of MDEvent being used.

  @date 2012-07-16
*/
template <typename MDEW_SPTR> class DLLExport MDEventInserter {
private:
  /// Loki IntToType, used for template overload deduction.
  template <int I> struct IntToType {
    enum { value = I };
  };

public:
  /// Type of MDEvent used by the MDEventWorkspace.
  using MDEventType = typename MDEW_SPTR::element_type::MDEventType;

  /**
  Constructor
  @param ws : MDEventWorkspace to add to.
  */
  MDEventInserter(MDEW_SPTR ws) : m_ws(ws) {}

  /**
  Creates an mdevent and adds it to the MDEW. The type of MDEvent generated is
  determined internally using type information on the MDEventType.
  @param signal : intensity
  @param errorSQ : squared value of the error
  @param runindex : run index (index into the vector of ExperimentInfo)
  @param goniometerIndex: 0-based index determines the goniometer settings when
  this event occurred
  @param detectno : detector number
  @param coords : pointer to coordinates array
  */
  void insertMDEvent(float signal, float errorSQ, uint16_t runindex, uint16_t goniometerIndex, int32_t detectno,
                     Mantid::coord_t *coords) {
    // compile-time overload selection based on nested type information on the
    // MDEventType.
    insertMDEvent(signal, errorSQ, runindex, goniometerIndex, detectno, coords,
                  IntToType<MDEventType::is_full_mdevent>());
  }

private:
  /// shared pointer to MDEW to add to.
  MDEW_SPTR m_ws;

  /**
  Creates a LEAN MDEvent and adds it to the MDEW.
  @param signal : intensity
  @param errorSQ : squared value of the error
  @param coords : pointer to coordinates array
 */
  void insertMDEvent(float signal, float errorSQ, uint16_t, uint16_t, int32_t, Mantid::coord_t *coords,
                     IntToType<false>) {
    m_ws->addEvent(MDEventType(signal, errorSQ, coords));
  }

  /**
  Creates a FULL MDEvent and adds it to the MDEW.
  @param signal : intensity
  @param errorSQ : squared value of the error
  @param runindex : run index
  @param goniometerIndex: 0-based index determines the goniometer settings when
  this event occurred
  @param detectno : detector number
  @param coords : pointer to coordinates array
  */
  void insertMDEvent(float signal, float errorSQ, uint16_t runindex, uint16_t goniometerIndex, int32_t detectno,
                     Mantid::coord_t *coords, IntToType<true>) {
    m_ws->addEvent(MDEventType(signal, errorSQ, runindex, goniometerIndex, detectno, coords));
  }
};

} // namespace DataObjects
} // namespace Mantid
