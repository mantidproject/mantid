#ifndef H_CENTERPIECE_REBINNING
#define H_CENTERPIECE_REBINNING
#include "SQW.h"
#include "SlicingData.h"

class CenterpieceRebinning
{
    
public:
    CenterpieceRebinning(const SQW &origin,const SlicingData &trf);
    void run_rebinning(SQW &target);
    virtual ~CenterpieceRebinning(void);
private:
    SQW const *const origin;
    SlicingData const*const trf;

 // function builds the vector of cell indexes which can contribute into the cut, described by the transformation matrix supplied;
    void preselect_cells(std::vector<long> &selected_cells,long &n_preselected_pix);
    void build_transformation_matrix();

// auxiliary variables extracted and transformed from input data
    unsigned int nDims; ///< number of dimensions in dataset
    std::vector<double> trans_bott_left;
    std::vector<double> cut_min;
    std::vector<double> cut_max;

};
#endif