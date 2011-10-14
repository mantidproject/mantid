#ifndef CUSTOM_INTERFACES_WORKSPACE_MEMENTO_SERVICE_TEST_H_
#define CUSTOM_INTERFACES_WORKSPACE_MEMENTO_SERVICE_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtCustomInterfaces/WorkspaceMementoService.h"
#include "MantidQtCustomInterfaces/AbstractMementoItem.h"
#include "MantidQtCustomInterfaces/WorkspaceMemento.h"
#include <boost/shared_ptr.hpp>

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;

class WorkspaceMementoServiceTest : public CxxTest::TestSuite
{

private:

  //Helper method to add data.
  static void addSomeData(ITableWorkspace_sptr ws)
  {
    ws->insertRow(0);
    TableRow row = ws->getRow(0);
    row << "TestWSRow" << "CNCS" << 1 << "SampleXML" << 1.0 << 1.0 << 1.0 << 90.0 << 90.0 << 90.0 << "Not Ready";
  }

  //Helper typedef
  typedef boost::shared_ptr<WorkspaceMementoService<WorkspaceMemento*> > WorkspaceMementoService_sptr;

  /*Helper method to generate a standard Service wrapping a memento with a single row of data.
  1) Creates a MementoTableWorkspace.
  2) Add a row with some data to that MementoTableWorkspace.
  3) Create a memento to wrap the Row.
  4) Create a service to point at the memento.
  5) Add all items to the memento via the service.
  6) Return the service.
  */
  static WorkspaceMementoService_sptr createService()
  {
     ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable("MementoTableWorkspace");
     addSomeData(ws);

     WorkspaceMemento* memento = new WorkspaceMemento(ws, "WsName");
     WorkspaceMementoService<WorkspaceMemento*>* service = new WorkspaceMementoService<WorkspaceMemento*>(memento);
     service->addAllItems(ws, 0);
     return WorkspaceMementoService_sptr(service);
  }

  /*Helper method to generate a standard Service wrapping a memento with a single row of data.
  1) Creates a MementoTableWorkspace.
  2) Add a row with some data to that MementoTableWorkspace.
  3) Create a memento to wrap the Row.
  4) Create a service to point at the memento.
  5) Add all items to the memento via the service.
  6) Return the service.
  */
  static WorkspaceMementoService_sptr createServiceWithLogValues()
  {
     ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable("MementoTableWorkspace");
     ws->addColumn("str", "Temp");
     ws->addColumn("str", "Pressure");

     ws->insertRow(0);
     TableRow row = ws->getRow(0);
     row << "TestWSRow" << "CNCS" << 1 << "SampleXML" << 1.0 << 1.0 << 1.0 << 90.0 << 90.0 << 90.0 << "Not Ready" << "1" << "2";

     WorkspaceMemento* memento = new WorkspaceMemento(ws, "WsName");
     WorkspaceMementoService<WorkspaceMemento*>* service = new WorkspaceMementoService<WorkspaceMemento*>(memento);
     service->addAllItems(ws, 0);
     return WorkspaceMementoService_sptr(service);
  }

public:

//=====================================================================================
// Functional tests
//=====================================================================================

   void testAddItemsThrows()
   {
     ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable("TableWorkspace");
     ws->addColumn("str", "WsName");
     ws->insertRow(0);

     WorkspaceMementoService<WorkspaceMemento*> service(new WorkspaceMemento(ws, "WsName"));
     TSM_ASSERT_THROWS("Should throw as invalid schema used to populate the collection.", service.addAllItems(ws, 0), std::runtime_error);
   }

   void testTestWrongColunmNumber()
   {
     WorkspaceMementoService_sptr service = createService();

     ITableWorkspace_sptr candidate = WorkspaceFactory::Instance().createTable("TableWorkspace");
     candidate->addColumn("str", "WsName");
     candidate->insertRow(0);

     TSM_ASSERT("Wrong number of columns. Schema match should fail.", !service->validMementoTableSchema(candidate));
   }

   void testPassValidation()
   {
     WorkspaceMementoService_sptr service = createService();

     ITableWorkspace_sptr candidate = WorkspaceFactory::Instance().createTable("MementoTableWorkspace");
     addSomeData(candidate);

     TSM_ASSERT("Mismatch between MementoTableWorkspaceSchema and WorkspaceMemento Schema", service->validMementoTableSchema(candidate));
   }

   void testSetWSName()
   {
     WorkspaceMementoService_sptr service = createService();

     TS_ASSERT_THROWS_NOTHING(service->setWorkspaceName("RandomWsName"));
     TS_ASSERT_EQUALS("RandomWsName", service->getWorkspaceName());
   }

   void testSetInstrumentName()
   {
     WorkspaceMementoService_sptr service = createService();

     TS_ASSERT_THROWS_NOTHING(service->setInstrumentName("RandomInstrumentName"));
     TS_ASSERT_EQUALS("RandomInstrumentName", service->getInstrumentName());
   }

   void testSetRunNumber()
   {
     WorkspaceMementoService_sptr service = createService();

     TS_ASSERT_THROWS_NOTHING(service->setRunNumber(2));
     TS_ASSERT_EQUALS(2, service->getRunNumber());
   }

   void testSetSampleXML()
   {
     WorkspaceMementoService_sptr service = createService();

     TS_ASSERT_THROWS_NOTHING(service->setShapeXML("<other></other>"));
     TS_ASSERT_EQUALS("<other></other>", service->getShapeXML());
   }

   void testSetLatticeParameters()
   {
     WorkspaceMementoService_sptr service = createService();

     TS_ASSERT_THROWS_NOTHING(service->setLatticeParameters(2, 2, 2, 90, 90, 90));
     TS_ASSERT_EQUALS(2, service->getA1());
     TS_ASSERT_EQUALS(2, service->getA2());
     TS_ASSERT_EQUALS(2, service->getA3());
     TS_ASSERT_EQUALS(90, service->getB1());
     TS_ASSERT_EQUALS(90, service->getB2());
     TS_ASSERT_EQUALS(90, service->getB3());
   }

   
   void testSetLogValues()
   {
     /*
     using namespace Mantid::Kernel;
     std::vector<Mantid::Kernel::Property*> vecLogValues;
     vecLogValues.push_back(new PropertyWithValue<std::string>("Temp" , "2.1", Direction::Input));
     vecLogValues.push_back(new PropertyWithValue<std::string>("Pressure" , "4", Direction::Input));

     WorkspaceMementoService_sptr service = createServiceWithLogValues();

     TS_ASSERT_THROWS_NOTHING(service->setLogData(vecLogValues));
     std::vector<AbstractMementoItem_sptr> fetchedLogValues = service->getLogData();

     TSM_ASSERT_EQUALS("Wrong number of log values.", 2, fetchedLogValues.size());

     std::string temp;
     fetchedLogValues[0]->getValue(temp);
     std::string pressure;
     fetchedLogValues[1]->getValue(pressure);

     TS_ASSERT_EQUALS("2.1", temp);
     TS_ASSERT_EQUALS("4", pressure);
     */
   }
   

   void testSetStatus()
   {
     WorkspaceMementoService_sptr service = createService();
     TS_ASSERT_THROWS_NOTHING(service->setStatus("Ready For Anything"));
     TS_ASSERT_EQUALS("Ready For Anything", service->getStatus());
   }
};

#endif