#include "MantidMDEvents/MDBoxFlatTree.h"


namespace Mantid
{
namespace MDEvents
{

    MDBoxFlatTree::MDBoxFlatTree(API::BoxController_const_sptr bc)
    {
      size_t maxBoxes = bc->getMaxId();
      int nDim        = int(bc->getNDims());
      this->initTree(maxBoxes,nDim);
    }

    void MDBoxFlatTree::initTree(size_t maxBoxes,int nDim)
    {
     // Box type (0=None, 1=MDBox, 2=MDGridBox
      m_BoxType.assign(maxBoxes, 0);
    // Recursion depth
      m_Depth.assign(maxBoxes, -1);
    // Start/end indices into the list of events
      m_BoxEventIndex.assign(maxBoxes*2, 0);
    // Min/Max extents in each dimension
      m_Extents.assign(maxBoxes*nDim*2, 0);
    // Inverse of the volume of the cell
      m_InverseVolume.assign(maxBoxes, 0);
    // Box cached signal/error squared
      m_BoxSignalErrorsquared.assign(maxBoxes*2, 0);
    // Start/end children IDs
      m_BoxChildren.assign(maxBoxes*2, 0);

    }


}
}