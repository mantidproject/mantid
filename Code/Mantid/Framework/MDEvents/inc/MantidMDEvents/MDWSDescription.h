#ifndef MANTID_MDEVENTS_WS_DESCRIPTION_H
#define MANTID_MDEVENTS_WS_DESCRIPTION_H


#include "MantidKernel/PropertyManager.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

#include "MantidMDEvents/MDEvent.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/LogManager.h"


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
    *
    * See http://www.mantidproject.org/Writing_custom_ConvertTo_MD_transformation for detailed description of this
    * class place in the algorithms hierarchy. 
    *
        
    @date 2011-28-12

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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


/// helper class describes the properties of target MD workspace, which should be obtained as the result of conversion algorithm. 
    class DLLExport MDWSDescription : public API::LogManager
{
public:  // for the time being
    /// the string which describes ChildAlgorithm, used to convert source ws to target MD ws. At the moment, it coincides with Q-mode
    std::string AlgID; 
    // the matrix which describes target coordinate system of the workspace and connected with convert_to_factor;
    Kernel::DblMatrix m_Wtransf; 
    // the vector which represent linear form of momentum transformation 
    std::vector<double> m_RotMatrix;

    // preprocessed detectors workspace:
    DataObjects::TableWorkspace_const_sptr m_PreprDetTable;
    //helper parameter, which identifies if we are building new workspace or adding data to the existing one. Allows to generate clearer error messages
    bool m_buildingNewWorkspace;
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
    Kernel::DeltaEMode::Type getEMode()const{return m_Emode;}
    std::string              getQMode()const{return AlgID;}


    /**check if one needs to perform Lorentz corrections */
    bool isLorentsCorrections()const{return m_LorentzCorr;}
    void getMinMax(std::vector<double> &min,std::vector<double> &max)const;
    std::vector<double> getTransfMatrix()const{return m_RotMatrix;}
    
    // workspace related helper functions, providing access to various workspace functions
    API::MatrixWorkspace_const_sptr getInWS()const{return m_InWS;}
    void setWS(API::MatrixWorkspace_sptr otherMatrixWS);
    std::string getWSName()const{return m_InWS->name();}
    bool isPowder()const;
    bool hasLattice()const{return m_InWS->sample().hasOrientedLattice();}

    boost::shared_ptr<Geometry::OrientedLattice> getLattice()const{return getOrientedLattice(m_InWS);}
    Kernel::Matrix<double> getGoniometerMatr()const;
    bool hasGoniometer()const;

  /// constructor
  MDWSDescription(unsigned int nDimensions=0);

  /// method builds MD Event description from existing MD event workspace
  void buildFromMDWS(const API::IMDEventWorkspace_const_sptr &pWS);
  /// copy some parameters from the input workspace, as target md WS do not have all information about the algorithm.  
  void setUpMissingParameters(const MDEvents::MDWSDescription &SourceMatrixWorkspace);

  /// method builds MD Event ws description from a matrix workspace and the transformations, requested to be performed on the workspace
   void buildFromMatrixWS(const API::MatrixWorkspace_sptr &pWS,const std::string &QMode,const std::string dEMode,
                            const std::vector<std::string> &dimProperyNames = std::vector<std::string>());

  /// compare two descriptions and select the complimentary result. 
   void checkWSCorresponsMDWorkspace(MDEvents::MDWSDescription &NewMDWorkspace);
 
   void setMinMax(const std::vector<double> &minVal,const std::vector<double> &maxVal);
   void setDimName(unsigned int nDim,const std::string &Name);
   // this is rather misleading function, as MD workspace do not have dimension units
   void setDimUnit(unsigned int nDim,const std::string &Unit);
   /** do we need to perform Lorentz corrections */ 
   void setLorentsCorr(bool On=false){m_LorentzCorr=On;}
// static helper functions:
    /// helper function checks if min values are less them max values and are consistent between each other 
    static void checkMinMaxNdimConsistent(const std::vector<double> &minVal,const std::vector<double> &maxVal);
    /** function extracts the coordinates from additional workspace properties and places them to AddCoord vector for further usage*/
    static void fillAddProperties(Mantid::API::MatrixWorkspace_const_sptr inWS2D,const std::vector<std::string> &dimProperyNames,std::vector<coord_t> &AddCoord);

    static boost::shared_ptr<Geometry::OrientedLattice> getOrientedLattice(Mantid::API::MatrixWorkspace_const_sptr inWS2D);

    /// Set the special coordinate system if any.
    void setCoordinateSystem(const Mantid::API::SpecialCoordinateSystem system);
    /// @return the special coordinate system if any.
    Mantid::API::SpecialCoordinateSystem getCoordinateSystem() const;
    /// sets number of bins each dimension is split
    void setNumBins(const std::vector<int> &nBins);
protected: // until MDWSDesctiptionDepricatedExist
    /// the variable which describes the number of the dimensions, in the target workspace. 
    /// Calculated from number of input properties and the operations, performed on input workspace;
    unsigned int m_NDims;
    // shared pointer to the source matrix workspace
    API::MatrixWorkspace_sptr m_InWS;
    /// energy transfer analysis mode 
    Kernel::DeltaEMode::Type m_Emode;
    /// if one needs to calculate Lorentz corrections
    bool m_LorentzCorr;
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

private:
  /// Coordinate system.
  Mantid::API::SpecialCoordinateSystem m_coordinateSystem;
}; 

}
}
#endif