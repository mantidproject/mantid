#ifndef H_CENTERPIECE_REBINNING
#define H_CENTERPIECE_REBINNING


#include "MantidAPI/MDPropertyGeometry.h"
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MDDataObjects/MDWorkspace.h"
#include "MantidAPI/ImplicitFunction.h"

namespace Mantid
{
    namespace MDAlgorithms
{

class DLLExport CenterpieceRebinning: public API::Algorithm
{
    
public:

    CenterpieceRebinning(void);

    virtual ~CenterpieceRebinning(void);

  /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "CenterpieceRebinningPG";}
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "MD-Algorithms";}

      void set_from_VISIT(const std::string &slicing_description_in_hxml,const std::string &definition);
      void init_source(MDDataObjects::MDWorkspace_sptr inputWSX);
     
private:
    // Overridden Algorithm methods
      void init();
      void exec();

      /// The progress reporting object
      API::Progress *m_progress;

      /// the parameters which describe how to treat nan and inf data in the dataset (used for masking etc)
      bool ignore_nan,ignore_inf;

  // function builds the vector of cell indexes which can contribute into the cut, described by the transformation matrix supplied;
   // void preselect_cells(std::vector<long> &selected_cells,long &n_preselected_pix);

  // the function protected for testing purposes; they are actually private to this algorithm.
protected:
/*! function returns the list of the cell numbers which can contribute into the cut described by transformation matrix
 *  input arguments:
 @param source           -- the geometry of the initial workspace
 @param target           -- the description class, which describes final geometry and the cut.
 @param cells_to_select  -- the list of the cell indexes, which can contribute into the cut
 @param n_preselected_pix-- number of pixels contributed into the cells. 
*/
    void preselect_cells(const MDDataObjects::MDImageData &Source, const Geometry::MDGeometryDescription &target, std::vector<size_t> &cells_to_select,size_t &n_preselected_pix);
/// internal helper class to identify the indexes on an auxilary 3D lattice;
    class nCell3D
    {
     size_t NX,NY;
     public:
        nCell3D(size_t nx,size_t ny):NX(nx),NY(ny){};
        size_t nCell(long i,long j, long k)const{return i+NX*(j+k*NY);}  
    };
// 

    /// build transformation matrix from the slicing data;
    MDDataObjects::transf_matrix build_scaled_transformation_matrix(const Geometry::MDGeometry &Source,const Geometry::MDGeometryDescription &target);
    /** rebinning 4D dataset
    *   @param rescaled_transf    the data describing the final geometry of the cut
    *   @param *source_pix        the pointer to the start of the buffer with source points for rebinning
    *   @param  nPix              number of points in the buffer
    *   @param  *data             the pointer to the structure with Image data;
    */
    size_t rebin_dataset4D(const MDDataObjects::transf_matrix &rescaled_transf, const std::vector<size_t> &strides,const MDDataObjects::sqw_pixel *source_pix, size_t nPix, MDDataObjects::MD_image_point *data,
                           double boxMin[],double boxMax[]);
    // finalsizes rebinoing operations; e.g. calculates averages and calculates location of pixels (filesystem)
    size_t finalise_rebinning(MDDataObjects::MD_image_point *data,size_t data_size);
};
}
}

#endif