#ifndef MANTID_ALGORITHM_DIAGSCRIPTINPUT_H_
#define MANTID_ALGORITHM_DIAGSCRIPTINPUT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace Algorithms
  {
    using namespace API;
    /**
    


    @author Steve D Williams, ISIS Facility Rutherford Appleton Laboratory
    @date 15/06/2009

    Copyright &copy; 2009 STFC Rutherford Appleton Laboratory

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
    class DLLExport DiagScriptInput : public Algorithm
    {
    public:
      /// Default constructor only runs the base class constructor
      DiagScriptInput() : Algorithm()                          //call the base class constructor
      {};
      /// Destructor
      virtual ~DiagScriptInput() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "DiagScriptInput";}
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return (1);}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Diagnostics";}

    protected:
      // Overridden Algorithm methods
      void init();
      void exec();
   };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_DIAGSCRIPTINPUT_H_*/
