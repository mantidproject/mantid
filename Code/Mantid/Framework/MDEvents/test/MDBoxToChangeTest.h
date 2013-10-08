#ifndef MANTID_MDEVENTS_MDBOX_TOCHANGEETEST_H_
#define MANTID_MDEVENTS_MDBOX_TOCHANGEETEST_H_

#include "MantidMDEvents/MDBoxToChange.h"
#include "MantidMDEvents/../../src/MDBoxToChange.cpp"
#include "MantidMDEvents/MDEvent.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace API;
using namespace Mantid::MDEvents;


class MDBoxToChangeTest : public CxxTest::TestSuite
{

    BoxController_sptr splitter;
    MDBoxBase<MDEvent<2>,2>* rootBox;

public:
static MDBoxToChangeTest *createSuite() { return new MDBoxToChangeTest(); }
static void destroySuite(MDBoxToChangeTest * suite) { delete suite; }

void testConstructor()
{
  rootBox = makeMDBox2();
  MDBoxToChange<MDEvent<2>,2> * pBoxChanger(NULL);
  TS_ASSERT_THROWS_NOTHING((pBoxChanger = new MDBoxToChange<MDEvent<2> , 2>()));
  delete pBoxChanger;
  TS_ASSERT_THROWS_NOTHING((pBoxChanger = new MDBoxToChange<MDEvent<2> , 2>(dynamic_cast<MDBox<MDEvent<2>,2>*>(rootBox),0)));
  
  delete pBoxChanger;

}

void testSplitRootToGridbox()
{
  MDBoxToChange<MDEvent<2>,2>  BoxToSplit(dynamic_cast<MDBox<MDEvent<2>,2>*>(rootBox),0);

  TSM_ASSERT("root box at this stage has to be an MDBox:",(dynamic_cast<MDBox<MDEvent<2>,2>*>(rootBox)));

  TS_ASSERT_THROWS_NOTHING(rootBox=BoxToSplit.splitToGridBox());
  TSM_ASSERT("root box at this stage has to be an MDGridBox:",(dynamic_cast<MDGridBox<MDEvent<2>,2>*>(rootBox)));

  TSM_ASSERT("root box and internal box for this stuff should be equal :",rootBox == BoxToSplit.getParent());
}

void testSplitAMemberToGridbox()
{
  API::IMDNode * aChildBox(NULL);
  TS_ASSERT_THROWS_NOTHING(aChildBox = rootBox->getChild(10));

  MDBoxToChange<MDEvent<2>,2>  BoxToSplit(dynamic_cast<MDBox<MDEvent<2>,2>*>(aChildBox),10);

  TSM_ASSERT("parent for the box to split should be rootbox: ",(BoxToSplit.getParent() == rootBox));

  MDBoxBase<MDEvent<2>,2>* aGridBox(NULL);
  TS_ASSERT_THROWS_NOTHING(aGridBox=BoxToSplit.splitToGridBox());

  TSM_ASSERT("This should be a grid box",(dynamic_cast<MDGridBox<MDEvent<2>,2>*>(aGridBox)));
  TSM_ASSERT("and this grid box siting in place 10 of the root grid-box:",((dynamic_cast<MDGridBox<MDEvent<2>,2>*>(aGridBox))==dynamic_cast<MDGridBox<MDEvent<2>,2>*>(rootBox->getChild(10))));

  
}

MDBoxToChangeTest()
{
  splitter = BoxController_sptr(new BoxController(2));
}

private:
  /** Generate an MDBox , 10x10*/
MDBox<MDEvent<2>,2> * makeMDBox2()
{

    splitter->setSplitThreshold(5);
    // Splits into 10 boxes
    splitter->setSplitInto(10);
    // Set the size
    MDBox<MDEvent<2>,2> * out = new MDBox<MDEvent<2>,2>(splitter.get());
    out->setExtents(0, 0.0, 10.0);
    out->setExtents(1, 0.0, 10.0);
    out->calcVolume();
    // Fill events that are more spread in dimension 1.
    for (double x=40; x<60; x++) //20
      for (double y=20; y<80; y++) //60
      {
        coord_t centers[2] = {coord_t(x*0.1),coord_t(y*0.1 + 0.05)};
        out->addEvent( MDEvent<2>(2.0, 2.0, centers) );
      }

    return out;
}


};

#endif
