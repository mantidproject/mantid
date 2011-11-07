#ifndef CUSTOM_INTERFACES_INELASTIC_ISIS_TEST_H_
#define CUSTOM_INTERFACES_INELASTIC_ISIS_TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidQtCustomInterfaces/InelasticISIS.h"
#include "MantidQtCustomInterfaces/WorkspaceMemento.h"
#include "MantidQtCustomInterfaces/WorkspaceMementoService.h"
#include "MantidQtCustomInterfaces/WorkspaceMementoItem.h"
#include "MantidQtCustomInterfaces/LoanedMemento.h"
#include "MantidQtCustomInterfaces/ParameterisedLatticeView.h"
#include "MantidDataObjects/MementoTableWorkspace.h"
#include "MantidAPI/TableRow.h" 

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class InelasticISISTest : public CxxTest::TestSuite
{

private:

    // Helper method to generate a workspace memento;
  /*static WorkspaceMemento* makeMemento()
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
  }*/

public:

//=====================================================================================
// Functional tests
//=====================================================================================

   //Test lattice view apects of this 'Approach'
   void  testsLatticeView()
   {
   //  LoanedMemento loaned(makeMemento());
   //  InelasticISIS* factory = new InelasticISIS(loaned);

   //  LatticeView* view;
   //  TS_ASSERT_THROWS_NOTHING(view = factory->createLatticeView());
   //  TS_ASSERT(NULL != view);
   //  TS_ASSERT(NULL != dynamic_cast<ParameterisedLatticeView*>(view));
   //  delete view;
   }

   //TODO. Test more Factory methods on abstract factory.
};

#endif