#ifndef MANTID_KERNEL_RANDOMNUMBERGENERATOR_H_
#define MANTID_KERNEL_RANDOMNUMBERGENERATOR_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "DllExport.h"

namespace Mantid
{
  namespace Kernel
  {
    /** 
      This class defines an interface for random number generators. 

      @author Martyn Gigg, Tessella plc
      @date 19/11/2007

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
    class EXPORT_OPT_MANTID_KERNEL RandomNumberGenerator
    {

    public:
      /// Virtual destructor to ensure that all inheriting classes have one
      virtual ~RandomNumberGenerator() {};
      /// Set the random number seed
      virtual void setSeed(long seedValue) = 0;
      /// Sets the range of the subsequent calls to next;
      virtual void setRange(double start, double end) = 0;
      /// Generate the next random number in the sequence within the given range.
      virtual double next() = 0;
    };
    
  }
}

#endif
