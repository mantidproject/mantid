#ifndef H_CONV2_MDEVENTS_DET_INFO
#define H_CONV2_MDEVENTS_DET_INFO
/** This structure is the basis and temporary replacement for future subalgorithm, which calculates 
   * matrix workspace with various precprocessed detectors parameters
   * 
   * @date 22-12-2011

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
#include "MantidKernel/System.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/V3D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/Progress.h"
  /** the lightweight class below contain 3D uint vectors, pointing to the positions of the detectors
      This vector used to preprocess and catch the partial positions of the detectors in Q-space
      to avoid repetative calculations, and (possibly) to write these data as part of the physical compression scheme
      in a very common situation when the physical instrument does not change in all runs, contributed into MD workspace
   */
namespace Mantid
{
namespace MDAlgorithms
{

struct PreprocessedDetectors{
    double L1;                       //< source-sample distance;
    Kernel::V3D   minDetPosition;    //< minimal and
    Kernel::V3D   maxDetPosition;    //< maxinal position for the detectors
    /** shared pointer to the base instrument, which was source of the detector's information. If the instrument changed, 
      * the detectors positions should be recalculated. 
      *  IT IS USER's RESPONSIBLITY TO RECALCULATE THE DETECTOR's PARAMETERS 
      *  if the derived instrument have changed in a way, which affects the detectors positions and TOF calculations */
    boost::shared_ptr< const Geometry::Instrument > pBaseInstr; 
    std::vector<Kernel::V3D>  det_dir; //< unit vector pointing from the sample to the detector;
    std::vector<double>       L2;      //< sample-detector distance
    std::vector<double>       TwoTheta; //< Diffraction angle
    std::vector<int32_t>      det_id;   //< the detector ID;
    std::vector<size_t>       detIDMap; //< stores spectra index which corresponds to a valid detector index;
    std::vector<size_t>       spec2detMap; //< stores detector index which corresponds to the workspace index;
    //
    bool isDefined(const API::MatrixWorkspace_const_sptr &inputWS)const;
    bool isDefined(size_t new_size)const{return det_dir.size()==new_size;}
    size_t nDetectors()const{return TwoTheta.size();}
    std::vector<double>const & getL2()const{return L2;}
    std::vector<double>const & getTwoTheta()const{return TwoTheta;}
    std::vector<size_t>const  & getDetIDMap()const{return detIDMap;}
    std::vector<size_t>const  & getSpec2DetMap()const{return spec2detMap;}
    std::vector<Kernel::V3D>const & getDetDir()const{return det_dir;}
    // function allocates the class detectors memory 
    void allocDetMemory(size_t nSpectra);
};

/** helper function, does preliminary calculations of the detectors positions to convert results into k-dE space ;
      and places the resutls into static cash to be used in subsequent calls to this algorithm */
void DLLExport processDetectorsPositions(const API::MatrixWorkspace_sptr inputWS,PreprocessedDetectors &det,Kernel::Logger& convert_log,API::Progress *pProg);
/** function builds fake cpectra-detector map and all other detectors position for the case when detector information has been lost */
void DLLExport buildFakeDetectorsPositions(const API::MatrixWorkspace_sptr inputWS,PreprocessedDetectors &det);
} // end MDAlgorithms
}
#endif