#ifndef MANTID_ALGORITHMS_CONVERTTOEVENTWORKSPACE_H_
#define MANTID_ALGORITHMS_CONVERTTOEVENTWORKSPACE_H_
/*WIKI* 

This algorithm takes a Workspace2D with any binning or units as its input. An event is created for each bin of each histogram, except if the bin count is 0.0. The event is created with an X position (typically time-of-flight) equal to the **center** of the bin. The weight and error of the event are taken from the histogram value.

If the GenerateMultipleEvents option is set, then instead of a single event per bin, a certain number of events evenly distributed along the X bin are generated. The number of events generated in each bin is calculated by N = (Y/E)^2 .
*WIKI*/
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 

namespace Mantid
{
namespace Algorithms
{

  /** Perform a conversion for a Workspace2D to an equivalent
   * EventWorkspace
    
    @author Janik Zikovsky
    @date 2011-08-23

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport ConvertToEventWorkspace  : public API::Algorithm
  {
  public:
    ConvertToEventWorkspace();
    ~ConvertToEventWorkspace();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "ConvertToEventWorkspace";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "Algorithms";}
    
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();


  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_CONVERTTOEVENTWORKSPACE_H_ */
