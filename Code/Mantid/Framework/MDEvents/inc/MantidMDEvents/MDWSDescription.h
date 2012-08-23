#ifndef MANTID_MDEVENTS_WS_DESCRIPTION_H
#define MANTID_MDEVENTS_WS_DESCRIPTION_H


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
public:  // for the time being
    /// the string which describes subalgorithm, used to convert source ws to target MD ws. At the moment, it coinsides with Q-mode
    std::string AlgID; 
    // the matrix which describes target coordiante system of the workpsace and connected with convert_to_factor;
    Kernel::DblMatrix m_Wtransf; 
    // UB matrix components:
    // Goniometer is always present in a workspace but can be a unit matrix
    Kernel::DblMatrix m_GoniomMatr;
    // the vector which represent linear form of momentun transformation 
    std::vector<double> m_RotMatrix;
   //=======================
/*---> accessors: */
    unsigned int         nDimensions()const{return m_NDims;}

    std::vector<std::string> getDimNames()const{return m_DimNames;}
    std::vector<std::string> getDimIDs()const{return m_DimIDs;}
    std::vector<std::string> getDimUnits()const{return m_DimUnits;}
    std::vector<double>      getDimMin()const{return m_DimMin;}
    std::vector<double>      getDimMax()const{return m_DimMax;}
    std::vector<size_t>      getNBins()const{return m_NBins;}
    std::vector<coord_t>     getAddCoord()const{return m_AddCoord;}
    std::string              getEModeStr()const;
    CnvrtToMD::EModes        getEMode()const{return m_Emode;}
    std::string              getQMode()const{return AlgID;}


    void getMinMax(std::vector<double> &min,std::vector<double> &max)const;
    std::vector<double> getTransfMatrix()const{return m_RotMatrix;}
    
    ConvToMDPreprocDet const * getDetectors(){return m_DetLoc;}
    ConvToMDPreprocDet const * getDetectors()const{return m_DetLoc;}

    API::MatrixWorkspace_const_sptr getInWS()                const{return m_InWS;}
    std::string getWSName()                                  const{return m_InWS->name();}
    bool isPowder()                                          const{return !m_InWS->sample().hasOrientedLattice();}
    bool hasLattice()                                        const{return m_InWS->sample().hasOrientedLattice();}
    bool isDetInfoLost()                                     const{return isDetInfoLost(m_InWS);}
    double getEi()                                           const{return getEi(m_InWS);}
    boost::shared_ptr<Geometry::OrientedLattice> getLattice()const{return getOrientedLattice(m_InWS);}

  /// constructor
  MDWSDescription(unsigned int nDimensions=0);

  /// method builds MD Event description from existing MD event workspace
  void buildFromMDWS(const API::IMDEventWorkspace_const_sptr &pWS);
  /// copy some parameters from the input workspace, as target md WS do not have all information about the algorithm.  
  void setUpMissingParameters(const MDEvents::MDWSDescription &SourceMatrixWorkspace);

  /// method builds MD Event ws description from a matrix workspace and the transformations, requested to be performed on the workspace
   void buildFromMatrixWS(const API::MatrixWorkspace_const_sptr &pWS,const std::string &QMode,const std::string dEMode,
                            const std::vector<std::string> &dimProperyNames = std::vector<std::string>());

  /// compare two descriptions and select the coplimentary result. 
   void checkWSCorresponsMDWorkspace(MDEvents::MDWSDescription &NewMDWorkspace);
 
   void setMinMax(const std::vector<double> &minVal,const std::vector<double> &maxVal);
   void setDimName(unsigned int nDim,const std::string &Name);
   // this is rather misleading function, as MD workspace do not have dimension units
   void setDimUnit(unsigned int nDim,const std::string &Unit);
   void setDetectors(const ConvToMDPreprocDet &g_DetLoc);
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
    unsigned int m_NDims;
    // shared pointer to the source matrix workspace
    API::MatrixWorkspace_const_sptr m_InWS;
    /// energy transfer analysis mode 
    CnvrtToMD::EModes m_Emode;
   // pointer to the array of detector's directions in the reciprocal space
    ConvToMDPreprocDet const * m_DetLoc;
    /// the vector of MD coordinates, which are obtained from workspace properties.
    std::vector<coord_t> m_AddCoord;
    /// the names for the target workspace dimensions and properties of input MD workspace
    std::vector<std::string> m_DimNames;
    /// the ID-s for the target workspace, which allow to identify the dimensions according to their ID
    std::vector<std::string> m_DimIDs;
    /// the units of target workspace dimensions and properties of input MD workspace dimensions
    std::vector<std::string> m_DimUnits;
    /// if defined, specifies number of bins split in each dimension
    std::vector<size_t> m_NBins;
    /// minimal and maximal values for the workspace dimensions. Usually obtained from WS parameters;
    std::vector<double>      m_DimMin,m_DimMax;
//********************* internal helpers
     /// helper function to resize all vectors, responsible for MD dimensions in one go
     void resizeDimDescriptions(unsigned int Dims,size_t nBins=10);

}; 

}
}
#endif