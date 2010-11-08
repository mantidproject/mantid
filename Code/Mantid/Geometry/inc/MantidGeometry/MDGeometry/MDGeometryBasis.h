#ifndef MD_GEOMETRY_BASIS_H
#define MD_GEOMETRY_BASIS_H

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include <string>
#include <map>
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidGeometry/MDGeometry/MDWorkspaceConstants.h"

/** The class is the part of the VisualisationWorkspace and describes the basic multidimentional geometry of the object, 
*   e.g. the dimensions of the reciprocal space and other possible dimenions  
*   the reference reciprocal lattice, the size and shape of a primary crystall cell
*   and number of additional ortogonal dimensions, e.g. temperature, pressure etc. 
*
*   Class provides the reference framework for visualisation and analysis operations alowing to compare different workspaces and to modify 
    the visualisation geometry as requested by user.

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

/**  The class DimensionID describes set of the dimensions, which can be present in dataset;
*    it has just name and ID and set these values properly is the MDGeometryBasis responsibility;
*/
   class DimensionID
    {
    public:
        /// returns  the symbol tag of this dimension ID;
        std::string getDimensionTag(void)const{return DimensionTag;}
        /// returns true if this dimension is reciprocal
        bool isReciprocal(void)const{return is_reciprocal;}
        /// all dimensions has to be arranged in the order of incrreased id;
        bool operator <(const DimensionID &other)const{return this->iDimID< other.iDimID;}
        bool operator<=(const DimensionID &other)const{return this->iDimID<=other.iDimID;}
        bool operator> (const DimensionID &other)const{return this->iDimID> other.iDimID;}
        bool operator>=(const DimensionID &other)const{return this->iDimID>=other.iDimID;}
        bool operator==(const DimensionID &other)const{return this->iDimID==other.iDimID;}
        /// copy constructor and operator()= are public
       /// to initate array, which has to be reset as meaningless and not checked against this later
        DimensionID(int iDimID0=-1,const char *name0="",bool if_reciprocal=true):
        iDimID(iDimID0),tagHash(0),is_reciprocal(if_reciprocal),DimensionTag(name0){}; 
        /// function compares the tag of this DimensionID with the input tag and returns the numeric value of the ID if the names coinside or -1 if not. 
        int getDimID(const std::string &aTag)const{return (this->DimensionTag.compare(aTag)==0)?this->iDimID:-1;}
        /// this should be prviate or protected?;
        int getDimID(void)const{return iDimID;}
        /// 
        size_t getDimHash(void)const{return tagHash;}
       /// this should be reserved to friends?
       void setDimensionIDValues(int newNum,const std::string &newTag,bool is_recipocal);
    private:
        /// actual Dimension ID numbe used to identify the dimension; Strictly internal or shared with MDGeometryBasis
       int iDimID; 
       /// hash for the dimension tag used for sorting
       size_t tagHash;
       /// number of reciprocal dimensions (which are non-orthogonal to each other n_rsprcl_dim<=n_total_dim)
       bool is_reciprocal;
        /// default dimension name used to identify a dimension among others;
        std::string DimensionTag;
        //         
    };
    
//****************************************************************************************************************************************
    class DLLExport MDGeometryBasis
    {
    public:
    virtual ~MDGeometryBasis(void);
    /// return the numbers of dimensions in current geometry; 
    unsigned int getNumDims(void)const{return n_total_dim;}
    /// returns the number of reciprocal dimensions
    unsigned int getNumReciprocalDims(void)const{return this->n_reciprocal_dimensions;};

    /// function returns the vector of the names of the dimensions, which are defined in the workspace
    std::vector<std::string> getBasisTags(void)const;

    /// function returns the workspace ID, which defines the workspace with particular kind of dimensions
    std::string getWorkspaceIDname()const{return workspace_name;}
    /// function checks if the tags supplied  coinside with the tags for current basis ?
    bool calculateTagsCompartibility(const std::vector<std::string> &newTags)const;

    /// gettind dimension tag (name)
    std::string getTag(unsigned int nDim)const;
    /// ort of the dimension. Initial are reciprocal (up to 3), returning {x,y,z}, x^2+y^2+z^2=1; and other are orthogonal {1}. (or may be should be scaled?)
    //const std::vector<double> & getOrt(unsigned int nDim)const;
    //const std::vector<double> & getOrt(const std::string &tag)const;
    /// 
    double getScale(unsigned int nDim);
    size_t getDimHash(const std::string &tag)const;
protected: 
   /*! class constructor:; Protected as GeometryBasis should not exist alone 
    *  @param nDimensions -- maximal number of the dimensions all datasets will have
    *                        Default is 4 dimensions for crystall e.g. 3 wave-vectors (coordinates in reciprocal space) and energy
    *                        or 2 for powder (angle and energy or n_detector and energy or |Q| and energy)
    */
     MDGeometryBasis(unsigned int nDimensions=4,unsigned int nReciprocalDimensions=3);  

    // init class with new number of dimensions and new dimensions types regardless of previous initialisation (which will be lost)
    void reinit_GeometryBasis(const std::vector<std::string> &tags,unsigned int nReciprocal_dims=3);
    
 
  /** return the number of the  dimension which corresponds to the tag provided
    *   in a future there will be group of this kind of functions used to construct a transformation matrix from the set of 
    *   dim ID-s 
    *   @param tag      -- the tag of the dimention 
    *   @param nothrow  -- function throws if the dimension ID falls outside of the range defined for this geometry
    *                      if nothrow==true, function returns negative value instead
    */  
     int getDimIDNum(const std::string &tag, bool do_throw=false)const;
     std::vector<DimensionID> & getDimensionIDs(void){return DimensionIDs;}

     /// logger -> to provide logging, for MD workspaces
    static Kernel::Logger& g_log;
    /// number of total dimensions in dataset;
    unsigned int n_total_dim;
   /// number of reciprocal dimensions (which are non-orthogonal to each other n_rsprcl_dim<=n_total_dim)
   unsigned int n_reciprocal_dimensions;
private:
    /// vector of dimensions id-s, specific for current architecture, the size of dimensions is n_total_dimensions,
    std::vector<DimensionID> DimensionIDs;
    //
    int findTag(const std::string &tag, bool do_throw)const;
      /// function returns the id of the dimension No requseted
    //DimensionID getDimensionID(unsigned int nDim)const;

    std::map<size_t ,int>  dim_list;
   

       // this are the vectors of the primitive cell of the reciprocal lattice
    // expressed in the framework of ???; All other vectors are orthogonal to this triplet
    std::vector<double> lattice_ort[3];
    std::vector<double> unit;



    // build default geometry based on cubic lattice. 
    void buildCubicGeometry(void);
      /// copy constructor
    MDGeometryBasis(const MDGeometryBasis&);                 
 
    /// it is unclear what is the meaning of =
    MDGeometryBasis& operator=(const MDGeometryBasis&);      
   

    /// gettind dimension name 
    //std::string getTag(const DimensionID &id)const;
    // unique name of workspace as an assembly of a sorted dimension tags 
    std::string workspace_name;
    };
    } // namespace Geometry
}  // namespace MANTID
#endif
