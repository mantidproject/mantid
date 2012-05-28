#ifndef H_CONV2_MDEVENTS_DET_INFO_
#define H_CONV2_MDEVENTS_DET_INFO_
/** This structure is the basis and temporary replacement for future subalgorithm, which calculates 
   * matrix workspace with various precprocessed detectors parameters.
   * The lightweight class below contain 3D uint vectors, pointing to the positions of the detectors
   *
   *  This vector used to preprocess and catch the partial positions of the detectors in Q-space
   *  to avoid repetative calculations, and (possibly) to write these data as part of the physical compression scheme
   *  in a very common situation when the physical instrument does not change in all runs, contributed into MD workspace
   *
   *  IT IS USER's RESPONIBLITY TO RECALCULATE THE DETECTOR's PARAMETERS 
   *  if the derived instrument have changed in a way, which affects the detectors positions and TOF calculations 

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
namespace Mantid
{
namespace MDEvents
{
/** Class to calculate/keep informaion about detector's positions, this information is used for doing 
  * various transformation of signals into reciprocal place and stored by this class for efficiency and 
  * additional convenience. 

  * @date 26-04-2012

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
 

class DLLExport ConvToMDPreprocDet{
    /** shared pointer to the base instrument, which was source of the detector's information. If the instrument changed, 
      * the detectors positions should be recalculated. 
      *  IT IS USER's RESPONIBLITY TO RECALCULATE THE DETECTOR's PARAMETERS 
      *  if the derived instrument have changed in a way, which affects the detectors positions and TOF calculations */
    boost::shared_ptr< const Geometry::Instrument > pBaseInstr; 
public:    
    // checks if preprocessed detectors were calculated
    bool isDefined(const API::MatrixWorkspace_const_sptr &inputWS)const;
    bool isDefined(size_t new_size)const{return det_dir.size()==new_size;}

    size_t nDetectors()const{return TwoTheta.size();}

    std::vector<double>const & getL2()const{return L2;}
    std::vector<double>const & getTwoTheta()const{return TwoTheta;}
    std::vector<size_t>const  & getDetIDMap()const{return detIDMap;}
    std::vector<size_t>const  & getSpec2DetMap()const{return spec2detMap;}
    std::vector<Kernel::V3D>const & getDetDir()const{return det_dir;}

    /// obtain detector id, which correspond to the spectra index
    size_t  getWSDet(size_t iws)const{return spec2detMap[iws];}
    /// obtain spectra id, corrsponing to detector id;
    size_t  getDetSpectra(size_t i)const{return detIDMap[i];}
    int32_t getDetID(size_t i)const{return det_id[i];}

    int    getEmode()const{return emode;}
    double getEfix()const{return efix;}
    double getL1()const{return L1;}

    void setEmode(int mode);
    void setEfix(double Ei);
    void setL1(double Dist);

   /** function, does preliminary calculations of the detectors positions to convert results into k-dE space */     
    void processDetectorsPositions(const API::MatrixWorkspace_sptr inputWS, Kernel::Logger& convert_log,API::Progress *pProg);
   /** function builds fake cpectra-detector map and all other detectors position for the case when detector information has been lost */
    void buildFakeDetectorsPositions(const API::MatrixWorkspace_sptr inputWS);

    void clearAll();
    ConvToMDPreprocDet();
private:
    // function allocates the class detectors memory 
    void allocDetMemory(size_t nSpectra);

    void setEi(const API::MatrixWorkspace_sptr inputWS);


   // parameter which describes the conversion mode, used to convert uints using TOF and detector's positions
    int emode;
    // parameter wjocj describes the energy used to convert uints using TOF and detector's positions
    double efix;
    // source-sample distance  used to convert uints using TOF and detector's positions
    double L1; 
    // minimal position for the detectors
    Kernel::V3D   minDetPosition;    
    // maxinal position for the detectors
    Kernel::V3D   maxDetPosition;    

     // unit vector pointing from the sample to the detector;
    std::vector<Kernel::V3D>  det_dir;
    // sample-detector distance
    std::vector<double>       L2;     
    // Diffraction angle
    std::vector<double>       TwoTheta;
    // the detector ID;
    std::vector<int32_t>      det_id;  
    // stores spectra index which corresponds to a valid detector index;
    std::vector<size_t>       detIDMap; 
    // stores detector index which corresponds to the workspace index;
    std::vector<size_t>       spec2detMap; 
};


} // end MDAlgorithms
}
#endif