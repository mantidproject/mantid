#ifndef MANTID_ALGORITHM_CHANGEBINOFFSET_H_
#define MANTID_ALGORITHM_CHANGEBINOFFSET_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Workspace.h"

namespace Mantid
{
  namespace Algorithms
  {
    /**Takes a workspace and adjusts all the time bin values by the same amount.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result </LI>
    <LI> Offset - The number by which to change the time bins by</LI>
    </UL>
      
    @author 
    @date 11/07/2008

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>    
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport ChangeBinOffset : public API::Algorithm
    {
    public:
      /// Default constructor
      ChangeBinOffset();
      /// Destructor
      virtual ~ChangeBinOffset();
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "ChangeBinOffset";}
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Adjusts all the time bin values in a workspace by a specified amount.";}

      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Transforms\\Axes";}
	  /// Algorithm's Alternate Name
      virtual const std::string alias() const { return "OffsetX"; }

    private:
      
      // Overridden Algorithm methods
      void init();
      void exec();

      /// Execute algorithm for EventWorkspaces
      void execEvent();
    
      /// Create output workspace
      API::MatrixWorkspace_sptr createOutputWS(API::MatrixWorkspace_sptr input);
       
      /// The progress reporting object
      API::Progress *m_progress;

      /// Offset to shift by
      double offset;
      /// Start workspace index
      int64_t wi_min;
      /// Stop workspace index
      int64_t wi_max;
       
    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_CHANGEBINOFFSET_H_*/
