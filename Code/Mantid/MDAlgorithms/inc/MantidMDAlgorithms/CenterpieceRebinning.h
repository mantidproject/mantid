#ifndef H_CENTERPIECE_REBINNING
#define H_CENTERPIECE_REBINNING

#include "MDDataObjects/MDWorkspace.h"
#include "MDDataObjects/SlicingProperty.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
    namespace MDAlgorithms
{
        using namespace MDDataObjects;
 
class DLLExport CenterpieceRebinning: public API::Algorithm
{
    
public:

    CenterpieceRebinning(void);

    virtual ~CenterpieceRebinning(void);

  /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "CenterpieceRebinning";}
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "MD-Algorithms";}

private:
    // Overridden Algorithm methods
      void init();
      void exec();

    
      /// Create output workspace
     // API::MatrixWorkspace_sptr createOutputWS(API::MatrixWorkspace_sptr input);
       
      /// The progress reporting object
      API::Progress *m_progress;

/*
    MDWorkspace const *const origin;
    SlicingProperty const*const trf;

 // function builds the vector of cell indexes which can contribute into the cut, described by the transformation matrix supplied;
    void preselect_cells(std::vector<long> &selected_cells,long &n_preselected_pix);
    void build_transformation_matrix();

// auxiliary variables extracted and transformed from input data
    unsigned int nDims; ///< number of dimensions in dataset
    std::vector<double> trans_bott_left;
    std::vector<double> cut_min;
    std::vector<double> cut_max;
*/
};
}
}

#endif