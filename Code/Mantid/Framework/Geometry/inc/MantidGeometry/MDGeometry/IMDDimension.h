#ifndef I_MD_DIMENSION_H
#define I_MD_DIMENSION_H
#include <vector>

/** The class discribes one dimension of multidimensional dataset representing an ortogonal dimension and linear axis. 
*
*   Abstract type for a multi dimensional dimension. Gives a read-only layer to the concrete implementation.

    @author Owen Arnold, RAL ISIS
    @date 12/11/2010

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
#include "MantidKernel/System.h"
#include "MantidGeometry/V3D.h"

namespace Mantid
{
  namespace Geometry
  {

  class DLLExport IMDDimension
  {
  public:
    /// the name of the dimennlsion as can be displayed along the axis
    virtual std::string getName() const = 0;
    /// short name which identify the dimension among other dimensin. A dimension can be usually find by its ID and various  
    /// various method exist to manipulate set of dimensions by their names. 
    virtual std::string getDimensionId() const = 0;

    /// if the dimension is integrated (e.g. have single bin)
    virtual bool getIsIntegrated() const = 0;
    // it is sometimes convinient to shift image data by some number along specific dimension
    virtual double getDataShift()const = 0;

    virtual double getMaximum() const = 0;

    virtual double getMinimum() const = 0;
    /// number of bins dimension have (an integrated has one). A axis directed along dimension would have getNBins+1 axis points.
    virtual unsigned int getNBins() const = 0;
    /// the change of the location in the multidimensional image array, which occurs if the index of this dimension changes by one.
    virtual size_t      getStride()const = 0;
    /// defines if the dimension is reciprocal or not. The reciprocal dimensions are treated differently from an orthogonal one
    virtual bool isReciprocal() const = 0;
    /// defines the dimension scale in physical units. ->TODO: it is unclear if we need this here
    // virtual double getScale()const = 0;
    ///  Get coordinate for index;
    virtual double getX(unsigned int ind)const = 0;
    /** function returns a direction of the dimension in the system of coordinates described by the MDBasis;
     *  Orthogonal dimensions always have direction 1, but we set direction of this to 0  (e.g. direction={0,0,0})? questionable, 1 may be better;
     *  while reciprocal dimension can be directed anywhere withing the reciprocal space;
	     Norm of the vector, returned by this function has to be 1    */
    virtual V3D getDirection(void)const = 0;
    /** Return direction in the crystallogrpahical sence, e.g. output V3D is normalized in such a way that the size of
	   smallest (by module) non-0 component of the vector is 1; In this case, all vectors representing valid crystallographical axis would 
	   have integer values; */
    virtual V3D getDirectionCryst(void)const = 0;

    /** the function returns the center points of the axis bins; There are nBins of such points 
     (when axis has nBins+1 points with point 0 equal rMin and nBins+1 equal rMax) */
    virtual void getAxisPoints(std::vector<double>  &)const=0;


    //Dimensions must be xml serializable.
    virtual std::string toXMLString() const = 0;

    virtual ~IMDDimension(){};
  };

  }
}
#endif
