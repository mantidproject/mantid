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

#include "MantidMDAlgorithms/CenterpieceRebinningFunctions.h"

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
      virtual const std::string name() const { return "CenterpieceRebinning";}
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "MD-Algorithms";}

      void set_from_VISIT(const std::string &slicing_description_in_hxml,const std::string &definition);
      void init_property(MDDataObjects::MDWorkspace_sptr inputWSX);
     
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

 

};
}
}

#endif