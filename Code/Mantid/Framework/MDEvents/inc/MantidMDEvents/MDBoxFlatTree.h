#ifndef MDBOX_FLAT_TREE_H_
#define MDBOX_FLAT_TREE_H_

#include "MantidAPI/BoxController.h"

namespace Mantid
{
namespace MDEvents
{

  class MDBoxFlatTree
  {
    /// Box type (0=None, 1=MDBox, 2=MDGridBox
    std::vector<int> m_BoxType;
    /// Recursion depth
    std::vector<int> m_Depth;
    /// Start/end indices into the list of events
    std::vector<uint64_t> m_BoxEventIndex;
    /// Min/Max extents in each dimension
    std::vector<double> m_Extents;
    /// Inverse of the volume of the cell
    std::vector<double> m_InverseVolume;
    /// Box cached signal/error squared
    std::vector<double> m_BoxSignalErrorsquared;
    /// Start/end children IDs
    std::vector<int> m_BoxChildren;
  public:
    MDBoxFlatTree(API::BoxController_const_sptr bc);
  private:
    void initTree(size_t nBoxes,int nDim);
  };
}
}
#endif