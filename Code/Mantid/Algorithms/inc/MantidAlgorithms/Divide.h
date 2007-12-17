#ifndef MANTID_ALGORITHM_DIVIDE_H_
#define MANTID_ALGORITHM_DIVIDE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/Logger.h"
#include "MantidAPI/TripleRef.h" 

#include <algorithm>
#include <functional>
#include <iterator>

namespace Mantid
{
  namespace Algorithms
  {
    /** @class Divide Divide.h Algorithms/Divide.h


    Divide performs the division of two input workspaces.
    It inherits from the Algorithm class, and overrides
    the init(), exec() & final() methods.

    Required Properties:
    <UL>
    <LI> InputWorkspace1 - The name of the workspace </LI>
    <LI> InputWorkspace2 - The name of the workspace </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the division data </LI>
    </UL>

    @author Nick Draper
    @date 14/12/2007

    Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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
    */

    class DLLExport Divide : public API::Algorithm
    {
    public:
      /// Default constructor
      Divide() : API::Algorithm() {};
      /// Destructor
      virtual ~Divide() {};

    private:
      // Overridden Algorithm methods
      void init();
      void exec();
      void final();
      /// Static reference to the logger class
      static Mantid::Kernel::Logger& g_log;

      class Divide_fn : public std::binary_function<API::TripleRef<double&>,API::TripleRef<double&>,API::TripleRef<double&> >
      {
      public:
        /* Performs the Division with Gausian errors within the transform function
        * @param a The triple ref of the first workspace data item
        * @param b The triple ref of the second workspace data item
        * @returns A triple ref of the result with Gausian errors
        */
        API::TripleRef<double&> operator()(const API::TripleRef<double&>&,const API::TripleRef<double&>&);
      private:
        ///Temporary cache of calculated signal value
        double ret_sig;
        ///Temporary cache of calculated error value
        double ret_err;
      };

    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_DIVIDE_H_*/
