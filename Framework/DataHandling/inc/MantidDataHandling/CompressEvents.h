#ifndef MANTID_DATAHANDLING_COMPRESSEVENTS_H_
#define MANTID_DATAHANDLING_COMPRESSEVENTS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace DataHandling {
/** Compress an EventWorkspace by lumping together events with very close TOF
 value,
 * while ignoring the event's pulse time.
 *
 * This algorithm will go through all event lists and sum up together the
 weights and errors
 * of events with times-of-flight within a specified tolerance.
 * The event list data type is converted to WeightedEventNoTime, where the pulse
 time information
 * is not saved, in order to save memory.

    @author Janik Zikovsky, SNS
    @date Jan 19, 2011

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport CompressEvents : public API::Algorithm {
public:
  CompressEvents();
  virtual ~CompressEvents();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "CompressEvents"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Reduce the number of events in an EventWorkspace by grouping "
           "together events with identical or similar X-values "
           "(time-of-flight).";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; };
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Events"; }

private:
  // Implement abstract Algorithm methods
  void init();
  void exec();
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_COMPRESSEVENTS_H_*/
