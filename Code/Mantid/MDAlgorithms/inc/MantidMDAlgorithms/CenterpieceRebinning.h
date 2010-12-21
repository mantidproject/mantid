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
      virtual const std::string name() const { return "CenterpieceRebinning";}
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "MD-Algorithms";}
      ///
      void init_property(MDDataObjects::MDWorkspace_sptr inputWSX);
     
private:
    // Overridden Algorithm methods
      void init();
      void exec();

  
 
  
protected:

 

};
}
}

#endif