#ifndef H_MDEVENT_WS_DESCRIPTION
#define H_MDEVENT_WS_DESCRIPTION


#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/ConvToMDPreprocDet.h"
#include "MantidMDEvents/MDTransfDEHelper.h"


namespace Mantid
{
namespace MDEvents
{
 /***  Lighteweight class wrapping together all parameters, related to conversion from a workspace to MDEventoWorkspace 
    *  used mainly to reduce number of parameters trasferred between  an algorithm, creating a MD workspace and the UI.
    *
    * It also defines some auxiliary functions, used for convenient description of MD workspace, see below. 
    *   
        
    @date 2011-28-12

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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


/// helper class describes the properties of target MD workspace, which should be obtained as the result of conversion algorithm. 
class DLLExport MDWSDescription
{
    // shared pointer to the source matrix workspace
    API::MatrixWorkspace_const_sptr inWS;
   // pointer to the array of detector's directions in the reciprocal space
    ConvToMDPreprocDet const * pDetLocations;
    /// energy transfer analysis mode 
    ConvertToMD::EModes emode;
    /// energy of incident neutrons, relevant to inelastic modes
    double Ei; 
    /// the vector of MD coordinates, which are obtained from workspace properties.
    std::vector<coord_t> AddCoord;
    /// the variable which describes the number of the dimensions, in the target workspace. 
    /// Calculated from number of input properties and the operations, performed on input workspace;
    unsigned int nDims;
    /// the names for the target workspace dimensions and properties of input MD workspace
    std::vector<std::string> dimNames;
    /// the ID-s for the target workspace, which allow to identify the dimensions according to their ID
    std::vector<std::string> dimIDs;
    /// the units of target workspace dimensions and properties of input MD workspace dimensions
    std::vector<std::string> dimUnits;
    /// if defined, specifies number of bins split in each dimension
    std::vector<size_t> nBins;
    /// minimal and maximal values for the workspace dimensions. Usually obtained from WS parameters;
    std::vector<double>      dimMin,dimMax;
    /// shows if source workspace still has information about detectors. Some ws (like rebinned one) do not have this information any more. 
    bool detInfoLost;
public:
    /// the string which describes subalgorithm, used to convert source ws to target MD ws. 
    std::string AlgID; 
    // the matrix which describes target coordiante system of the workpsace and connected with convert_to_factor;
    Kernel::DblMatrix Wtransf; 
    // UB matrix components:
    /// the oriented lattice which should be picked up from source ws and be carryed out to target ws. Defined for transfromation from Matrix or Event WS
    std::auto_ptr<Geometry::OrientedLattice> pLatt;
    // Goniometer is always present in a workspace but can be a unit matrix
    Kernel::DblMatrix GoniomMatr;
    // the vector which represent linear form of momentun transformation 
    std::vector<double> rotMatrix;
   //=======================
/**> helper functions: To assist with units conversion done by separate class and get access to some important internal states of the subalgorithm */
    unsigned int         nDimensions()const{return nDims;}

    std::vector<std::string> getDimNames()const{return dimNames;}
    std::vector<std::string> getDimIDs()const{return dimIDs;}
    std::vector<std::string> getDimUnits()const{return dimUnits;}
    std::vector<double>      getDimMin()const{return dimMin;}
    std::vector<double>      getDimMax()const{return dimMax;}
    std::vector<size_t>      getNBins()const{return nBins;}
    std::vector<coord_t>     getAddCoord()const{return AddCoord;}
    API::MatrixWorkspace_const_sptr getInWS()const{return inWS;}


    bool isDetInfoLost()const{return detInfoLost;}
    bool isPowder()const;

    double               getEi()const{return Ei;}
    double               getEi(API::MatrixWorkspace_const_sptr inWS2D)const;
    ConvertToMD::EModes  getEMode()const{return emode;}
    std::vector<double> getTransfMatrix()const{return this->rotMatrix;}
    //
    void getMinMax(std::vector<double> &min,std::vector<double> &max)const;
    
    ConvToMDPreprocDet const * getDetectors(){return pDetLocations;}
    ConvToMDPreprocDet const * getDetectors()const{return pDetLocations;}

      /// constructor
     MDWSDescription(unsigned int nDimensions=0);
     /// method builds MD Event description from existing MD event workspace
     void buildFromMDWS(const API::IMDEventWorkspace_const_sptr &pWS);
     /// method builds MD Event ws description from a matrix workspace and the transformations, requested to be performed on the workspace
     void buildFromMatrixWS(const API::MatrixWorkspace_const_sptr &pWS,const std::string &QMode,const std::string dEMode,
                            const std::vector<std::string> &dimProperyNames,size_t maxNdims);

     /// compare two descriptions and select the coplimentary result.
     void compareDescriptions(MDEvents::MDWSDescription &NewMDWorkspace);
     /// copy some parameters from the target workspace;
     void setUpMissingParameters(const MDEvents::MDWSDescription &SourceMDWorkspace);
     // default does not do any more;
     MDWSDescription & operator=(const MDWSDescription &rhs);

   /// set up min and max values for dimensions in MD workspace
   void setMinMax(const std::vector<double> &minVal,const std::vector<double> &maxVal);
   /// set non-default dimension name
   void setDimName(unsigned int nDim,const std::string &Name);
   void setDimUnit(unsigned int nDim,const std::string &Unit);
   void setDetectors(const ConvToMDPreprocDet &det_loc);
private:
     // let's hide copy constructor for the time being as defaults are incorrect and it is unclear if one is needed.
     MDWSDescription(const MDWSDescription &);

   /** function extracts the coordinates from additional workspace porperties and places them to AddCoord vector for further usage*/
     void fillAddProperties(Mantid::API::MatrixWorkspace_const_sptr inWS2D,const std::vector<std::string> &dimProperyNames,std::vector<coord_t> &AddCoord)const;

     /// helper function to resize all vectors, responsible for MD dimensions in one go
     void resizeDimDescriptions(unsigned int Dims,size_t nBins=10);

     /// helper function checks if min values are less them max values and are consistent between each other 
    void checkMinMaxNdimConsistent()const;

}; 

}
}
#endif