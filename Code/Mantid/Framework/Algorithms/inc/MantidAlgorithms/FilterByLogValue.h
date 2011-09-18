#ifndef MANTID_ALGORITHMS_FILTERBYLOGVALUE_H_
#define MANTID_ALGORITHMS_FILTERBYLOGVALUE_H_
/*WIKI* 


Filters out events using the entries in the Sample Logs. 

Sample logs consist of a series of <Time, Value> pairs. The first step in filtering is to generate a list of start-stop time intervals that will be kept, using those logs.
* Each log value is compared to the min/max value filters to determine whether it is "good" or not.
** For a single log value that satisfies the criteria at time T, all events between T+-Tolerance (LogBoundary=Centre), or T and T+Tolerance (LogBoundary=Left) are kept.
** If there are several consecutive log values matching the filter, events between T1-Tolerance and T2+Tolerance (LogBoundary=Centre), or T1 and T2+Tolerance (LogBoundary=Left) are kept.
* The filter is then applied to all events in all spectra. Any events with pulse times outside of any "good" time ranges are removed.

There is no interpolation of log values between the discrete sample log times at this time.
However, the log value is assumed to be constant at times before its first point and after its last. For example, if the first temperature measurement was at time=10 seconds and a temperature within the acceptable range, then all events between 0 and 10 seconds will be included also. If a log has a single point in time, then that log value is assumed to be constant for all time and if it falls within the range, then all events will be kept.


*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid
{
using DataObjects::EventList;
using DataObjects::EventWorkspace;
using DataObjects::EventWorkspace_sptr;
using DataObjects::EventWorkspace_const_sptr;

namespace Algorithms
{


/** Filters events in an EventWorkspace using values in a SampleLog.

    @author Janik Zikovsky, SNS
    @date September 15th, 2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport FilterByLogValue : public API::Algorithm
{
public:
  FilterByLogValue();
  virtual ~FilterByLogValue();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "FilterByLogValue";};
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1;};
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "General";}

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Implement abstract Algorithm methods
  void init();
  void exec();

  /// Pointer for an event workspace
  EventWorkspace_const_sptr eventW;
};



} // namespace Algorithms
} // namespace Mantid


#endif /* MANTID_ALGORITHMS_FILTERBYLOGVALUE_H_ */


