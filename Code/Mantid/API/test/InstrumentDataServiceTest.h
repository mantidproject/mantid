#ifndef INSTRUMENTDATASERVICETEST_H_
#define INSTRUMENTDATASERVICETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/Instrument.h"
#include "MantidKernel/Exception.h"
#include <iostream>

class InstrumentDataServiceTest : public CxxTest::TestSuite
{
public:
	InstrumentDataServiceTest():inst1(new Mantid::API::Instrument), inst2(new Mantid::API::Instrument)
	{
		//Mantid::API::FrameworkManager::Instance();
	}
	void testAdd()
	{
		// Adding an Instrument with empty name should throw
		TS_ASSERT_THROWS(Mantid::API::InstrumentDataService::Instance().add("",inst1),std::runtime_error);
		// This should not throw, valid name
		TS_ASSERT_THROWS_NOTHING(Mantid::API::InstrumentDataService::Instance().add("inst1",inst1));
		TS_ASSERT_EQUALS(inst1.use_count(),2);
	}
	void testAddOrReplace()
	{
		// AddorReplace an Instrument with empty name should throw
		TS_ASSERT_THROWS(Mantid::API::InstrumentDataService::Instance().addOrReplace("",inst2),std::runtime_error);
		TS_ASSERT_THROWS_NOTHING(Mantid::API::InstrumentDataService::Instance().addOrReplace("inst2",inst2));
		TS_ASSERT_EQUALS(inst2.use_count(),2);
		//Test replace
		TS_ASSERT_THROWS_NOTHING(Mantid::API::InstrumentDataService::Instance().addOrReplace("inst1",inst2));
		TS_ASSERT_EQUALS(inst2.use_count(),3);
		TS_ASSERT_EQUALS(inst1.use_count(),1);
		// i
		TS_ASSERT_EQUALS(Mantid::API::InstrumentDataService::Instance().retrieve("inst1"),inst2);
		//Change back 
		TS_ASSERT_THROWS_NOTHING(Mantid::API::InstrumentDataService::Instance().addOrReplace("inst1",inst1));
		TS_ASSERT_EQUALS(inst2.use_count(),2);
		TS_ASSERT_EQUALS(inst1.use_count(),2);
}
	void testSize()
	{
		// Number of elements in the store should now be 1
		TS_ASSERT_EQUALS(Mantid::API::InstrumentDataService::Instance().size(),2);
	}
	void testRetrieve()
	{
		// Retrieve the instrument
		TS_ASSERT_EQUALS(Mantid::API::InstrumentDataService::Instance().retrieve("inst1"),inst1);
		// Should throw if the instrument can not be retrieved
		TS_ASSERT_THROWS(Mantid::API::InstrumentDataService::Instance().retrieve("notregistered"),Mantid::Kernel::Exception::NotFoundError);			
	}
	void testRemove()
	{
		// Removing a non-existing data Object should give a warning in the Log but not throw
		TS_ASSERT_THROWS_NOTHING(Mantid::API::InstrumentDataService::Instance().remove("inst3"));
		// Removing a valid instrument
		TS_ASSERT_THROWS_NOTHING(Mantid::API::InstrumentDataService::Instance().remove("inst1"));
		TS_ASSERT_EQUALS(Mantid::API::InstrumentDataService::Instance().size(),1);
		TS_ASSERT_EQUALS(inst1.use_count(),1);
	}
	void testClear()
	{
		TS_ASSERT_THROWS_NOTHING(Mantid::API::InstrumentDataService::Instance().clear());
		TS_ASSERT_EQUALS(Mantid::API::InstrumentDataService::Instance().size(),0);
		TS_ASSERT_EQUALS(inst1.use_count(),1);
		TS_ASSERT_EQUALS(inst2.use_count(),1);
	}
	void testDoesExist()
	{
		// Add inst1 
		Mantid::API::InstrumentDataService::Instance().add("inst1",inst1);
		TS_ASSERT_THROWS_NOTHING(Mantid::API::InstrumentDataService::Instance().doesExist("youpla"));;
		TS_ASSERT(Mantid::API::InstrumentDataService::Instance().doesExist("inst1"));
		TS_ASSERT(!Mantid::API::InstrumentDataService::Instance().doesExist("inst3"));
	}
	void testGetObjectNames()
	{
		Mantid::API::InstrumentDataService::Instance().add("inst2",inst2);
		std::vector<std::string> names(2);
		names[0]="inst1";
		names[1]="inst2";
		std::vector<std::string> result;
		result=Mantid::API::InstrumentDataService::Instance().getObjectNames();
		TS_ASSERT_EQUALS(result,names);
		//Check with an empty store
		Mantid::API::InstrumentDataService::Instance().clear();
		names.clear();
		result=Mantid::API::InstrumentDataService::Instance().getObjectNames();
		TS_ASSERT_EQUALS(result,names);
	}
private:
	boost::shared_ptr<Mantid::API::Instrument> inst1, inst2;
};

#endif /*INSTRUMENTDATASERVICETEST_H_*/

