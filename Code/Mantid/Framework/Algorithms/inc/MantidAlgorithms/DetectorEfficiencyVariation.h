#ifndef MANTID_ALGORITHM_DETECTOREFFICIENCYVARIATION_H_
#define MANTID_ALGORITHM_DETECTOREFFICIENCYVARIATION_H_
/*WIKI* 


It is intended that the input white beam vanadium workspaces are from the same instrument and were collected before and after an experimental run of interest. First the ratios of the total number of counts in corresponding histograms from each input workspace are calculated and then the median ratio is calculated. Each ratio is compared to the median and a histogram will fail when any of the following conditions are true:
<ul>
 <li>(sum1/sum2)/median(sum1/sum2) > Variation</li>
 <li>(sum1/sum2)/median(sum1/sum2) < 1/Variation</li>
</ul>
where sum1 is the sum of the counts in a histogram in the workspace WhiteBeamBase and sum2 is the sum of the counts in the equivalent histogram in WhiteBeamCompare.  The above equations only make sense for identifying bad detectors if Variation > 1.  If a value of less than one is given for Variation then Variation will be set to the reciprocal.

The output workspace contains a MaskWorkspace where those spectra that fail the tests are masked and those that pass them are assigned a single positive value. 

====Subalgorithms used====

Uses the [[Integration]] algorithm to sum the spectra.


*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/DetectorDiagnostic.h"

namespace Mantid
{
  namespace Algorithms
  {
    /**
       Required Properties:
       <UL>
       <LI> WhiteBeamBase - Name of a white beam vanadium workspace </LI>
       <LI> WhiteBeamCompare - Name of a matching second white beam vanadium run from the same instrument </LI>
       <LI> OutputWorkspace - A MaskWorkpace where each spectra that failed the test is masked </LI>
       <LI> Variation - Identify spectra whose total number of counts has changed by more than this factor of the median change between the two input workspaces </LI>
       </UL>

       Optional Properties:
       <UL>
       <LI> StartWorkspaceIndex - The index number of the first entry in the Workspace to include in the calculation </LI>
       <LI> EndWorkspaceIndex - The index number of the last entry in the Workspace to include in the calculation </LI>
       <LI> RangeLower - No bin with a boundary at an x value less than this will be included in the summation used to decide if a detector is 'bad' </LI>
       <LI> RangeUpper - No bin with a boundary at an x value higher than this value will be included in the summation used to decide if a detector is 'bad' </LI>
       </UL>
    
       @author Steve D Williams, ISIS Facility Rutherford Appleton Laboratory
       @date 15/06/2009

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
    class DLLExport DetectorEfficiencyVariation : public DetectorDiagnostic
    {
    public:
      /// Default constructor
      DetectorEfficiencyVariation();
      /// Destructor
      virtual ~DetectorEfficiencyVariation() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "DetectorEfficiencyVariation";}
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return (1);}

    protected:
	// Overridden Algorithm methods
	void init();
	void exec();
      
	/// Loads and checks the values passed to the algorithm
	void retrieveProperties( API::MatrixWorkspace_sptr &whiteBeam1,
				 API::MatrixWorkspace_sptr &whiteBeam2, double &vari,
				 int &minSpec, int &maxSpec );
	/// Apply the detector test criterion
	int doDetectorTests(API::MatrixWorkspace_const_sptr counts1, 
			    API::MatrixWorkspace_const_sptr counts2,
			    const double average, double variation,
			    const std::set<int> & badIndices);

	/// the number of numbers on each line of the output file
	static const int LINESIZE = 10;

    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      /// when this is set to false reading and writing to the detector map is disabled, 
      /// this is done if there is no map in the workspace
      bool m_usableMaskMap;
    };

  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_DETECTOREFFICIENCYVARIATION_H_*/
