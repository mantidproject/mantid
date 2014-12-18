#ifndef MANTID_ALGORITHMS_FILTERBYTIME_H_
#define MANTID_ALGORITHMS_FILTERBYTIME_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid
{

namespace Algorithms
{


/** Filters an EventWorkspace by wall-clock time, and outputs to a new event workspace
    or replaces the existing one.

    @author Janik Zikovsky, SNS
    @date September 14th, 2010

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class DLLExport FilterByTime : public API::Algorithm
{
public:
  FilterByTime();
  virtual ~FilterByTime();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "FilterByTime";};
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "This algorithm filters out events from an EventWorkspace that are not between given start and stop times.";}

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1;};
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Events\\EventFiltering";}

private:
  
  // Implement abstract Algorithm methods
  void init();
  void exec();

  /// Pointer for an event workspace
  DataObjects::EventWorkspace_const_sptr eventW;
};



} // namespace Algorithms
} // namespace Mantid


#endif /* FILTERBYTIME_H_ */


