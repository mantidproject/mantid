#include "MantidMDEvents/MDBoxFlatTree.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDLeanEvent.h"


namespace Mantid
{
namespace MDEvents
{

    MDBoxFlatTree::MDBoxFlatTree():
    nDim(-1)
    {     
    }

    template<typename MDE,size_t nd>
    void MDBoxFlatTree::initFlatStructure(API::IMDEventWorkspace_sptr pws)
    {
      m_bcXMLDescr = pws->getBoxController()->toXMLString();

      nDim = int(pws->getNumDims());
     // flatten the box structure
      pws->getBoxes(m_Boxes, 1000, false);
      size_t maxBoxes = m_Boxes.size();

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

      MDBoxBase<MDE,nd> *Box;
      for(size_t i=0;i<maxBoxes;i++)
      {
       Box = dynamic_cast<MDBoxBase<MDE,nd> *>(m_Boxes[i]);
      // currently ID is the number of the box, but it may change in a future. TODO: uint64_t
       size_t id = Box->getId();
       size_t numChildren = Box->getNumChildren();
       if (numChildren > 0)
       {
          // DEBUG:
          //// Make sure that all children are ordered. TODO: This might not be needed if the IDs are rigorously done
          //size_t lastId = Box->getChild(0)->getId();
          //for (size_t i = 1; i < numChildren; i++)
          //{
          //  if (Box->getChild(i)->getId() != lastId+1)
          //    throw std::runtime_error("Non-sequential child ID encountered!");
          //  lastId = Box->getChild(i)->getId();
          //}

          m_BoxType[id] = 2;
          m_BoxChildren[id*2] = int(Box->getChild(0)->getId());
          m_BoxChildren[id*2+1] = int(Box->getChild(numChildren-1)->getId());

          // no events but index defined -- TODO -- The proper file has to have consequent indexes for all boxes too. 
          m_BoxEventIndex[id*2]   = 0;
          m_BoxEventIndex[id*2+1] = 0;
       }
       else
       {
          m_BoxType[id] = 1;
          m_BoxChildren[id*2]=0;
          m_BoxChildren[id*2+1]=0;

          MDBox<MDE,nd> * mdBox = dynamic_cast<MDBox<MDE,nd> *>(Box);
          if(!mdBox) throw std::runtime_error("found unfamiliar type of box");
          // Store the index

          uint64_t nPoints = mdBox->getNPoints();
          m_BoxEventIndex[id*2]   = mdBox->getFilePosition();
          m_BoxEventIndex[id*2+1] = nPoints;   
       }

     // Various bits of data about the box
        m_Depth[id] = int(Box->getDepth());
        m_BoxSignalErrorsquared[id*2] = double(Box->getSignal());
        m_BoxSignalErrorsquared[id*2+1] = double(Box->getErrorSquared());
        m_InverseVolume[id] = Box->getInverseVolume();
        for (size_t d=0; d<nd; d++)
        {
          size_t newIndex = id*(nd*2) + d*2;
          m_Extents[newIndex]   = Box->getExtents(d).getMin();
          m_Extents[newIndex+1] = Box->getExtents(d).getMax();

        }
      }

    }


   void MDBoxFlatTree::setBoxesFilePositions(bool makeFileBacked)
   {
      // this will preserve file-backed workspace and information in it as we are not loading old box data and not?
      // this would be right for binary axcess but questionable for Nexus --TODO: needs testing
      Kernel::ISaveable::sortObjByFilePos(m_Boxes);
      // calculate the box positions in the resulting file and save it on place
       uint64_t eventsStart=0;
       bool rememberBoxIsSaved = makeFileBacked;
       for(size_t i=0;i<m_Boxes.size();i++)
       {
          Kernel::ISaveable * mdBox = m_Boxes[i];        
          if(!mdBox->isBox())continue;
          size_t nEvents = mdBox->getTotalDataSize();
          mdBox->setFilePosition(eventsStart,nEvents,rememberBoxIsSaved);
          eventsStart+=nEvents;
       }

   }

    void MDBoxFlatTree::saveBoxStructure(::NeXus::File *hFile)
    {
      size_t maxBoxes = this->getNBoxes();
      if(maxBoxes==0)return;

      bool update=true;
    // Start the box data group
      try
      {
          hFile->openGroup("box_structure", "NXdata");
      }catch(...)
      {
        update = false;
        hFile->makeGroup("box_structure", "NXdata",true);
      }
      hFile->putAttr("version", "1.0");
      // Add box controller info to this group
      hFile->putAttr("box_controller_xml", m_bcXMLDescr);

      std::vector<int> exents_dims(2,0);
      exents_dims[0] = (int(maxBoxes));
      exents_dims[1] = (nDim*2);
      std::vector<int> exents_chunk(2,0);
      exents_chunk[0] = int(16384);
      exents_chunk[1] = (nDim*2);

      std::vector<int> box_2_dims(2,0);
      box_2_dims[0] = int(maxBoxes);
      box_2_dims[1] = (2);
      std::vector<int> box_2_chunk(2,0);
      box_2_chunk[0] = int(16384);
      box_2_chunk[1] = (2);

      if (!update)
      {
      // Update the extendible data sets
        hFile->writeUpdatedData("box_type", m_BoxType);
        hFile->writeUpdatedData("depth", m_Depth);
        hFile->writeUpdatedData("inverse_volume", m_InverseVolume);
        hFile->writeUpdatedData("extents", m_Extents, exents_dims);
        hFile->writeUpdatedData("box_children", m_BoxChildren, box_2_dims);
        hFile->writeUpdatedData("box_signal_errorsquared", m_BoxSignalErrorsquared, box_2_dims);
        hFile->writeUpdatedData("box_event_index", m_BoxEventIndex, box_2_dims);

      }
      else
      {
      // Write it for the first time
        hFile->writeExtendibleData("box_type", m_BoxType);
        hFile->writeExtendibleData("depth", m_Depth);
        hFile->writeExtendibleData("inverse_volume", m_InverseVolume);
        hFile->writeExtendibleData("extents", m_Extents, exents_dims, exents_chunk);
        hFile->writeExtendibleData("box_children", m_BoxChildren, box_2_dims, box_2_chunk);
        hFile->writeExtendibleData("box_signal_errorsquared", m_BoxSignalErrorsquared, box_2_dims, box_2_chunk);
        hFile->writeExtendibleData("box_event_index", m_BoxEventIndex, box_2_dims, box_2_chunk);
      }

      // Finished - close the file. This ensures everything gets written out even when updating.
      hFile->close();

    }
    
// TODO: Get rid of this!
    template DLLExport void MDBoxFlatTree::initFlatStructure<MDEvent<1>, 1>(API::IMDEventWorkspace_sptr pws);
    template DLLExport void MDBoxFlatTree::initFlatStructure<MDEvent<2>, 2>(API::IMDEventWorkspace_sptr pws);
    template DLLExport void MDBoxFlatTree::initFlatStructure<MDEvent<3>, 3>(API::IMDEventWorkspace_sptr pws);
    template DLLExport void MDBoxFlatTree::initFlatStructure<MDEvent<4>, 4>(API::IMDEventWorkspace_sptr pws);
    template DLLExport void MDBoxFlatTree::initFlatStructure<MDEvent<5>, 5>(API::IMDEventWorkspace_sptr pws);
    template DLLExport void MDBoxFlatTree::initFlatStructure<MDEvent<6>, 6>(API::IMDEventWorkspace_sptr pws);
    template DLLExport void MDBoxFlatTree::initFlatStructure<MDEvent<7>, 7>(API::IMDEventWorkspace_sptr pws);
    template DLLExport void MDBoxFlatTree::initFlatStructure<MDEvent<8>, 8>(API::IMDEventWorkspace_sptr pws);
    template DLLExport void MDBoxFlatTree::initFlatStructure<MDEvent<9>, 9>(API::IMDEventWorkspace_sptr pws);

    template DLLExport void MDBoxFlatTree::initFlatStructure<MDLeanEvent<1>, 1>(API::IMDEventWorkspace_sptr pws);
    template DLLExport void MDBoxFlatTree::initFlatStructure<MDLeanEvent<2>, 2>(API::IMDEventWorkspace_sptr pws);
    template DLLExport void MDBoxFlatTree::initFlatStructure<MDLeanEvent<3>, 3>(API::IMDEventWorkspace_sptr pws);
    template DLLExport void MDBoxFlatTree::initFlatStructure<MDLeanEvent<4>, 4>(API::IMDEventWorkspace_sptr pws);
    template DLLExport void MDBoxFlatTree::initFlatStructure<MDLeanEvent<5>, 5>(API::IMDEventWorkspace_sptr pws);
    template DLLExport void MDBoxFlatTree::initFlatStructure<MDLeanEvent<6>, 6>(API::IMDEventWorkspace_sptr pws);
    template DLLExport void MDBoxFlatTree::initFlatStructure<MDLeanEvent<7>, 7>(API::IMDEventWorkspace_sptr pws);
    template DLLExport void MDBoxFlatTree::initFlatStructure<MDLeanEvent<8>, 8>(API::IMDEventWorkspace_sptr pws);
    template DLLExport void MDBoxFlatTree::initFlatStructure<MDLeanEvent<9>, 9>(API::IMDEventWorkspace_sptr pws);

}
}
