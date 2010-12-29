#ifndef ALGORITHMS_REBINTOWORKSPACE_H_
#define ALGORITHMS_REBINTOWORKSPACE_H_

#include "MantidAPI/PairedGroupAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{

namespace Algorithms
{
  /**
     Rebins a workspace so that the binning, for all its spectra, match that of the first spectrum
     of a second workspace.

     Required properties:
     <UL>
     <LI>WorkspaceToRebin - The workspace to rebin</LI>
     <LI>WorkspaceToMatch - The name of the workspace whose bin parameters are to be matched.</LI>
     <LI>OutputWorkspace - The name of the output workspace</LI>
     </UL>

     @author Martyn Gigg, Tessella Support Services plc
     @date 19/01/2009
     
     Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
     
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

class DLLExport RebinToWorkspace : public Mantid::API::PairedGroupAlgorithm
{
public:
  /// Constructor
  RebinToWorkspace() : Mantid::API::PairedGroupAlgorithm() {}
  /// Virtual destructor
  virtual ~RebinToWorkspace() {}
  /// Algorithm's name
  virtual const std::string name() const { return "RebinToWorkspace"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Rebin"; }

private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

  /// Create the rebin paraeters
  void createRebinParameters(Mantid::API::MatrixWorkspace_sptr toMatch, std::vector<double> & rb_params);
  
};

}

}

#endif /*ALGORITHMS_REBINTOWORKSPACE_H_*/
