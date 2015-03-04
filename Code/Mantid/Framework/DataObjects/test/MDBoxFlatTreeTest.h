#ifndef MANTID_DATAOBJECTS_MDBOX_FLATTREE_H_
#define MANTID_DATAOBJECTS_MDBOX_FLATTREE_H_

#include "MantidDataObjects/MDBoxFlatTree.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidDataObjects/MDLeanEvent.h"

#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>
#include <Poco/File.h>

using Mantid::DataObjects::MDBoxFlatTree;

class MDBoxFlatTreeTest :public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDBoxFlatTreeTest  *createSuite() { return new MDBoxFlatTreeTest (); }
  static void destroySuite( MDBoxFlatTreeTest  *suite ) { delete suite; }

  MDBoxFlatTreeTest()
  {
    using Mantid::DataObjects::MDEventsTestHelper::makeFakeMDEventWorkspace;
    // make non-file backet mdEv workspace with 10000 events
    spEw3 = makeFakeMDEventWorkspace("TestLeanEvWS", 10000);
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
    auto new_bc = boost::make_shared<Mantid::API::BoxController>(nDim);
    new_bc->fromXMLString(BoxStoredTree.getBCXMLdescr());

    TSM_ASSERT("Should restore the box controller equal to the one before saving ",*(spEw3->getBoxController())==*(new_bc));

    std::vector<Mantid::API::IMDNode *>Boxes;
    TS_ASSERT_THROWS_NOTHING(BoxStoredTree.restoreBoxTree(Boxes ,new_bc, false,false));

    std::vector<Mantid::API::IMDNode *>OldBoxes;
    TS_ASSERT_THROWS_NOTHING(spEw3->getBoxes(OldBoxes, 1000, false));
    // just in case, should be already sorted
    Mantid::API::IMDNode::sortObjByID(OldBoxes);

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
