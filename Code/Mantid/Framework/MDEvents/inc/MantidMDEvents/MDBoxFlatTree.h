#ifndef MDBOX_FLAT_TREE_H_
#define MDBOX_FLAT_TREE_H_

#include "MantidAPI/BoxController.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDGridBox.h"
#include "MantidMDEvents/MDEventWorkspace.h"

namespace Mantid
{
namespace MDEvents
{

  class DLLExport MDBoxFlatTree
  {
  public:
    MDBoxFlatTree();

    size_t getNBoxes()const{return m_BoxType.size();}

    const std::string &getBCXMLdescr()const {return m_bcXMLDescr;}

    std::vector<Kernel::ISaveable *> &getBoxes(){return m_Boxes;}

    // TODO: this does not have to be a template-> refactoring needed.
    template<typename MDE,size_t nd>
    void initFlatStructure(API::IMDEventWorkspace_sptr pws);
    /**Method resotores the interconnected box structure in memory, e.g. internal nodes connections */
    template<typename MDE,size_t nd>
    uint64_t restoreBoxTree(std::vector<MDBoxBase<MDE,nd> *>&Boxes ,API::BoxController_sptr bc, bool FileBackEnd,bool BoxStructureOnly);

    /*** this function tries to set file positions of the boxes to 
          make data slatially located close to each otger to be as close as possible on the HDD */
    void setBoxesFilePositions(bool makeFileBacked);
    /**Save flat box structure into a nexus file*/
    void saveBoxStructure(::NeXus::File *hFile);
    /**Save flat box structure from a nexus file*/
    void loadBoxStructure(::NeXus::File *hFile);

  private:
    int m_nDim;
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

    std::vector<Kernel::ISaveable *> m_Boxes;

    std::string m_bcXMLDescr;

  };
}
}
#endif