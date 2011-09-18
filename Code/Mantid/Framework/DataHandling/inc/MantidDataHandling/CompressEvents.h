#ifndef MANTID_DATAHANDLING_COMPRESSEVENTS_H_
#define MANTID_DATAHANDLING_COMPRESSEVENTS_H_
/*WIKI* 


This algorithm starts by sorting the event lists by TOF; therefore you may gain speed by calling [[Sort]] beforehand. Starting from the smallest TOF, all events within Tolerance are considered to be identical. Pulse times are ignored. A weighted event without time information is created; its TOF is the average value of the summed events; its weight is the sum of the weights of the input events; its error is the sum of the square of the errors of the input events.

Note that using CompressEvents may introduce errors if you use too large of a tolerance. Rebinning an event workspace still uses an all-or-nothing view: if the TOF of the event is in the bin, then the counts of the bin is increased by the event's weight. If your tolerance is large enough that the compound event spans more than one bin, then you will get small differences in the final histogram.

If you are working from the raw events with TOF resolution of 0.100 microseconds, then you can safely use a tolerance of, e.g., 0.05 microseconds to group events together. In this case, histograms with/without compression are identical. If your workspace has undergone changes to its X values (unit conversion for example), you have to use your best judgement for the Tolerance value.



*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace DataHandling
{
/** Compress an EventWorkspace by lumping together events with very close TOF value,
 * while ignoring the event's pulse time.
 *
 * This algorithm will go through all event lists and sum up together the weights and errors
 * of events with times-of-flight within a specified tolerance.
 * The event list data type is converted to WeightedEventNoTime, where the pulse time information
 * is not saved, in order to save memory.

    @author Janik Zikovsky, SNS
    @date Jan 19, 2011

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport CompressEvents : public API::Algorithm
{
public:
  CompressEvents();
  virtual ~CompressEvents();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "CompressEvents";};
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1;};
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "DataHandling";}

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Implement abstract Algorithm methods
  void init();
  void exec();
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_COMPRESSEVENTS_H_*/
