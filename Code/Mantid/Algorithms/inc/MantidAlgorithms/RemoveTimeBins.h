#ifndef MANTID_ALGORITHM_REMOVETIMEBINS_H_
#define MANTID_ALGORITHM_REMOVETIMEBINS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid
{
	
  namespace Algorithms
  {
   /**Removes time bins from either the front or the back of a workspace. Do not use to remove time
	  bins from the middle!

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result </LI>
    <LI> XMin - The time bin to start removing from (including the bin specified). Note: is zero based, i.e. first bin has index 0.</LI>
    <LI> XMax - The time bin to end removing from (including the bin specified). Note: is zero based. </LI>
    <LI> Interpolation - The type of interpolation to use for removed bins. </LI>
    </UL>
      
    @author 
    @date 11/07/2008

    Copyright &copy; 2008 STFC Rutherford Appleton Laboratories

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

    
    class DLLExport RemoveTimeBins : public API::Algorithm
    {
    public:
      /// Default constructor
      RemoveTimeBins() : API::Algorithm() {};
      /// Destructor
      virtual ~RemoveTimeBins() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "RemoveBins";}
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return 1;}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "General";}

    private:
      // Overridden Algorithm methods
      void init();
      void exec();
      void RemoveFromEnds(Mantid::API::MatrixWorkspace_const_sptr inputW, 
		Mantid::API::MatrixWorkspace_sptr outputW, int numHists, int start, int end);
    
      void RemoveFromMiddle(Mantid::API::MatrixWorkspace_const_sptr inputW, 
		Mantid::API::MatrixWorkspace_sptr outputW, int numHists, int start, int end);
            
      /// Static reference to the logger class
      static Mantid::Kernel::Logger& g_log;
    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_RemoveTimeBins_H_*/
