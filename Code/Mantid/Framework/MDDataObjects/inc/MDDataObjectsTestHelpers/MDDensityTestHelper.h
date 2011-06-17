#ifndef MDDENSITY_TEST_HELPER_H
#define MDDENSITY_TEST_HELPER_H

#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"

/** helper class to calculate number of signals in a different size cell given the function of signal density (image in the functional form)
 *    The test data generated and connected in a way, similar for the way, one can get running rebinnin, so the cell data obtained from 
 *     test dataset are equivalent to data, obtained from rebinning
 *
 *   Created to help testing the rebinning in any dimensions; 
 *   rotations are ignored for the time being -- relatively easy to implement
 *   axis swap is disabled as Owen seems uses different approach to achieve it
 *   integration over an axis is enabled, poroviding (nDim - N_integrated_axis) -- dimensions image mapping nDim dataset

    @author Alex Buts, RAL ISIS
    @date 07/06/2011

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
namespace MDDataTestHelper{
    /// MDDatapoint type
    typedef float MDDPoint_t;
class DLLExport MDDensityHomogeneous
{
public:
    MDDensityHomogeneous(const Geometry::MDGeometryDescription &geomDescr);
    /// returns the pixels data, contributed into the cell;
    virtual void getMDDPointData(size_t cell_index,char *pBuffer,size_t BufSize,size_t &nDataPoints)const;
    /// returs the average signal, error and number of pixels, contribured into this the cell, identified by index cell_index;
    virtual void getMDImageCellData(size_t cell_index,double &Signal,double &Error,uint64_t &nPixels)const;

    /// size of MDDataPoint in bytes
    virtual size_t sizeofMDDataPoint()const{return size_t(MDDPixel_size);}
    /// returns and calculates if not calculated before the number of fine pixels contributing into coarce cell
    uint64_t coarseCellCapacity(size_t cell_ind)const;
    uint64_t coarseCellCapacity(const std::vector<size_t> &indexes)const;
    /// returns number of pixels, contributed into dataset;
    uint64_t getNContribPixels()const{return nContributedPixels;}
    ///
    virtual ~MDDensityHomogeneous(){}
protected:
    // number of pixels contributed into image; 
    uint64_t     nContributedPixels;
    /// number of the dimension for the dataset and full (expanded, non-integrated) dimensions of MDImage
    unsigned int nDims,nFullDims,nIndexes;
    //
    int MDDPixel_size;

// protected as can be used by the functions, overloading homogeneous density; 
    /// min values in every direction
    std::vector<double>   r_min;
    /// max values in every direction
    std::vector<double>   r_max;

    /// the bins which define microgrid
    uint64_t              fine_grid_size;
    std::vector<uint64_t> fine_nbin;
    std::vector<uint64_t> fine_bin_stride;
    std::vector<double>   fine_bin_size;

  /// the bins, which define coarce grid
    size_t              coarse_grid_size;
    std::vector<size_t> coarse_nbin;
    std::vector<size_t> coarse_bin_stride;
    std::vector<double>   coarse_bin_size;
  
protected: // for testing, actually private;
    /// function returns the MD-coordinates of the points which contributed into the cell defined by index ind; Function returns number of points
    /// when the coordinates themself returned in coord vector as nDim blocks of values;
    uint64_t getCellPixCoordinates(size_t ind, std::vector<MDDPoint_t> &coord)const;
//
     std::vector<size_t>const & getCoarseStride()const{return coarse_bin_stride;}
     std::vector<uint64_t>const & getFineStride()const{return fine_bin_stride;}

    void get_contributed_pixels(size_t macro_cell_ind,std::vector<uint64_t> &ipix)const;
    void get_rCoarseCell(size_t ind, std::vector<float> &r_cell)const;
    //
    void findFineIndexes(uint64_t   ind,std::vector<uint64_t> &fine_ind)const;
    void findCoarseIndexes(size_t ind,std::vector<size_t> &coarse_ind)const;
   /// add one to multidimensional index untill this index is less then ind_max; if ind and max_ind are equal, returns false
    /// to work correctly, needs initial index to be equal ind_min; should generate P(DeltaInd) indexes imitating size(ind)
    /// nested loops;
    bool ind_plus(const std::vector<uint64_t> &ind_min,const std::vector<uint64_t> &ind_max, std::vector<uint64_t> &ind)const;
 
  
};

/** The class describes multidimensional data points and 
*/
class DLLExport MDPeakData: public MDDensityHomogeneous
{
    unsigned int nRecDim;
    double SigmaSq;

public:
    MDPeakData(double sigmaSq, const Geometry::MDGeometryDescription &geomDescr);
  //  void getMDDPointData(size_t cell_index,char *pBuffer,size_t &nDataPoints)const;
 //   void getMDImageCellData(size_t index,double &Signal,double &Error,uint64_t &nPixels)const;
private:

   //double signal(const std::vector<float> &r)const;
    /// simple approximation of the error function (from wikipedia)
    double erf_apr(double x)const{
#define   a_par 0.147
        double x2=x*x;
        return (int((x)>0)-int((x)<0))*sqrt(1-exp(-x2*(4./M_PI+a_par*x2)/(1+a_par*x2)));
    }
};

} // endMDDataObjects namespace
} // end Mantid namespace

#endif