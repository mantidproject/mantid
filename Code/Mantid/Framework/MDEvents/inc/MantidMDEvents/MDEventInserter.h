#ifndef MANTID_MDEVENTS_MDEVENTINSERTER_H_
#define MANTID_MDEVENTS_MDEVENTINSERTER_H_

#include "MantidKernel/System.h"

namespace Mantid
{
namespace MDEvents
{

  /** MDEventInserter : Helper class that provides a generic interface for adding events to an MDWorkspace without knowing whether the workspace is storing MDLeanEvents or full MDEvents

    Uses LOKI techniques for choosing the overload addition operation based on embedded type arguments in the respective MDLeanEventTypes. This solution is
    nice because depending upon the workspace type, only one of the private addEvent funtions is instantiated. For usage, you only need to know the dimensionality of the workspace,
    not the underlying type of MDEvent being used.
    
    @date 2012-07-16

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  template <typename MDEW_SPTR>
  class DLLExport MDEventInserter
  {
  private:

    /// Loki IntToType, used for template overload deduction.
    template<int I>
    struct IntToType
    {
      enum{value = I};
    };

  public:

    /// Type of MDEvent used by the MDEventWorkspace.
    typedef typename MDEW_SPTR::element_type::MDEventType MDEventType;

    /**
    Constructor 
    @param ws : MDEventWorkspace to add to.
    */
    MDEventInserter(MDEW_SPTR ws) : m_ws(ws)
    {
    }

    /**
    Creates an mdevent and adds it to the MDEW. The type of MDEvent generated is determined internally using type information on the MDEventType.
    @param signal : intensity
    @param errorSQ : squared value of the error
    @param runno : run number
    @param detectno : detector number
    @param coords : pointer to coordinates array
    */
    void insertMDEvent(float signal, float errorSQ, uint16_t runno, int32_t detectno, Mantid::coord_t* coords)
    {
      // compile-time overload selection based on nested type information on the MDEventType.
      insertMDEvent(signal, errorSQ, runno, detectno, coords, IntToType<MDEventType::is_full_mdevent>());
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
    void insertMDEvent(float signal, float errorSQ, uint16_t, int32_t, Mantid::coord_t* coords, IntToType<false>)
    {
      m_ws->addEvent(MDEventType(signal, errorSQ, coords));
    }

    
    /**
    Creates a FULL MDEvent and adds it to the MDEW. 
    @param signal : intensity
    @param errorSQ : squared value of the error
    @param runno : run number 
    @param detectno : detector number 
    @param coords : pointer to coordinates array
    */
    void insertMDEvent(float signal, float errorSQ, uint16_t runno, int32_t detectno, Mantid::coord_t* coords, IntToType<true>)
    {
      m_ws->addEvent(MDEventType(signal, errorSQ, runno, detectno, coords));
    }
  };

} // namespace MDEvents
} // namespace Mantid

#endif  /* MANTID_MDEVENTS_MDEVENTINSERTER_H_ */
