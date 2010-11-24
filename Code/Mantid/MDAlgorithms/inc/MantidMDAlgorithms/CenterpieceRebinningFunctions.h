#include "MDDataObjects/MDWorkspace.h"
namespace Mantid
{
    namespace MDAlgorithms
    {
/** 
* Rebinning matrix Internal class which describes rebinning transformation in the terms of current MDImageData
*/
class transf_matrix
{
public:
        int nDimensions;                     // real number of dimensions in a dataset???
        double rotations[9];                 // rotation matrix for qx,qy,qz coordinates; 
        bool ignore_NaN,ignore_Inf;
        std::vector<double> trans_bott_left; // shift in all directions (tans_elo is 4th element of transf_bott_left
        std::vector<double> cut_min;         // min limits to extract data;
        std::vector<double> cut_max;         // max limits to extract data;
        std::vector<double> axis_step;       // (cut_max-cut_min)/(nBins);
      
};



/// helper class to identify the indexes on an auxilary 3D lattice; Needed for Preselect_Cells
    class nCell3D
    {
     size_t NX,NY;
     public:
        nCell3D(size_t nx,size_t ny):NX(nx),NY(ny){};
        size_t nCell(long i,long j, long k)const{return i+NX*(j+k*NY);}  
    };
// 
/*! function returns the list of the cell numbers which can contribute into the cut described by transformation matrix
 *  input arguments:
 @param source           -- the geometry of the initial workspace
 @param target           -- the description class, which describes final geometry and the cut.
 @param cells_to_select  -- the list of the cell indexes, which can contribute into the cut
 @param n_preselected_pix-- number of pixels contributed into the cells. 
*/
    void preselect_cells(const MDDataObjects::MDWorkspace &Source, const Geometry::MDGeometryDescription &target, std::vector<size_t> &cells_to_select,size_t &n_preselected_pix);

  /// build transformation matrix from the slicing data;
    transf_matrix build_scaled_transformation_matrix(const Geometry::MDGeometry &Source,const Geometry::MDGeometryDescription &target,bool ignoreNaN,bool ignoreInf);

   // finalsizes rebinoing operations; e.g. calculates averages and calculates location of pixels (filesystem)
    size_t finalise_rebinning(MDDataObjects::MD_image_point *data,size_t data_size);


   size_t rebin_Nx3dataset(const transf_matrix &rescaled_transf, const char *source_pix_buf, size_t nPix, MDDataObjects::MDWorkspace &TargetWorkspace);
} //namespaceAlgorithms
}