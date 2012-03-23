#ifndef H_MDEVENT_WS_DESCRIPTION
#define H_MDEVENT_WS_DESCRIPTION

#include "MantidMDEvents/MDEvent.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

namespace Mantid
{
namespace MDEvents
{
 /***  Lighteweith class wrapping together all parameters, related to conversion from a workspace to MDEventoWorkspace 
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

/** describes default dimensions ID currently used by multidimensional workspace
 * 
 *  DimensionID is the short name which used to retrieve this dimesnion from MD workspace.
 *  The names themself are defined in constructor
 */
enum defaultDimID
{
    ModQ_ID,  //< the defauld |Q| id for mod Q or powder mode
    Q1_ID,    //< 1 of 3 dimID in Q3D mode
    Q2_ID,    //< 2 of 3 dimID in Q3D mode
    Q3_ID,    //< 3 of 3 dimID in Q3D mode
    dE_ID,    //< energy transfer ID
    nDefaultID //< ID conunter
};

/// enum descrines availble momentum scalings, interpreted by this class: TODO: Reconsile this with future third 
enum CoordScaling
{ 
    NoScaling, //< momentums in A^-1
    SingleScale, //< momentuns divided by  2*Pi/Lattice -- equivalend to d-spacing in some sence
    OrthogonalHKLScale,  //< each momentum component divided by appropriate lattice parameter; equivalent to hkl for orthogonal axis
    HKLScale,            //< non-orthogonal system for non-orthogonal lattice
    NCoordScalings
}; 


/// helper class describes the properties of target MD workspace, which should be obtained as the result of conversion algorithm. 
class DLLExport MDWSDescription
{
  public:
    /// the variable which describes the number of the dimensions, in the target workspace. 
    /// Calculated from number of input properties and the operations, performed on input workspace;
    size_t nDims;
    /// conversion mode (see its description below)
    int emode;
    /// energy of incident neutrons, relevant in inelastic mode
    double Ei; 
    /// minimal and maximal values for the workspace dimensions:
    std::vector<double>      dimMin,dimMax;
    /// the names for the target workspace dimensions and properties of input MD workspace
    std::vector<std::string> dimNames;
    /// the ID-s for the target workspace, which allow to identify the dimensions according to their ID
    std::vector<std::string> dimIDs;
    /// the units of target workspace dimensions and properties of input MD workspace dimensions
    std::vector<std::string> dimUnits;
    /// if defined, specifies number of bins in each dimension
    std::vector<size_t> nBins;
    /** the swich, specifying if the target Q3D -dimensions should be converted to hkl. Ignored in NoQ and powder mode (but used in cryst as powder) 
       and if no oriented lattice is found in input ws. */
    CoordScaling convert_to_factor;
    /// the matrix to transform momentums of the workspace into target coordinate system, it is constructed from UB matix and W-matrix;
    std::vector<double> rotMatrix;  // can be Quat if not for non-orthogonal lattices

    /// the string which describes subalgorithm, used to convert source ws to target MD ws. 
    std::string AlgID; 

    // UB matrix components:
    /// the oriented lattice which should be picked up from source ws and be carryed out to target ws. Defined for transfromation from Matrix or Event WS
    std::auto_ptr<Geometry::OrientedLattice> pLatt;
    // Goniometer is always present in a workspace but can be a unit matrix
    Kernel::DblMatrix GoniomMatr;   
  /// the matrix transforming Q-coodinates in crystal cartesian coordinate system into target coodinate system. 
    Kernel::DblMatrix  Wtransf;
    /// shows if source workspace still has information about detectors. Some ws (like rebinned one) do not have this information any more. 
    bool detInfoLost;
//=======================
      /// constructor
     MDWSDescription(size_t nDimesnions=0);
     /// function build MD Event description from existing workspace
     void build_from_MDWS(const API::IMDEventWorkspace_const_sptr &pWS);
     /// compare two descriptions and select the coplimentary result.
     void compareDescriptions(MDEvents::MDWSDescription &NewMDWorkspaceD);

    /// helper function checks if min values are less them max values and are consistent between each other 
    void checkMinMaxNdimConsistent(Mantid::Kernel::Logger& log)const;
    // default does not do any more;
    MDWSDescription & operator=(const MDWSDescription &rhs);

    /// function returns default dimension id-s for different Q and dE modes, defined by this class
    std::vector<std::string> getDefaultDimIDQ3D(int dEmode)const;
    std::vector<std::string> getDefaultDimIDModQ(int dEmode)const;
  /// return the list of possible scalings for momentums
   std::vector<std::string> getQScalings()const{return QScalingID;}
   CoordScaling getQScaling(const std::string &ScID)const;
  private:
      // let's hide copy constructor for the time being as defaults are incorrect and it is unclear if one is needed.
      MDWSDescription(const MDWSDescription &);
     /// the vector describes default dimension names, specified along the axis if no names are explicitly requested;
     std::vector<std::string> default_dim_ID;
     ///
     std::vector<std::string> QScalingID;


}; 
/** function to build mslice-like axis name from the vector, which describes crystallographic direction along this axis*/
std::string DLLExport makeAxisName(const Kernel::V3D &vector,const std::vector<std::string> &Q1Names);
/**creates string representation of the number with accuracy, cpecified by eps*/
std::string DLLExport sprintfd(const double data, const double eps);

}
}
#endif