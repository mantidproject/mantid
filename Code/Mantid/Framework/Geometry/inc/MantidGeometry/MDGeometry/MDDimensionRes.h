#ifndef H_DIMENSIONRES
#define H_DIMENSIONRES
#include "MantidGeometry/MDGeometry/MDDimension.h"
namespace Mantid{
    namespace Geometry{
/** This is the dimension which corresponds to reciprocal dimension
*
* It differs from usual dimension because can have some  nonortogonal direction in the WorkspaceGeometry system of coordinates
*
    @author Alex Buts, RAL ISIS
    @date 28/09/2010

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
 /** enum describes reciprocal dimension numbers and reflects the request to up to three reciprocal dimensions */
 enum rec_dim{
        q1,
        q2,
        q3
 }; 

class DLLExport MDDimensionRes :   public MDDimension
{
   // this is to initiate and set the Dimensions from the Geometry
    friend class MDGeometry;
public:
    virtual ~MDDimensionRes(void);

  ///virtual;  it is reciprocal dimension -> convenient to use instead of dynamic cast 
    bool isReciprocal(void)const{return true;}


    /// indicates which reciprocal primitive vector this reciprocal dimension is associated with, either q1, q2 or q3
    rec_dim getReciprocalVectorType() const 
    {
      return this->nRecDim;
    }

    ///virtual Implementation of toXMLString providing additional fields over parent MDDimension relating to reciprocal nature.
     std::string toXMLString() const;
    /// virtual 
	V3D getDirectionCryst(void)const;
	/**Main constructor for a reciprocal dimension Initial direction should be set as in MDGeometryBasis, correspondent direction */
    MDDimensionRes(const std::string &ID,const rec_dim nDim, const V3D *pDir=NULL);
	MDDimensionRes(const MDBasisDimension &Dim);
protected:
    // function sets the coordinates of the dimension;
    virtual void setDirection(const V3D &theDirection);
private:
    /// helper method for converting between qtypes as enums to strings.
    std::string getQTypeAsString() const;

    /// number of this reciprocal dimension (one of 3)
    rec_dim nRecDim; 
};

/**  The class is used in algorithms when number of reciprocal dimensions is less then 3 to provide meaningfull representation of missing reciprocal dimensions
*/
class DLLExport MDDimDummy : public MDDimensionRes
{
public:
  MDDimDummy(unsigned int nRecDim);
 ~MDDimDummy(){};
// these functions are implemented through the parent
// function returns the name of the axis in this direction
//    virtual std::string const &getName()const{return "DUMMY AXIS";}
/// function return the unique dimension ID, identifying the current dimension among others 
//    virtual std::string getDimensionTag(void)const{return "DUMMY REC_DIM";}
// get Axis data; 
//    std::vector<double> const &  getAxis(void)const{std::vector<double> tmp(2,0); tmp[1]=1; return tmp;}

/// virtual get maximal value along the dimension
    double       getMaximum(void)const{return 1;}
/// virtual get minimal value along the dimension
     double       getMinimum(void)const{return 0;}
/// virtual range of data along this axis
    double       getRange(void)const{return (1);}
/// virtual  scale of the data along this axis 
    /// TO DO: what is this scale and what constraint we want to apply on it? 
    double getScale(void)const{return 1;}
/** return the state of this dimension i.e if it is integrated. If it is, it has one bin only, the axis consis of two points, 
 *   coinsiding with min and max values rMin and rMax; */
    bool        getIntegrated(void)const{return true;}

    /** virtual  the function returns the center points of the axis bins; There are nBins of such points 
     *(when axis has nBins+1 points with point 0 equal rMin and nBins+1 equal rMax) */
    void getAxisPoints(std::vector<double>  &p)const{std::vector<double> tmp(1,0.5);p=tmp;}
    ///virtual  function returns number of the bins this dimension has
    size_t getNBins(void)const{return 1;}
    ///virtual  function returns the dimension stride, e.g. the step of change for 1D index of a dimension in multidimensional array, if an index of this dimension changes by one
    size_t       getStride(void)const{return 0;}
  
    //virtual Get coordinate for index; Throws  if ind is out of range 
    double getX(size_t ind)const;
    ///virtual  it is not reciprocal dimension -> convenience function
    bool isReciprocal(void)const{return true;}
    // no mutator allowed
private:

};
} // MDDataObjects
} // Mantid
#endif
