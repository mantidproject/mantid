#ifndef MANTID_ALGORITHM_MINUSEXPLICIT_H_
#define MANTID_ALGORITHM_MINUSEXPLICIT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace Algorithms
  {
    /** @class Minus Minus.h Algorithms/Minus.h


    Minus performs the difference of two input workspaces.
    It inherits from the Algorithm class, and overrides
    the init() & exec() methods.

    Required Properties:
    <UL>
    <LI> InputWorkspace1 - The name of the workspace </LI>
    <LI> InputWorkspace2 - The name of the workspace </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the difference data </LI>
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

    class DLLExport MinusExplicit : public API::Algorithm
    {
    public:
      /// Default constructor
      MinusExplicit():API::Algorithm(){}
      /// Destructor
      virtual ~MinusExplicit() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "MinusExplicit";}
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return 1;}

    private:
    	void init();
    	void exec();
      /// Static reference to the logger class
      static Mantid::Kernel::Logger& g_log;

    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_MINUSEXPLICIT_H_*/
