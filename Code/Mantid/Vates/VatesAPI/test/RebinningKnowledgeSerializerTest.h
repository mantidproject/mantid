#ifndef REBINNING_XML_GENERATOR_TEST_H
#define REBINNING_XML_GENERATOR_TEST_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cxxtest/TestSuite.h>
#include <MantidGeometry/MDGeometry/MDPoint.h>
#include <MantidGeometry/MDGeometry/IMDDimension.h>
#include "MantidVatesAPI/RebinningKnowledgeSerializer.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/ImplicitFunction.h"
#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>

using namespace Mantid::VATES;

class RebinningKnowledgeSerializerTest: public CxxTest::TestSuite
{
private:

  //Helper class
  class MockImplicitFunction: public Mantid::API::ImplicitFunction
  {
  public:
    MOCK_CONST_METHOD1(evaluate, bool(const Mantid::API::Point3D* pPoint));
    MOCK_CONST_METHOD0(getName, std::string());
    MOCK_CONST_METHOD0(toXMLString, std::string());
    ~MockImplicitFunction()
    {
    }
  };

  //Helper class. Not all methods can be mocked.

  class MockIMDWorkspace: public Mantid::API::IMDWorkspace
  {
  public:

    MOCK_CONST_METHOD0(id, const std::string());
    MOCK_CONST_METHOD0(getMemorySize, size_t());
    MOCK_CONST_METHOD1(getPoint,const Mantid::Geometry::SignalAggregate&(size_t index));
    MOCK_CONST_METHOD1(getCell,const Mantid::Geometry::SignalAggregate&(size_t dim1Increment));
    MOCK_CONST_METHOD2(getCell,const Mantid::Geometry::SignalAggregate&(size_t dim1Increment, size_t dim2Increment));
    MOCK_CONST_METHOD3(getCell,const Mantid::Geometry::SignalAggregate&(size_t dim1Increment, size_t dim2Increment, size_t dim3Increment));
    MOCK_CONST_METHOD4(getCell,const Mantid::Geometry::SignalAggregate&(size_t dim1Increment, size_t dim2Increment, size_t dim3Increment, size_t dim4Increment));
    MOCK_CONST_METHOD0(getNonIntegratedDimensions, Mantid::Geometry::VecIMDDimension_const_sptr());
    MOCK_CONST_METHOD0(getWSLocation,std::string());
    MOCK_CONST_METHOD0(getGeometryXML,std::string());

    boost::shared_ptr<const Mantid::Geometry::IMDDimension> getXDimension() const
    {
      throw std::runtime_error("Not Implemented");
    }
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> getYDimension() const
    {
      throw std::runtime_error("Not Implemented");
    }
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> getZDimension() const
    {
      throw std::runtime_error("Not Implemented");
    }
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> getTDimension() const
    {
      throw std::runtime_error("Not Implemented");
    }
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> getDimension(std::string id) const
    {
      UNUSED_ARG(id);
      throw std::runtime_error("Not Implemented");
    }
    const Mantid::Geometry::SignalAggregate& getCell(...) const
    {
      throw std::runtime_error("Not Implemented");
    }
    virtual uint64_t getNPoints() const
    {
      throw std::runtime_error("Not Implemented");
    }
    virtual size_t getNumDims() const
    {
      throw std::runtime_error("Not Implemented");
    }
    virtual const std::vector<std::string> getDimensionIDs() const
    {
      throw std::runtime_error("Not Implemented");
    }

    //constructor allows a workspace name to be provide.
  MockIMDWorkspace(std::string name)
  {
    setName(name);
  }

  MockIMDWorkspace()
  {
  }
};

//Test methods

public:

void testNoWorkspaceThrows()
{
  RebinningKnowledgeSerializer generator;
  boost::shared_ptr<const Mantid::API::ImplicitFunction> impFunction(new MockImplicitFunction);
  generator.setImplicitFunction(impFunction);
  TSM_ASSERT_THROWS("Cannot generate the xml without the workspace", generator.createXMLString(), std::runtime_error);
}

void testNoImplicitFunctionThrows()
{
  RebinningKnowledgeSerializer generator;
  MockIMDWorkspace* pWorkspace = new MockIMDWorkspace;
  pWorkspace->setName("someName");
  EXPECT_CALL(*pWorkspace, getGeometryXML()).Times(1);
  EXPECT_CALL(*pWorkspace, getWSLocation()).Times(1);
  boost::shared_ptr<const Mantid::API::IMDWorkspace> workspace(pWorkspace);
  generator.setWorkspace(workspace);
  TSM_ASSERT_THROWS("Cannot generate the xml without the implicitFunction", generator.createXMLString(), std::runtime_error);
}

void testNoGeometryXMLThrows()
{
  boost::shared_ptr<const Mantid::API::ImplicitFunction> impFunction(new MockImplicitFunction);
  MockIMDWorkspace* pWorkspace = new MockIMDWorkspace;
  pWorkspace->setName("someName");
  EXPECT_CALL(*pWorkspace, getGeometryXML()).Times(1).WillRepeatedly(testing::Return(""));
  EXPECT_CALL(*pWorkspace, getWSLocation()).Times(1).WillRepeatedly(testing::Return("../somelocation/somefile.sqw"));
  boost::shared_ptr<const Mantid::API::IMDWorkspace> workspace(pWorkspace);
  RebinningKnowledgeSerializer generator;
  generator.setImplicitFunction(impFunction);
  generator.setWorkspace(workspace);

  TSM_ASSERT_THROWS("Cannot create the xml without geometry xml", generator.createXMLString(), std::runtime_error);
}

void testNoLocationThrows()
{
  boost::shared_ptr<const Mantid::API::ImplicitFunction> impFunction(new MockImplicitFunction);
  MockIMDWorkspace* pWorkspace = new MockIMDWorkspace;
  pWorkspace->setName("someName");
  EXPECT_CALL(*pWorkspace, getGeometryXML()).Times(1).WillRepeatedly(testing::Return("<DimensionSet/>"));
  EXPECT_CALL(*pWorkspace, getWSLocation()).Times(1).WillRepeatedly(testing::Return(""));
  boost::shared_ptr<const Mantid::API::IMDWorkspace> workspace(pWorkspace);
  RebinningKnowledgeSerializer generator;
  generator.setImplicitFunction(impFunction);
  generator.setWorkspace(workspace);

  TSM_ASSERT_THROWS("Cannot create the xml without the workspace location", generator.createXMLString(),
      std::runtime_error);
}

void testNoLocationDoesNotThrow()
{
  MockIMDWorkspace* pWorkspace = new MockIMDWorkspace;
  pWorkspace->setName("someName");
  EXPECT_CALL(*pWorkspace, getGeometryXML()).Times(1).WillRepeatedly(testing::Return("<DimensionSet/>"));
  EXPECT_CALL(*pWorkspace, getWSLocation()).Times(1).WillRepeatedly(testing::Return(""));
  boost::shared_ptr<const Mantid::API::IMDWorkspace> workspace(pWorkspace);

  MockImplicitFunction* pImpFunction = new MockImplicitFunction;
  EXPECT_CALL(*pImpFunction, toXMLString()).Times(1).WillRepeatedly(testing::Return("<ImplicitFunction/>"));
  boost::shared_ptr<const Mantid::API::ImplicitFunction> impFunction(pImpFunction);
  
  RebinningKnowledgeSerializer generator(LocationNotRequired); //Location is not required.
  generator.setImplicitFunction(impFunction);
  generator.setWorkspace(workspace);
  generator.setImplicitFunction(impFunction);

  TSM_ASSERT_THROWS_NOTHING("The location is not mandatory, should not throw", generator.createXMLString());
}

void testNoNameThrows()
{
  boost::shared_ptr<const Mantid::API::ImplicitFunction> impFunction(new MockImplicitFunction);
  MockIMDWorkspace* pWorkspace = new MockIMDWorkspace;
  EXPECT_CALL(*pWorkspace, getGeometryXML()).Times(1).WillRepeatedly(testing::Return("<DimensionSet/>"));
  EXPECT_CALL(*pWorkspace, getWSLocation()).Times(1).WillRepeatedly(testing::Return("..../somelocation/somefile.sqw"));
  boost::shared_ptr<const Mantid::API::IMDWorkspace> workspace(pWorkspace);
  RebinningKnowledgeSerializer generator;
  generator.setImplicitFunction(impFunction);
  generator.setWorkspace(workspace);

  TSM_ASSERT_THROWS("Cannot create the xml without the workspace name", generator.createXMLString(),
      std::runtime_error);
}

void testCreateXMLWithWorkspace() //Uses the workspace setter.
{
  MockImplicitFunction* pImpFunction = new MockImplicitFunction;
  EXPECT_CALL(*pImpFunction, toXMLString()).Times(1).WillRepeatedly(testing::Return("<ImplicitFunction/>"));

  MockIMDWorkspace* pWorkspace = new MockIMDWorkspace("name");
  EXPECT_CALL(*pWorkspace, getGeometryXML()).Times(1).WillRepeatedly(testing::Return("<DimensionSet/>"));
  EXPECT_CALL(*pWorkspace, getWSLocation()).Times(1).WillRepeatedly(testing::Return("location"));

  boost::shared_ptr<const Mantid::API::IMDWorkspace> workspace(pWorkspace);
  boost::shared_ptr<const Mantid::API::ImplicitFunction> impFunction(pImpFunction);
  RebinningKnowledgeSerializer generator;

  //Apply setters.
  generator.setImplicitFunction(impFunction);
  generator.setWorkspace(workspace);

  std::string xml = generator.createXMLString();

  TSM_ASSERT_EQUALS("The xml has been created, but is incorrect.", "<MDInstruction><MDWorkspaceName>name</MDWorkspaceName><MDWorkspaceLocation>location</MDWorkspaceLocation><DimensionSet/><ImplicitFunction/></MDInstruction>" ,xml)
}

void testCreateXMLWithComponents() //Uses individual setters for geometry, location and name.
{
  MockImplicitFunction* pImpFunction = new MockImplicitFunction;
  EXPECT_CALL(*pImpFunction, toXMLString()).Times(1).WillRepeatedly(testing::Return("<ImplicitFunction/>"));
  boost::shared_ptr<const Mantid::API::ImplicitFunction> impFunction(pImpFunction);

  RebinningKnowledgeSerializer generator;
  //Apply setters.
  generator.setImplicitFunction(impFunction);
  generator.setWorkspaceName("name");
  generator.setWorkspaceLocation("location");
  generator.setGeometryXML("<DimensionSet/>");

  std::string xml = generator.createXMLString();

  TSM_ASSERT_EQUALS("The xml has been created, but is incorrect.", "<MDInstruction><MDWorkspaceName>name</MDWorkspaceName><MDWorkspaceLocation>location</MDWorkspaceLocation><DimensionSet/><ImplicitFunction/></MDInstruction>" ,xml)
}

void testCreateXMLWithoutFunction()
{
  RebinningKnowledgeSerializer generator;
  //Apply setters.
  generator.setWorkspaceName("name");
  generator.setWorkspaceLocation("location");
  generator.setGeometryXML("<DimensionSet/>");

  std::string xml = generator.createXMLString();
  TSM_ASSERT_EQUALS("The xml has been created without a function incorrectly", "<MDInstruction><MDWorkspaceName>name</MDWorkspaceName><MDWorkspaceLocation>location</MDWorkspaceLocation><DimensionSet/></MDInstruction>", xml);
}

void testGetGeometryXML()
{
  RebinningKnowledgeSerializer generator;
  generator.setWorkspaceName("name");
  generator.setWorkspaceLocation("location");
  std::string dimensionXMLString = "<DimensionSet/>";
  generator.setGeometryXML(dimensionXMLString);

  std::string xml = generator.getWorkspaceGeometry();
  TSM_ASSERT_EQUALS("The geometry xml fetched is not the same as that provided", dimensionXMLString, generator.getWorkspaceGeometry());
}

void testHasFunction()
{
  RebinningKnowledgeSerializer withoutFunction;
  RebinningKnowledgeSerializer withFunction;
  boost::shared_ptr<const Mantid::API::ImplicitFunction> impFunction(new MockImplicitFunction);
  withFunction.setImplicitFunction(impFunction);

  TSM_ASSERT_EQUALS("A function has not been provided. ::hasFunctionInfo() should return false.", false,  withoutFunction.hasFunctionInfo());
  TSM_ASSERT_EQUALS("A function has been provided. ::hasFunctionInfo() should return true.", true, withFunction.hasFunctionInfo());
}

void testHasGeometryInfoWithoutGeometry()
{
  //Note that functions do not apply to this test set.
  RebinningKnowledgeSerializer withoutGeometry;
  withoutGeometry.setWorkspaceLocation("-");
  withoutGeometry.setWorkspaceName("-");
  TSM_ASSERT_EQUALS("No Geometry provided. ::hasGeometryInfo() should return false.", false, withoutGeometry.hasGeometryInfo());
}

void testHasGeometryInfoWithoutWSName()
{
  RebinningKnowledgeSerializer withoutWSName;
  withoutWSName.setGeometryXML("-");
  withoutWSName.setWorkspaceLocation("-");
  TSM_ASSERT_EQUALS("No WS name provided. ::hasGeometryInfo() should return false.", false, withoutWSName.hasGeometryInfo());
}

void testHasGeometryInfoWithoutWSLocation()
{
  RebinningKnowledgeSerializer withoutWSLocation;
  withoutWSLocation.setGeometryXML("-");
  withoutWSLocation.setWorkspaceName("-");
  TSM_ASSERT_EQUALS("No WS location provided. ::hasGeometryInfo() should return false.", false, withoutWSLocation.hasGeometryInfo());
}

void testHasGeometryAndWSInfo()
{
  RebinningKnowledgeSerializer withFullGeometryAndWSInfo;
  withFullGeometryAndWSInfo.setGeometryXML("-");
  withFullGeometryAndWSInfo.setWorkspaceName("-");
  withFullGeometryAndWSInfo.setWorkspaceLocation("-");
  TSM_ASSERT_EQUALS("All geometry and ws information has been provided. ::hasGeometryInfo() should return true.", true, withFullGeometryAndWSInfo.hasGeometryInfo());
}


};

#endif
