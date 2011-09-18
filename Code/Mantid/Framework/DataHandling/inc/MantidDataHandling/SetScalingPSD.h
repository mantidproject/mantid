#ifndef MANTID_DATAHANDLING_SETSCALINGPSD_H_
#define MANTID_DATAHANDLING_SETSCALINGPSD_H_
/*WIKI* 


This algorithm was developed for the Merlin instrument but may be used with other instruments if appropriate scaling data is available.
The scaling data should give the true centre point location of each pixel detector in the instrument.
This may be obtained by a calibration run and post-processing of the results.
Since the calibration data may vary with time, it is not convenient to store it in the instrument XML definition file.
Instead it can be stored as an ACSII file with the extension ".sca" or within the ".raw" file associated with the data, as data on the
position of each detector (r,theta,phi).

A scaling file (extension .sca) is expected to be an ASCII file with three header lines.
Of these, only the second line is actual read and the first item on this line should 
give the number of detectors described by the file as an integer value.
Each subsequent line after the first three will give the information for one detector with at least the five
ordered values <i>detector_ID</i>, <i>detector_offset</i>, <i>l2</i>, <i>code</i>, <i>theta</i> and <i>phi</i>.
Of these values only the <i>detector_ID</i> and the new position (<i>l2, theta, phi</i>) are used.
The latter three values are taken as defining the true position of the detector
in spherical polar coordinates relative to the origin (sample position).
If a raw file is given the true positions are taken from this instead.

This algorithm creates a parameter map for the instrument that applies a shift to each
detector so that is at the correct position.
Monitors are not moved.
Because the shift of detector locations can alter the effective width of the pixel
it is necessary to apply a scaling factor.
While each shift can have components in all three primary axes (X,Y,Z), it is assumed that a single PSD will maintain the co-linear nature of pixel centres.
The width scaling factor for a pixel <i>i</i> is approximated as average of the left and right side scalings cased by the change
in relative spacings with respect to neighbour pixels.
End of detector pixels only have one scaling value to use.
It is assumed that the scaling is both small and smooth so that the approximate scaling is reasonable.

Scaling and position correction will be reflected in properties of the detector objects including values such as the solid
angle, bounding box, etc.
The detector numbering in Merlin uses sequential numbers for pixels within a PSD and non-sequential jumps between PSDs.
This algorithm uses these jumps to identify individual PSDs.

To apply this algorithm to instruments other than Merlin it may be necessary to modify the code depending on the type of detectors
present and how they are numbered.

If the tube detector performance enhancement is used the results of the algorithm will not be visible in the instrument view in MantidPlot, at the same time all calclations will be performed correctly. 

=== Optional properties ===
ScalingOpt - this integer value controls the way in which the scaling is calculated for pixels that have both left and right values for the scaling.
The default is to just average the two together.
Setting this to 1 causes the maximum scaling to be used and setting it to 2 uses the maximum scaling plus 5% to be used.

===Subalgorithms used===
None


*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace DataHandling
  {
    /** @class SetScalingPSD SetScalingPSD.h MantidAlgorithm/SetScalingPSD.h

	Read the scaling information from a file (e.g. merlin_detector.sca) or from the RAW file (.raw)
	and adjusts the detectors positions and scaling appropriately.
	
	Required Properties:
	<UL>
	<LI> ScalingFilename - The path to the file containing the detector ositions to use either .raw or .sca</LI>
	<LI> Workspace - The name of the workspace to adjust </LI>
	</UL>
	Optional Properties:
	<UL>
	<LI> scalingOption - 0 => use average of left and right scaling (default). 
	1 => use maximum scaling. 
	2 => maximum + 5%</LI>
	</UL>

    @author Ronald Fowler

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>. 
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport SetScalingPSD : public API::Algorithm
    {
    public:
      /// Default constructor
      SetScalingPSD();

      /// Destructor
      ~SetScalingPSD() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "SetScalingPSD";};
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;};
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling\\Detectors";}

    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();

      /// Overwrites Algorithm method.
      void init();

      /// Overwrites Algorithm method
      void exec();

      /// The name and path of the input file
      std::string m_filename;
      /// An integer option controlling the scaling method
      int m_scalingOption;
      bool processScalingFile(const std::string& scalingFile, std::vector<Kernel::V3D>& truePos);
      API::MatrixWorkspace_sptr m_workspace; ///< Pointer to the workspace
      //void runMoveInstrumentComp(const int& detIndex, const Kernel::V3D& shift);

      /// get a vector of shared pointers to each detector in the comp
      void findAll(boost::shared_ptr<const Geometry::IComponent> comp);
      /// the vector of shared pointers
      std::vector<boost::shared_ptr<const Geometry::IComponent> > m_vectDet;
      /// apply the shifts in posMap to the detectors in WS
      void movePos(API::MatrixWorkspace_sptr& WS, std::map<int,Kernel::V3D>& posMap,
                            std::map<int,double>& scaleMap);
      /// read the positions of detectors defined in the raw file
      void getDetPositionsFromRaw(std::string rawfile,std::vector<int>& detID, std::vector<Kernel::V3D>& pos);
    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_SETSCALINGPSD_H_*/
