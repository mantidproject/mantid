#include "MantidMDEvents/MDBoxFlatTree.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDLeanEvent.h"


namespace Mantid
{
  namespace MDEvents
  {

    MDBoxFlatTree::MDBoxFlatTree():
  m_nDim(-1)
  {     
  }

  template<typename MDE,size_t nd>
  void MDBoxFlatTree::initFlatStructure(API::IMDEventWorkspace_sptr pws)
  {
    m_bcXMLDescr = pws->getBoxController()->toXMLString();

    m_nDim = int(pws->getNumDims());
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
    m_Extents.assign(maxBoxes*m_nDim*2, 0);
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
      if(mdBox->isBox())
      {
          size_t nEvents = mdBox->getTotalDataSize();
          mdBox->setFilePosition(eventsStart,nEvents,rememberBoxIsSaved);
          m_BoxEventIndex[i*2]   = eventsStart;
          m_BoxEventIndex[i*2+1] = nEvents;

          eventsStart+=nEvents;
      }
      else
      {
        m_BoxEventIndex[i*2]   = 0;
        m_BoxEventIndex[i*2+1] = 0;
      }
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
    exents_dims[1] = (m_nDim*2);
    std::vector<int> exents_chunk(2,0);
    exents_chunk[0] = int(16384);
    exents_chunk[1] = (m_nDim*2);

    std::vector<int> box_2_dims(2,0);
    box_2_dims[0] = int(maxBoxes);
    box_2_dims[1] = (2);
    std::vector<int> box_2_chunk(2,0);
    box_2_chunk[0] = int(16384);
    box_2_chunk[1] = (2);

    if (update)
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

  void MDBoxFlatTree::loadBoxStructure(::NeXus::File *hFile)
  {
    // ----------------------------------------- Box Structure ------------------------------
    hFile->openGroup("box_structure", "NXdata");

    // Load the box controller description
    hFile->getAttr("box_controller_xml", m_bcXMLDescr);


    // Read all the data blocks
    hFile->readData("box_type", m_BoxType);
    size_t numBoxes = m_BoxType.size();
    if (numBoxes == 0) throw std::runtime_error("Zero boxes found. There must have been an error reading or writing the file.");

    hFile->readData("depth", m_Depth);
    hFile->readData("inverse_volume", m_InverseVolume);
    hFile->readData("extents", m_Extents);

    m_nDim = int(m_Extents.size()/(numBoxes*2));
    hFile->readData("box_children", m_BoxChildren);
    hFile->readData("box_signal_errorsquared", m_BoxSignalErrorsquared);
    hFile->readData("box_event_index", m_BoxEventIndex);



    // Check all vector lengths match
    if (m_Depth.size() != numBoxes) throw std::runtime_error("Incompatible size for data: depth.");
    if (m_InverseVolume.size() != numBoxes) throw std::runtime_error("Incompatible size for data: inverse_volume.");
    //if (boxType.size() != numBoxes) throw std::runtime_error("Incompatible size for data: boxType.");
    //if (m_Extents.size() != numBoxes*m_nDim*2) throw std::runtime_error("Incompatible size for data: extents.");
    if (m_BoxChildren.size() != numBoxes*2) throw std::runtime_error("Incompatible size for data: box_children.");
    if (m_BoxEventIndex.size() != numBoxes*2) throw std::runtime_error("Incompatible size for data: box_event_index.");
    if (m_BoxSignalErrorsquared.size() != numBoxes*2) throw std::runtime_error("Incompatible size for data: box_signal_errorsquared.");

    hFile->closeGroup();

  }

  template<typename MDE,size_t nd>
  uint64_t MDBoxFlatTree::restoreBoxTree(std::vector<MDBoxBase<MDE,nd> *>&Boxes,API::BoxController_sptr bc, bool FileBackEnd,bool BoxStructureOnly)
  {

    size_t numBoxes = this->getNBoxes();
    Boxes.assign(numBoxes, NULL);

    uint64_t totalNumEvents(0);

    for (size_t i=0; i<numBoxes; i++)
    {

      size_t box_type = m_BoxType[i];
      if (box_type == 0)continue;

      MDBoxBase<MDE,nd> * ibox = NULL;
      MDBox<MDE,nd> * box;

      // Extents of the box, as a vector
      std::vector<Mantid::Geometry::MDDimensionExtents<coord_t> > extentsVector(m_nDim);
      for (size_t d=0; d<m_nDim; d++)
        extentsVector[d].setExtents(static_cast<double>(m_Extents[i*m_nDim*2 + d*2]),static_cast<double>(m_Extents[i*m_nDim*2 + d*2 + 1]));

      // retrieve initial and file location and the numner of the events which belong to this box stored on the HDD
      uint64_t indexStart = m_BoxEventIndex[i*2];
      uint64_t numEvents  = m_BoxEventIndex[i*2+1];

      totalNumEvents+=numEvents;
      if (box_type == 1)
      {
        // --- Make a MDBox -----
        if(BoxStructureOnly)
        {
          box = new MDBox<MDE,nd>(bc, m_Depth[i], extentsVector,-1);
          // Only the box structure is being loaded, so ISavable will be undefined (NeverSaved, 0 size data)
          box->setFilePosition(std::numeric_limits<uint64_t>::max(), 0,false); // this should be default state of ISavable
        }
        else // !BoxStructureOnly)
        {

          if(FileBackEnd)
          {
            box = new MDBox<MDE,nd>(bc, m_Depth[i], extentsVector,-1);
            // Set the index in the file in the box data
            box->setFilePosition(indexStart, numEvents,true);
          }
          else
          {
            box = new MDBox<MDE,nd>(bc, m_Depth[i], extentsVector,int64_t(numEvents));
            // Set the index in the file in the box data, and indicate that data were not saved
            box->setFilePosition(indexStart, numEvents,false);
          }           
        } // ifBoxStructureOnly
        ibox = box;
      }
      else if (box_type == 2)
      {
        // --- Make a MDGridBox -----
        ibox = new MDGridBox<MDE,nd>(bc, m_Depth[i], extentsVector);
      }
      else
        continue;

      // Force correct ID
      ibox->setId(i);
      // calculate volume from extents;
      ibox->calcVolume();

      // Set the cached values
      ibox->setSignal(m_BoxSignalErrorsquared[i*2]);
      ibox->setErrorSquared(m_BoxSignalErrorsquared[i*2+1]);

      // Save the box at its index in the vector.
      Boxes[i] = ibox;

    } // end Box loop

    // Go again, giving the children to the parents
    for (size_t i=0; i<numBoxes; i++)
    {
      if (m_BoxType[i] == 2)
      {
        size_t indexStart = m_BoxChildren[i*2];
        size_t indexEnd   = m_BoxChildren[i*2+1] + 1;
        Boxes[i]->setChildren(Boxes, indexStart, indexEnd);
      }
    }

    return totalNumEvents;
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


  template DLLExport uint64_t MDBoxFlatTree::restoreBoxTree<MDLeanEvent<1>, 1>(std::vector<MDBoxBase<MDLeanEvent<1>, 1>* >&Boxes,API::BoxController_sptr bc, bool FileBackEnd,bool BoxStructureOnly);
  template DLLExport uint64_t MDBoxFlatTree::restoreBoxTree<MDLeanEvent<2>, 2>(std::vector<MDBoxBase<MDLeanEvent<2>, 2>* >&Boxes,API::BoxController_sptr bc, bool FileBackEnd,bool BoxStructureOnly);
  template DLLExport uint64_t MDBoxFlatTree::restoreBoxTree<MDLeanEvent<3>, 3>(std::vector<MDBoxBase<MDLeanEvent<3>, 3>* >&Boxes,API::BoxController_sptr bc, bool FileBackEnd,bool BoxStructureOnly);
  template DLLExport uint64_t MDBoxFlatTree::restoreBoxTree<MDLeanEvent<4>, 4>(std::vector<MDBoxBase<MDLeanEvent<4>, 4>* >&Boxes,API::BoxController_sptr bc, bool FileBackEnd,bool BoxStructureOnly);
  template DLLExport uint64_t MDBoxFlatTree::restoreBoxTree<MDLeanEvent<5>, 5>(std::vector<MDBoxBase<MDLeanEvent<5>, 5>* >&Boxes,API::BoxController_sptr bc, bool FileBackEnd,bool BoxStructureOnly);
  template DLLExport uint64_t MDBoxFlatTree::restoreBoxTree<MDLeanEvent<6>, 6>(std::vector<MDBoxBase<MDLeanEvent<6>, 6>* >&Boxes,API::BoxController_sptr bc, bool FileBackEnd,bool BoxStructureOnly);
  template DLLExport uint64_t MDBoxFlatTree::restoreBoxTree<MDLeanEvent<7>, 7>(std::vector<MDBoxBase<MDLeanEvent<7>, 7>* >&Boxes,API::BoxController_sptr bc, bool FileBackEnd,bool BoxStructureOnly);
  template DLLExport uint64_t MDBoxFlatTree::restoreBoxTree<MDLeanEvent<8>, 8>(std::vector<MDBoxBase<MDLeanEvent<8>, 8>* >&Boxes,API::BoxController_sptr bc, bool FileBackEnd,bool BoxStructureOnly);
  template DLLExport uint64_t MDBoxFlatTree::restoreBoxTree<MDLeanEvent<9>, 9>(std::vector<MDBoxBase<MDLeanEvent<9>, 9>* >&Boxes,API::BoxController_sptr bc, bool FileBackEnd,bool BoxStructureOnly);

  template DLLExport uint64_t MDBoxFlatTree::restoreBoxTree<MDEvent<1>, 1>(std::vector<MDBoxBase<MDEvent<1>, 1>* >&Boxes,API::BoxController_sptr bc, bool FileBackEnd,bool BoxStructureOnly);
  template DLLExport uint64_t MDBoxFlatTree::restoreBoxTree<MDEvent<2>, 2>(std::vector<MDBoxBase<MDEvent<2>, 2>* >&Boxes,API::BoxController_sptr bc, bool FileBackEnd,bool BoxStructureOnly);
  template DLLExport uint64_t MDBoxFlatTree::restoreBoxTree<MDEvent<3>, 3>(std::vector<MDBoxBase<MDEvent<3>, 3>* >&Boxes,API::BoxController_sptr bc, bool FileBackEnd,bool BoxStructureOnly);
  template DLLExport uint64_t MDBoxFlatTree::restoreBoxTree<MDEvent<4>, 4>(std::vector<MDBoxBase<MDEvent<4>, 4>* >&Boxes,API::BoxController_sptr bc, bool FileBackEnd,bool BoxStructureOnly);
  template DLLExport uint64_t MDBoxFlatTree::restoreBoxTree<MDEvent<5>, 5>(std::vector<MDBoxBase<MDEvent<5>, 5>* >&Boxes,API::BoxController_sptr bc, bool FileBackEnd,bool BoxStructureOnly);
  template DLLExport uint64_t MDBoxFlatTree::restoreBoxTree<MDEvent<6>, 6>(std::vector<MDBoxBase<MDEvent<6>, 6>* >&Boxes,API::BoxController_sptr bc, bool FileBackEnd,bool BoxStructureOnly);
  template DLLExport uint64_t MDBoxFlatTree::restoreBoxTree<MDEvent<7>, 7>(std::vector<MDBoxBase<MDEvent<7>, 7>* >&Boxes,API::BoxController_sptr bc, bool FileBackEnd,bool BoxStructureOnly);
  template DLLExport uint64_t MDBoxFlatTree::restoreBoxTree<MDEvent<8>, 8>(std::vector<MDBoxBase<MDEvent<8>, 8>* >&Boxes,API::BoxController_sptr bc, bool FileBackEnd,bool BoxStructureOnly);
  template DLLExport uint64_t MDBoxFlatTree::restoreBoxTree<MDEvent<9>, 9>(std::vector<MDBoxBase<MDEvent<9>, 9>* >&Boxes,API::BoxController_sptr bc, bool FileBackEnd,bool BoxStructureOnly);

  }
}
