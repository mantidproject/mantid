#ifndef MD_GEOMETRY_BASIS_H
#define MD_GEOMETRY_BASIS_H

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include <string>
#include <map>
#include <set>
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidGeometry/MDGeometry/MDWorkspaceConstants.h"

/** The class is the part of the VisualisationWorkspace and describes the basic multidimentional geometry of the object, 
*   e.g. the dimensions of the reciprocal space and other possible dimenions  
*   the reference reciprocal lattice, the size and shape of a primary crystall cell
*   and number of additional ortogonal dimensions, e.g. temperature, pressure etc. 
*
*   Class provides the reference framework for visualisation and analysis operations alowing to compare different workspaces and to modify 
*   the visualisation geometry as requested by user
*   It also keeps the crystall information necessary to transform a Mantid workspace into MD workspace

    @author Alex Buts, RAL ISIS
    @date 27/09/2010

    Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

namespace Mantid
{
    namespace Geometry
    {

//****************************************************************************************************************************************
    class DLLExport MDGeometryBasis
    {
    public:

/**  The class DimensionID describes one of the set of the dimensions, which can be present in dataset;
*    it has just keeps the name of the dimension and the number which identifies, what data column corresponds to this tag  
*/
   class DimensionID
    {
    public:
        /// returns  the symbol tag of this dimension ID;
        std::string getDimensionTag(void)const{return DimensionTag;}
        /// returns true if this dimension is reciprocal
        bool isReciprocal(void)const{return is_reciprocal;}
        /// all dimensions has to be arranged in the order of incrreased ascii string value;
        bool operator <(const DimensionID &other)const{return ((this->DimensionTag.compare(other.DimensionTag)< 0)?true:false);}
        bool operator<=(const DimensionID &other)const{return ((this->DimensionTag.compare(other.DimensionTag)<=0)?true:false);}
        bool operator> (const DimensionID &other)const{return ((this->DimensionTag.compare(other.DimensionTag)> 0)?true:false);}
        bool operator>=(const DimensionID &other)const{return ((this->DimensionTag.compare(other.DimensionTag)>=0)?true:false);}
        bool operator==(const DimensionID &other)const{return ((this->DimensionTag.compare(other.DimensionTag)==0)?true:false);}
        /// copy constructor and operator()= are public
       /// to initate array, which has to be reset as meaningless and not checked against this later
        DimensionID(int iDimNum0=-1,const char *name0="",bool if_reciprocal=true):
        iDimNum(iDimNum0),is_reciprocal(if_reciprocal),DimensionTag(name0){}; 
        /// function compares the tag of this DimensionID with the input tag and returns the numeric value of the ID if the names coinside or -1 if not. 
        int getDimNum(const std::string &aTag)const{return (this->DimensionTag.compare(aTag)==0)?this->iDimNum:-1;}
        /// this should be prviate or protected?;
        int getDimNum(void)const{return iDimNum;}
       /// this should be reserved to friends?
       void setDimensionIDValues(int newNum,const std::string &newTag,bool recipocal=false){
         iDimNum=newNum;is_reciprocal=recipocal;DimensionTag.assign(newTag);}
    private:
        /// actual Dimension number used to identify the dimension; Strictly internal or shared with MDGeometryBasis
       int iDimNum; 
         /// number of reciprocal dimensions (which are non-orthogonal to each other n_rsprcl_dim<=n_total_dim)
       bool is_reciprocal;
        /// default dimension name used to identify a dimension among others;
        std::string DimensionTag;
  
    };
//************* ACCESSORS    ****************************************************************************************    

    virtual ~MDGeometryBasis(void);
    /// return the numbers of dimensions in current geometry; 
    unsigned int getNumDims(void)const{return n_total_dim;}
    /// returns the number of reciprocal dimensions
    unsigned int getNumReciprocalDims(void)const{return this->n_reciprocal_dimensions;};

    /// function returns the vector of the names of the dimensions, which are defined in the workspace
    std::vector<std::string> getBasisTags(void)const;
    /// function returns the workspace ID, which defines the workspace with particular kind of dimensions
    std::string getWorkspaceIDname()const{return workspace_name;}
 
  //TO DO: Define this operations better on the basis of algorithm to build MD from Mantid Workspaces;
    // ort of the dimension. Initial are reciprocal (up to 3), returning {x,y,z}, x^2+y^2+z^2=1; and other are orthogonal {1}. (or may be should be scaled?)
    //const std::vector<double> & getOrt(unsigned int nDim)const;
    //const std::vector<double> & getOrt(const std::string &tag)const;
    // 
    //double getScale(unsigned int nDim);

protected: 
   /*! class constructor:; Protected as GeometryBasis should not exist alone 
    *  @param nDimensions -- maximal number of the dimensions all datasets will have
    *                        Default is 4 dimensions for crystall e.g. 3 wave-vectors (coordinates in reciprocal space) and energy
    *                        or 2 for powder (angle and energy or n_detector and energy or |Q| and energy)
    */
     MDGeometryBasis(unsigned int nDimensions=4,unsigned int nReciprocalDimensions=3);  
     /// the constructor which builds dimensions from the list of its names; 
     MDGeometryBasis(const std::vector<std::string> &tags,unsigned int nReciprocal_dims=3);  
    /// init the class with new number of dimensions and new dimensions types regardless of previous initialisation (which will be lost); Both constructors
    /// refer to this class for its initiation; Reciprocal dimensions will be names by the names, located in the beginning of the list of tags;
    void reinit_GeometryBasis(const std::vector<std::string> &tags,unsigned int nReciprocal_dims=3);
    
 
  /** return the number of the  dimension which corresponds to the tag provided
     *  @param tag      -- the name(tag) of the dimention 
    *   @param nothrow  -- function throws if the dimension ID falls outside of the range defined for this geometry
    *                      if nothrow==true, function returns negative value instead
    */  
     int getDimNum(const std::string &tag, bool do_throw=false)const;
     /// returns tag of the column nColumn specified; throws out of range if asked too much;
     std::string getColumnName(unsigned int nColumn)const;

    /// get the list of the column numbers for the list of column names
    std::vector<int>  getColumnNumbers(const std::vector<std::string> &tag_list)const;

    ///  the function used by MDGeometry to obtain exisiting dimension ID and to initiate real dimensions in accordance with dimension basis
    std::vector<DimensionID> getDimIDs(void)const;
 
   /// function checks if the tags supplied  coinside with the tags for current basis e.g all existing tags have to be here (the order of tags may be different)
    bool checkTagsCompartibility(const std::vector<std::string> &newTags)const;

     /// logger -> to provide logging, for MD workspaces
    static Kernel::Logger& g_log;
    /// number of total dimensions in dataset;
    unsigned int n_total_dim;
   /// number of reciprocal dimensions (which are non-orthogonal to each other n_rsprcl_dim<=n_total_dim)
   unsigned int n_reciprocal_dimensions;
private:


       // this are the vectors of the primitive cell of the reciprocal lattice
    // expressed in the framework of ???; All other vectors are orthogonal to this triplet
    std::vector<double> lattice_ort[3];
    std::vector<double> unit;

    /// vector of dimensions id-s, specific for current architecture, the size of dimensions is n_total_dimensions,
     std::set<DimensionID> DimensionIDs;
     std::map<unsigned int,std::string> dim_names;

    // build default geometry based on cubic lattice. 
    void buildCubicGeometry(void);
      /// default copy constructor exist
    //MDGeometryBasis(const MDGeometryBasis&);                 
 
    /// it is unclear what is the meaning of =
    MDGeometryBasis& operator=(const MDGeometryBasis&){return *this;}
   
  
    // unique name of workspace as an assembly of a sorted dimension tags plus some ID for nDims and nReciprocalDims
    std::string workspace_name;
    /// checks if nDimensions consistent with n_reciprocal_dimensions; throws if not
    void MDGeometryBasis::check_nDims(unsigned int nDimensions,unsigned int nReciprocalDimensions);
    };
    } // namespace Geometry
}  // namespace MANTID
#endif
