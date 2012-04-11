#ifndef MANTID_ALGORITHMS_GENERATEEVENTSFILTER_H_
#define MANTID_ALGORITHMS_GENERATEEVENTSFILTER_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidDataObjects/SplittersWorkspace.h"

namespace Mantid
{
namespace Algorithms
{

  /** GenerateEventsFilter : Generate an events-filter, i.e., a SplittersWorkspace according
      to user's request.

      Request can be a combination of log value and time, i.e.,

      (1) T_min
      (2) T_max
      (3) delta Time
      (4) Min log value
      (5) Max log value
      (6) delta log value
      (7) identify log value increment (bool)
      (8) number of sections per interval (applied to log value only!)

      This algorithm can generate filters including
      (1) deltaT per interval from T_min to T_max with : Log value is not given
      (2) delta log value per interval from T_min to T_max and from min log value to max log value

      Note:
      (1) Time can be (a) relative time in ns  (b) relative time in second (float) (c) percentage time
      (2) if option "identify log value increment"
    
    @date 2012-04-09

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport GenerateEventsFilter : public API::Algorithm
  {
  public:
    GenerateEventsFilter();
    virtual ~GenerateEventsFilter();
    
    /// Algorithm's name for identification overriding a virtual method
    virtual const std::string name() const { return "GenerateEventsFilter";};
    /// Algorithm's version for identification overriding a virtual method
    virtual int version() const { return 1;};
    /// Algorithm's category for identification overriding a virtual method
    virtual const std::string category() const { return "Events\\EventFiltering";}

  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    // Implement abstract Algorithm methods
    void init();
    // Implement abstract Algorithm methods
    void exec();

    void processInputTime(Kernel::DateAndTime runstarttime);
    void setFilterByTimeOnly();
    void setFilterByValue();

    DataObjects::EventWorkspace_const_sptr mEventWS;
    DataObjects::SplittersWorkspace_sptr mSplitters;

    Kernel::DateAndTime mStartTime;
    Kernel::DateAndTime mStopTime;

  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_GENERATEEVENTSFILTER_H_ */
