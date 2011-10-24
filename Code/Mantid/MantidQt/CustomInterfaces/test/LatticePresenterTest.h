#ifndef CUSTOM_INTERFACES_LATTICE_PRESENTER_TEST_H_
#define CUSTOM_INTERFACES_LATTICE_PRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidDataObjects/MementoTableWorkspace.h"
#include "MantidAPI/TableRow.h" 
#include "MantidQtCustomInterfaces/WorkspaceMemento.h"
#include "MantidQtCustomInterfaces/WorkspaceMementoItem.h"
#include "MantidQtCustomInterfaces/LoanedMemento.h"
#include "MantidQtCustomInterfaces/LatticePresenter.h"
#include "MantidQtCustomInterfaces/LatticeView.h"
#include "MantidQtCustomInterfaces/WorkspaceMementoService.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace MantidQt::CustomInterfaces;
using namespace testing;

class LatticePresenterTest : public CxxTest::TestSuite
{

private:

  /// Helper mock class for the MVP view (LatticeView)
  class MockLatticeView : public LatticeView 
  {
  public:
    MOCK_CONST_METHOD0(getA1,
      double());
    MOCK_CONST_METHOD0(getA2,
      double());
    MOCK_CONST_METHOD0(getA3,
      double());
    MOCK_CONST_METHOD0(getB1,
      double());
    MOCK_CONST_METHOD0(getB2,
      double());
    MOCK_CONST_METHOD0(getB3,
      double());
    MOCK_METHOD0(indicateModified,
      void());
    MOCK_METHOD0(indicateDefault,
      void());
    MOCK_METHOD0(indicateInvalid,
      void());
    MOCK_METHOD6(initalize,
      void(double a1, double a2, double a3, double b1, double b2, double b3));
  };

  // Helper method to generate a workspace memento;
  static WorkspaceMemento* makeMemento()
  {
    TableWorkspace_sptr ws(new MementoTableWorkspace(1));
    TableRow row = ws->getRow(0);
    row << "TestWSRow" << "CNCS" << 1 << "SampleXML" << 1.0 << 1.0 << 1.0 << 90.0 << 90.0 << 90.0 << "Not Ready";
    int rowIndex = 0;

    WorkspaceMemento* memento = new WorkspaceMemento(ws, "TestWSRow", rowIndex);
    LoanedMemento managed(memento);
    WorkspaceMementoService<LoanedMemento> service(managed);
    service.addAllItems(ws, rowIndex);
    return memento;
  }


public:

//=====================================================================================
// Functional tests
//=====================================================================================
   void  testConstruction()
   {
     //Create a view to drive.
     MockLatticeView* view = new MockLatticeView();
     WorkspaceMemento* wsMemento = makeMemento();

     {
       EXPECT_CALL(*view, initalize(_, _, _, _, _, _)).Times(1); //Presenter will initalize view.
       //EXPECT_CALL(*view, indicateValid()).Times(1); //Because we're going to provide valid lattice data.

       //Make some input data to give to the presenter.
     
       LoanedMemento loanedMemento(wsMemento);

       //Create the presenter and give it the view.
       LatticePresenter presenter(loanedMemento);
       presenter.acceptView(view);

       TS_ASSERT(Mock::VerifyAndClearExpectations(view));
     }

     delete wsMemento;
     delete view;
   }

   void  testConstructionWithInvalidLattice()
   {
     MockLatticeView* view = new MockLatticeView();
     WorkspaceMemento* wsMemento = makeMemento();

     //Now overrite the lattice portion with junk/invalid numbers.
     std::vector<double> vars(6, 0); //All six variables initalized to zero!
     wsMemento->getItem(4)->setValue(vars[0]); //a1
     wsMemento->getItem(5)->setValue(vars[1]); //a2
     wsMemento->getItem(6)->setValue(vars[2]); //a3
     wsMemento->getItem(7)->setValue(vars[3]); //b1
     wsMemento->getItem(8)->setValue(vars[4]); //b2
     wsMemento->getItem(9)->setValue(vars[5]); //b3

     {
       EXPECT_CALL(*view, initalize(_, _, _, _, _, _)).Times(1); //Presenter will initalize view.
       EXPECT_CALL(*view, indicateInvalid()).Times(1); //Because we're going to provide bad lattice data.

       LoanedMemento loanedMemento(wsMemento);

       //Create the presenter and give it the view.
       LatticePresenter presenter(loanedMemento);
       presenter.acceptView(view);

       TS_ASSERT(Mock::VerifyAndClearExpectations(view));
     }

     delete wsMemento;
     delete view;
   }

   void  testNothingChanged()
   {
     //Create a view to drive.
     MockLatticeView* view = new MockLatticeView();
     WorkspaceMemento* wsMemento = makeMemento();

     {
       EXPECT_CALL(*view, initalize(_, _, _, _, _, _)).Times(1); //Presenter will initalize view.
       // --- Region These getteres will return the same values as provided originally. see makeMemento()!
       EXPECT_CALL(*view, getA1()).WillOnce(Return(1));
       EXPECT_CALL(*view, getA2()).WillOnce(Return(1));
       EXPECT_CALL(*view, getA3()).WillOnce(Return(1));
       EXPECT_CALL(*view, getB1()).WillOnce(Return(90));
       EXPECT_CALL(*view, getB2()).WillOnce(Return(90));
       EXPECT_CALL(*view, getB3()).WillOnce(Return(90));
       // -- End Region
       EXPECT_CALL(*view, indicateDefault()).Times(1); //Because Nothing is going to change!  VIEW WILL BE TOLD THIS.

       //Make some input data to give to the presenter.
     
       LoanedMemento loanedMemento(wsMemento);

       //Create the presenter and give it the view.
       LatticePresenter presenter(loanedMemento);
       presenter.acceptView(view);
       presenter.update(); // Update, but nothing has changed, should now go ahead and tell view this!

       TS_ASSERT(Mock::VerifyAndClearExpectations(view));
     }

     delete wsMemento;
     delete view;
   }

   void  testChangedButValid()
   {
     //Create a view to drive.
     MockLatticeView* view = new MockLatticeView();
     WorkspaceMemento* wsMemento = makeMemento();

     {
       EXPECT_CALL(*view, initalize(_, _, _, _, _, _)).Times(1); //Presenter will initalize view.
       // --- Region These getteres will return different values from those provided originally. see makeMemento()!
       EXPECT_CALL(*view, getA1()).WillOnce(Return(2));
       EXPECT_CALL(*view, getA2()).WillOnce(Return(2));
       EXPECT_CALL(*view, getA3()).WillOnce(Return(2));
       EXPECT_CALL(*view, getB1()).WillOnce(Return(90));
       EXPECT_CALL(*view, getB2()).WillOnce(Return(90));
       EXPECT_CALL(*view, getB3()).WillOnce(Return(90));
       // -- End Region
       EXPECT_CALL(*view, indicateModified()).Times(1); //Because stuff is going to change!  VIEW WILL BE TOLD THIS.

       //Make some input data to give to the presenter.
     
       LoanedMemento loanedMemento(wsMemento);

       //Create the presenter and give it the view.
       LatticePresenter presenter(loanedMemento);
       presenter.acceptView(view);
       presenter.update(); // Update, but now stuff  has changed, should now go ahead and tell view this!

       TS_ASSERT(Mock::VerifyAndClearExpectations(view));
     }

     delete wsMemento;
     delete view;
   }

   void  testChangedButNotValid()
   {
     //Create a view to drive.
     MockLatticeView* view = new MockLatticeView();
     WorkspaceMemento* wsMemento = makeMemento();

     {
       EXPECT_CALL(*view, initalize(_, _, _, _, _, _)).Times(1); //Presenter will initalize view.
       // --- Region These getteres will return different/invalid values from those provided originally. see makeMemento()!
       EXPECT_CALL(*view, getA1()).WillOnce(Return(0));
       EXPECT_CALL(*view, getA2()).WillOnce(Return(0));
       EXPECT_CALL(*view, getA3()).WillOnce(Return(0));
       EXPECT_CALL(*view, getB1()).WillOnce(Return(0));
       EXPECT_CALL(*view, getB2()).WillOnce(Return(0));
       EXPECT_CALL(*view, getB3()).WillOnce(Return(0));
       // -- End Region
       EXPECT_CALL(*view, indicateInvalid()).Times(1); //Because stuff is going to change for the worse! VIEW WILL BE TOLD THIS.

       //Make some input data to give to the presenter.
     
       LoanedMemento loanedMemento(wsMemento);

       //Create the presenter and give it the view.
       LatticePresenter presenter(loanedMemento);
       presenter.acceptView(view);
       presenter.update(); // Update, but now stuff has changed for the worse, should now go ahead and tell view this!

       TS_ASSERT(Mock::VerifyAndClearExpectations(view));
     }

     delete wsMemento;
     delete view;
   }

};

#endif