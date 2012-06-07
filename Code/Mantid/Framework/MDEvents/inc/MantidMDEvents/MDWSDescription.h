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
 /***  Class wrapping together all parameters, related to conversion from a workspace to MDEventoWorkspace 
    *
    *  used to provide common interface for subclasses, dealing with creation of MD workspace and conversion of 
    *  ws data into MDEvents
    *
    * It also defines some auxiliary functions, used for convenient description of MD workspace
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
public:
    /// the string which describes subalgorithm, used to convert source ws to target MD ws. 
    std::string AlgID; 
    // the matrix which describes target coordiante system of the workpsace and connected with convert_to_factor;
    Kernel::DblMatrix Wtransf; 
    // UB matrix components:
    // Goniometer is always present in a workspace but can be a unit matrix
    Kernel::DblMatrix GoniomMatr;
    // the vector which represent linear form of momentun transformation 
    std::vector<double> rotMatrix;
   //=======================
/*---> accessors: */
    unsigned int         nDimensions()const{return nDims;}

    std::vector<std::string> getDimNames()const{return dimNames;}
    std::vector<std::string> getDimIDs()const{return dimIDs;}
    std::vector<std::string> getDimUnits()const{return dimUnits;}
    std::vector<double>      getDimMin()const{return dimMin;}
    std::vector<double>      getDimMax()const{return dimMax;}
    std::vector<size_t>      getNBins()const{return nBins;}
    std::vector<coord_t>     getAddCoord()const{return AddCoord;}
    ConvertToMD::EModes      getEMode()const{return emode;}

    void getMinMax(std::vector<double> &min,std::vector<double> &max)const;
    std::vector<double> getTransfMatrix()const{return this->rotMatrix;}
    
    ConvToMDPreprocDet const * getDetectors(){return pDetLocations;}
    ConvToMDPreprocDet const * getDetectors()const{return pDetLocations;}


    API::MatrixWorkspace_const_sptr getInWS()const               {return inWS;}

    bool isPowder()const                                          {return !inWS->sample().hasOrientedLattice();}
    bool hasLattice()const                                        {return inWS->sample().hasOrientedLattice();}
    bool isDetInfoLost()const                                     {return isDetInfoLost(inWS);}
    double getEi()const                                           {return getEi(inWS);}
    boost::shared_ptr<Geometry::OrientedLattice> getLattice()const{return getOrientedLattice(inWS);}

    /// constructor
    MDWSDescription(unsigned int nDimensions=0);


    /// method builds MD Event description from existing MD event workspace
  void buildFromMDWS(const API::IMDEventWorkspace_const_sptr &pWS);
  /// copy some parameters from the input workspace, as target md WS do not have all information about the algorithm.  
  void setUpMissingParameters(const MDEvents::MDWSDescription &SourceMatrixWorkspace);

  /// method builds MD Event ws description from a matrix workspace and the transformations, requested to be performed on the workspace
   void buildFromMatrixWS(const API::MatrixWorkspace_const_sptr &pWS,const std::string &QMode,const std::string dEMode,
                            const std::vector<std::string> &dimProperyNames);

  /// compare two descriptions and select the coplimentary result. 
   void checkWSCorresponsMDWorkspace(MDEvents::MDWSDescription &NewMDWorkspace);
 
   void setMinMax(const std::vector<double> &minVal,const std::vector<double> &maxVal);
   void setDimName(unsigned int nDim,const std::string &Name);
   // this is rather misleading function, as MD workspace do not have dimension units
   void setDimUnit(unsigned int nDim,const std::string &Unit);
   void setDetectors(const ConvToMDPreprocDet &det_loc);
// static helper functions:
    /// helper function checks if min values are less them max values and are consistent between each other 
    static void checkMinMaxNdimConsistent(const std::vector<double> &minVal,const std::vector<double> &maxVal);
   /** Obtain input workspace energy (relevend in inelastic mode)*/
    static double   getEi(API::MatrixWorkspace_const_sptr inWS2D);
   /** function extracts the coordinates from additional workspace porperties and places them to AddCoord vector for further usage*/
    static void fillAddProperties(Mantid::API::MatrixWorkspace_const_sptr inWS2D,const std::vector<std::string> &dimProperyNames,std::vector<coord_t> &AddCoord);
    /** checks if matrix ws has information about detectors*/
    static bool isDetInfoLost(Mantid::API::MatrixWorkspace_const_sptr inWS2D);

    static boost::shared_ptr<Geometry::OrientedLattice> getOrientedLattice(Mantid::API::MatrixWorkspace_const_sptr inWS2D);
private:
    /// the variable which describes the number of the dimensions, in the target workspace. 
    /// Calculated from number of input properties and the operations, performed on input workspace;
    unsigned int nDims;
    // shared pointer to the source matrix workspace
    API::MatrixWorkspace_const_sptr inWS;
   // pointer to the array of detector's directions in the reciprocal space
    ConvToMDPreprocDet const * pDetLocations;
    /// energy transfer analysis mode 
    ConvertToMD::EModes emode;
    /// the vector of MD coordinates, which are obtained from workspace properties.
    std::vector<coord_t> AddCoord;
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
//********************* internal helpers
     /// helper function to resize all vectors, responsible for MD dimensions in one go
     void resizeDimDescriptions(unsigned int Dims,size_t nBins=10);

}; 

}
}
#endif