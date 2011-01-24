#ifndef MANTID_ALGORITHMS_DELETEWORKSPACE_H_
#define MANTID_ALGORITHMS_DELETEWORKSPACE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace Algorithms
  {
    /** 
      A simple algorithm to remove a workspace from the ADS. Basically so that it is logged
      and can be recreated from a history record
      
      @author Martyn Gigg, Tessella plc
      @date 2011-01-24
      
      Copyright &copy; 20011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
      
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
    class DLLExport DeleteWorkspace : public API::Algorithm
    {
    public:
      /// Algorithm's name
      virtual const std::string name() const { return "DeleteWorkspace"; }
      /// Algorithm's category for identification
      virtual const std::string category() const { return "General"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1; }

    private:
      /// Overridden init
      void init();
      /// Overridden exec
      void exec();

    };

  } // namespace Algorithm
} // namespace Mantid

#endif // MANTID_ALGORITHMS_DELETEWORKSPACE_H_
