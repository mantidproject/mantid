#ifndef MDDIMENSION_H
#define MDDIMENSION_H
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
//#include <algorithm>
#include <limits>
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidGeometry/MDGeometry/MDWorkspaceConstants.h"
#include "MantidGeometry/MDGeometry/MDGeometryBasis.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
/*! The class discribes one dimension of multidimensional dataset representing an ortogonal dimension and linear axis. 
*
*   A multidimensional dataset has N such dimensions and usual problem would have maximal number of 
*   dimensions N_max with N<=N_max

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

namespace Mantid{
    namespace Geometry{

class DLLExport MDDimension : public IMDDimension
{
public:

  virtual std::string getDimensionId() const;

  virtual bool getIsIntegrated() const;

     virtual ~MDDimension();
/// function returns the name of the axis in this direction
    std::string getName()const{return AxisName;}
/// function return the unique dimension ID, identifying the current dimension among others 
    std::string getDimensionTag(void)const{return dimTag;}
/// get maximal value along the dimension
    double       getMaximum(void)const{return rMax;}
/// get minimal value along the dimension
    double       getMinimum(void)const{return rMin;}
/// range of data along this axis
    double       getRange(void)const{return (rMax-rMin);}
/// scale of the data along this axis 
    /// TO DO: what is this scale and what constraint we want to apply on it? 
    double getScale(void)const{return latticeParam;}
/*! return the state of this dimension i.e if it is integrated. If it is, it has one bin only, the axis consis of two points, 
 *   coinsiding with min and max values rMin and rMax; */
    bool        getIntegrated(void)const{return isIntegrated;}
/// coordinate along this direction; It is rather interface as the coordinate of usual dimension along orthogonal axis is always 1
    virtual std::vector<double>const & getCoord(void)const{return coord;}
    /// get Axis data; 
    std::vector<double> const &  getAxis(void)const{return Axis;}
    /// the function returns the center points of the axis bins; There are nBins of such points 
    /// (when axis has nBins+1 points with point 0 equal rMin and nBins+1 equal rMax)
    void getAxisPoints(std::vector<double>  &)const;
    /// function returns number of the bins this dimension has
    unsigned int getNBins(void)const{return nBins;}
    /// function returns the dimension stride, e.g. the step of change for 1D index of a dimension in multidimensional array, if an index of this dimension changes by one
    size_t       getStride(void)const{return nStride;}
  
    //Get coordinate for index; Throws through vector if ind out of range 
    double getX(unsigned int ind){return Axis.at(ind);}
    /// it is not reciprocal dimension -> convenience function
    virtual bool isReciprocal(void)const{return false;}
    
    MDDimension(const std::string &ID);
protected:
    /// this is to initiate and set the Dimensions from the Geometry; The geometry is in fact a collection of Dimensions + a bit more
    friend class MDGeometry;
 
//********  SET. -> geometry should set it properly;
    // function sets the coordinates of the dimension; An orthogonal dimension does nothing with it
    virtual void setCoord(const std::vector<double> &){};
    // function sets the dimension as a linear dimension with specific ranges and number of bins
    virtual void  setRange(double rMin=-1,double rMax=1,unsigned int nBins=1);
    void  setName(const char *name) {this->AxisName.assign(name);}
    void  setName(const std::string & name){this->AxisName.assign(name); }
      /*! Set the scale of a particular dimension
     * @param Value -- the value to set;    */
    void   setScale(double Value){latticeParam=Value;}
    /// functions clears axis, makes nBins=1 and sets "integrated" sign to the dimension. Meaningless and dangerous without real integration procedure
    void   setIntegrated(void);
    /// as setIntegrated(void) but integration within the range specified
    void   setIntegrated(double rxMin);
    void   setIntegrated(double rxMin, double rxMax);
    /// set the dimension expanded (e.g. real with proper number of bins, non-integrated)
    /// also changes the number of bins if the dimension was not expanded before. 
    /// if nBins == 1 works like setIntegrated;
    void   setExpanded(unsigned int nBins);
    /// differs from setRange by the fact that the limits has to be within the existing ranges
    void   setExpanded(double rxMin, double rxMax,unsigned int nBins);

  /// should not be public to everybody as chanded by  MDGeometry while reshaping or rebinning;
    void  setStride(size_t newStride){nStride=newStride; }
/// the coordinate of a dimension in an WorkspaceGeometry system of coordinates (always 1 here and triplet for reciprocals) -- need further exploration -> which coordinate systen it is. 
    std::vector<double> coord;
 /// logger -> to provide logging, for MD workspaces in this dimension
    static Kernel::Logger& g_log;
private:
     /// name of the axis;
    std::string AxisName;
    /// the name of the dimension in the dimensions basis;
    std::string dimTag;


    /// The parameter, which specify if the axis is integraged. If it is, nBins =1;
    bool isIntegrated;
    /// number of bins the axis have
    unsigned int nBins;
    /// dimension stride in a multidimensional array
    size_t   nStride;
    /// vector of leftmost bin ranges plus rightmost value;  
    std::vector<double> Axis;
     /// min and maximum values along this dimension;
    double rMin,rMax;
    /// lattice sacale in this direction
    double latticeParam;

   // *************  should be prohibited?:
    MDDimension(const MDDimension &){};
    MDDimension & operator=(const MDDimension &rhs);
    /// internal function which verify if the ranges of the argumens are permitted; Used by many setRanges functions
    void check_ranges(double rxMin,double rxMax);

 
};
} // MDDataObjects
} // Mantid

#endif
