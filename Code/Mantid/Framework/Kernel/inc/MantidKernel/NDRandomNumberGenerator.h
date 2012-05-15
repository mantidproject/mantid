#ifndef MANTID_KERNEL_NDRANDOMNUMBERGENERATOR_H_
#define MANTID_KERNEL_NDRANDOMNUMBERGENERATOR_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "DllConfig.h"
#include <vector>

namespace Mantid
{
  namespace Kernel
  {
    /** 
      This class defines an interface for N dimensional random number generators.
      A call to next produces N points in an ND space

      @author Martyn Gigg, Tessella plc
      @date 19/05/2012

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
    class MANTID_KERNEL_DLL NDRandomNumberGenerator
    {
    public:
      /// Virtual destructor to ensure that all inheriting classes have one
      virtual ~NDRandomNumberGenerator() {};
      /// Generate the next set of values that form a point in ND space
      virtual std::vector<double> nextPoint() = 0;
      /// Reset the generator
      virtual void reset() = 0;
    };
  }
}

#endif
