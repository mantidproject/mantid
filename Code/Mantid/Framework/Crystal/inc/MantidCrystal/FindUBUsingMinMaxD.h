#ifndef MANTID_CRYSTAL_FIND_UB_USING_MIN_MAX_D_H_
#define MANTID_CRYSTAL_FIND_UB_USING_MIN_MAX_D_H_ 
/*WIKI* 

Given a set of peaks, and given a range of possible a,b,c values, this algorithm will attempt to find a UB matrix, that fits the data.  The algorithm searches over a range of possible directions and unit cell lengths for directions and lengths that match plane normals and plane spacings in reciprocal space.  It then chooses three of these vectors with the shortest lengths that are linearly independent and that are separated by at least a minimum angle.  The minimum angle is calculated from the specified min and max d values.  A UB matrix is formed using these three vectors and the resulting UB matrix is optimized using a least squares method. If the specified peaks are accurate and belong to a single crystal, this method should produce some UB matrix that indexes the peaks. However, other software will usually be needed to adjust this UB to match a desired conventional cell.
*WIKI*/
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 
#include "MantidKernel/Logger.h"

namespace Mantid
{
namespace Crystal
{
/** FindUBUsingMinMaxD : Algorithm to calculate a UB matrix,
    given lattice parameters and a list of peaks
    
    @author Dennis Mikkelson(adapted from Andrei Savici's CalculateUMatrix)
    @date   2011-08-17

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & 
                     NScD Oak Ridge National Laboratory

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

    File change history is stored at: 
    <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport FindUBUsingMinMaxD : public API::Algorithm
  {
  public:
    FindUBUsingMinMaxD();
    ~FindUBUsingMinMaxD();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const 
            { return "FindUBUsingMinMaxD";};

    /// Algorithm's version for identification 
    virtual int version() const 
            { return 1;};

    /// Algorithm's category for identification
    virtual const std::string category() const 
            { return "Crystal";}
    
  private:

    /// Sets documentation strings for this algorithm
    virtual void initDocs();

    /// Initialise the properties
    void init();

    /// Run the algorithm
    void exec();

    /// Static reference to the logger class
        static Kernel::Logger& g_log;
  };


} // namespace Crystal
} // namespace Mantid

#endif  /* MANTID_CRYSTAL_FIND_UB_USING_MIN_MAX_D_H_ */
