#ifndef MANTID_ALGORITHM_DIFFRACTIONFOCUSSING_H_
#define MANTID_ALGORITHM_DIFFRACTIONFOCUSSING_H_
/*WIKI* 

[[Image:GEM Focused.png|200px|thumb|right|Example of RAW GEM data focused across the 5 detector banks]] 
Given an InputWorkspace and a Grouping filename, the algorithm performs the following:
# The calibration file is read and a map of corresponding udet-group is created.
# The algorithm determine the X boundaries for each group as the upper and lower limits of all contributing detectors to this group and determine a logarithmic step that will ensure preserving the number of bins in the initial workspace.
# All histograms are read and rebinned to the new grid for their group.
# A new workspace with N histograms is created.

Within the [[CalFile]] any detectors with the 'select' flag can be set to zero or with a group number of 0 or -ve groups are not included in the analysis.

Since the new X boundaries depend on the group and not the entire workspace,
this focusing algorithm does not create overestimated X ranges for multi-group instruments.
However it is important to remember that this means that this algorithm outputs a [[Ragged_Workspace|ragged workspace]].  Some 2D and 3D plots will not display the data correctly.

The DiffractionFocussing algorithm uses GroupDetectors algorithm to combine data from several spectra according to GroupingFileName file which is a [[CalFile]].

===For EventWorkspaces===

The algorithm can be used with an [[EventWorkspace]] input, and will create an EventWorkspace output if a different workspace is specified.

The main difference vs. using a Workspace2D is that the event lists from all the incoming pixels are simply appended in the grouped spectra; this means that you can rebin the resulting spectra to finer bins with no loss of data. In fact, it is unnecessary to bin your incoming data at all; binning can be performed as the very last step.

==Usage==
'''Python'''
    DiffractionFocussing("InWS","OutWS","filename")
'''C++'''
    IAlgorithm* alg = FrameworkManager::Instance().createAlgorithm("DiffractionFocussing");
    alg->setPropertyValue("InputWorkspace", "InWS"); 
    alg->setPropertyValue("OutputWorkspace", "OutWS");    
    alg->setPropertyValue("GroupingFileName", "filename");
    alg->execute();
    Workspace* ws = FrameworkManager::Instance().getWorkspace("OutWS");


*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include <Poco/NObserver.h>

namespace Mantid
{
  namespace Algorithms
  {
    /** 
    This is a parent algorithm that uses several different child algorithms to perform it's task.
    Takes a workspace as input and the filename of a grouping file of a suitable format.
    
    The input workspace is 
    1) Converted to d-spacing units
    2) Rebinnned to a common set of bins
    3) The spectra are grouped according to the grouping file.
    
	Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the 2D Workspace to take as input </LI>
    <LI> GroupingFileName - The path to a grouping file</LI>
    <LI> OutputWorkspace - The name of the 2D workspace in which to store the result </LI>
    </UL>

    The structure of the grouping file is as follows:
    # Format: number  UDET offset  select  group
    0        611  0.0000000  1    0
    1        612  0.0000000  1    0
    2        601  0.0000000  0    0
    3        602  0.0000000  0    0
    4        621  0.0000000  1    0

   
    @author Nick Draper, Tessella
    @date 11/07/2008

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport DiffractionFocussing : public API::Algorithm, public API::DeprecatedAlgorithm
    {
    public:
      /// Constructor
      DiffractionFocussing();
      /// Destructor
      virtual ~DiffractionFocussing() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "DiffractionFocussing";}
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Diffraction";}
    
    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      // Overridden Algorithm methods
      void init();
      void exec();
      API::MatrixWorkspace_sptr convertUnitsToDSpacing(const API::MatrixWorkspace_sptr& workspace);
      void RebinWorkspace(API::MatrixWorkspace_sptr& workspace);
      void calculateRebinParams(const API::MatrixWorkspace_const_sptr& workspace,double& min,double& max,double& step);
      bool readGroupingFile(std::string groupingFileName, std::multimap<int64_t,int64_t>& detectorGroups);

    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_DIFFRACTIONFOCUSSING_H_*/
