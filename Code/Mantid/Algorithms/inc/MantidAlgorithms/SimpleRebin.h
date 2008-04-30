#ifndef MANTID_ALGORITHM_SIMPLEREBIN_H_
#define MANTID_ALGORITHM_SIMPLEREBIN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace Algorithms
  {
    /** 

    Takes a 1D workspace as input and rebins the data according to the input rebin parameters.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
    <LI> RebinParameters -  </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result </LI>
    </UL>

    Optional Properties (assume that you count from zero):
    <UL>
    <LI>   -  (default max)</LI>
    </UL>

    The algorithms used in the SimpleRebin::rebin() and SimpleRebin::newAxis() are based on the
    algorithms used in OPENGENIE /src/transform_utils.cxx (Freddie Akeroyd, ISIS)

    @author Dickon Champion, STFC
    @date 25/02/2008

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
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport SimpleRebin : public API::Algorithm
    {
    public:
      /// Default constructor
      SimpleRebin() : API::Algorithm() {};
      /// Destructor
      virtual ~SimpleRebin() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "SimpleRebin";}
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return 1;}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "General";}

    private:
      // Overridden Algorithm methods
      void init();
      void exec();
      void rebin(const std::vector<double>& , const std::vector<double>& , const std::vector<double>& , 
          const std::vector<double>& , std::vector<double>&, std::vector<double>&, bool);
      int newAxis(const std::vector<double>& , std::vector<double>& );       
        /// Static reference to the logger class
      static Mantid::Kernel::Logger& g_log;
    };



  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_SIMPLEREBIN_H_*/
