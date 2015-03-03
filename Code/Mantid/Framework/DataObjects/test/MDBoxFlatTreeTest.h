#ifndef MANTID_DATAOBJECTS_MDBOX_FLATTREE_H_
#define MANTID_DATAOBJECTS_MDBOX_FLATTREE_H_

#include "MantidAPI/FrameworkManager.h"
#include "MantidDataObjects/MDBoxFlatTree.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidDataObjects/MDLeanEvent.h"
#include "MantidAPI/BoxController.h"

#include <cxxtest/TestSuite.h>
#include <Poco/File.h>

using namespace Mantid;
using namespace Mantid::DataObjects;

class MDBoxFlatTreeTest :public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDBoxFlatTreeTest  *createSuite() { return new MDBoxFlatTreeTest (); }
  static void destroySuite( MDBoxFlatTreeTest  *suite ) { delete suite; }

  MDBoxFlatTreeTest()
  {
    // load dependent DLL, which are used in MDEventsTestHelper (e.g. MDAlgorithms to create MD workspace)
    // Mantid::API::FrameworkManager::Instance();
    // make non-file backet mdEv workspace with 10000 events
     spEw3 = MDEventsTestHelper::makeFileBackedMDEW("TestLeanEvWS", false,10000);
  }

  void testFlatTreeOperations()
  {
    MDBoxFlatTree BoxTree;

    TS_ASSERT_EQUALS(0,BoxTree.getNBoxes());

    TS_ASSERT_THROWS_NOTHING((BoxTree.initFlatStructure(spEw3,"aFile")));

    TSM_ASSERT_EQUALS("Workspace creatrion helper should generate ws split into 1001 boxes",1001,BoxTree.getNBoxes());

    TS_ASSERT_THROWS_NOTHING(BoxTree.saveBoxStructure("someFile.nxs"));

    Poco::File testFile("someFile.nxs");
    TSM_ASSERT("BoxTree was not able to create test file",testFile.exists());


    MDBoxFlatTree BoxStoredTree;
    int nDims = 3;
    TSM_ASSERT_THROWS("Should throw as the box data were written for lean event and now we try to retrieve full events",
      BoxStoredTree.loadBoxStructure("someFile.nxs",nDims,"MDEvent"),std::runtime_error);

    nDims = 0;
    TSM_ASSERT_THROWS_NOTHING("Should path now and return nDims",BoxStoredTree.loadBoxStructure("someFile.nxs",nDims,"MDLeanEvent"));

    TSM_ASSERT_EQUALS("Should be nDims = 3",3,nDims);
    TS_ASSERT_THROWS_NOTHING(BoxStoredTree.loadBoxStructure("someFile.nxs",nDims,"MDLeanEvent"));

    size_t nDim = size_t(BoxStoredTree.getNDims());
    API::BoxController_sptr new_bc = boost::shared_ptr<API::BoxController>(new API::BoxController(nDim));    
    new_bc->fromXMLString(BoxStoredTree.getBCXMLdescr());

    TSM_ASSERT("Should restore the box controller equal to the one before saving ",*(spEw3->getBoxController())==*(new_bc));

    std::vector<API::IMDNode *>Boxes;
    TS_ASSERT_THROWS_NOTHING(BoxStoredTree.restoreBoxTree(Boxes ,new_bc, false,false));

    std::vector<API::IMDNode *>OldBoxes;
    TS_ASSERT_THROWS_NOTHING(spEw3->getBoxes(OldBoxes, 1000, false));
    // just in case, should be already sorted
    API::IMDNode::sortObjByID(OldBoxes);

    for(size_t i=0;i<OldBoxes.size();i++)
    {
      TS_ASSERT(OldBoxes[i]->getID()==Boxes[i]->getID());
      size_t numChildren = Boxes[i]->getNumChildren();
      TS_ASSERT(OldBoxes[i]->getNumChildren()==numChildren);
      if(numChildren>0)
      {
         TS_ASSERT(OldBoxes[i]->getChild(0)->getID()==Boxes[i]->getChild(0)->getID());
      }
    }


    if(testFile.exists())
      testFile.remove();
  }

private:
 
    Mantid::API::IMDEventWorkspace_sptr spEw3;

};


#endif