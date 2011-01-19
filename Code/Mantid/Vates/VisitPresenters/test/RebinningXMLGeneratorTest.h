#ifndef REBINNING_XML_GENERATOR_TEST_H
#define REBINNING_XML_GENERATOR_TEST_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cxxtest/TestSuite.h>
#include <MantidGeometry/MDGeometry/MDPoint.h>
#include <MantidGeometry/MDGeometry/IMDDimension.h>
#include "MantidVisitPresenters/RebinningXMLGenerator.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/ImplicitFunction.h"
#include "boost/shared_ptr.hpp"

using namespace Mantid::VATES;

class RebinningXMLGeneratorTest: public CxxTest::TestSuite
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
    MOCK_CONST_METHOD0(getNPoints,long unsigned int());
    MOCK_CONST_METHOD1(getPoint,const Mantid::Geometry::SignalAggregate&(int index));
    MOCK_CONST_METHOD1(getCell,const Mantid::Geometry::SignalAggregate&(int dim1Increment));
    MOCK_CONST_METHOD2(getCell,const Mantid::Geometry::SignalAggregate&(int dim1Increment, int dim2Increment));
    MOCK_CONST_METHOD3(getCell,const Mantid::Geometry::SignalAggregate&(int dim1Increment, int dim2Increment, int dim3Increment));
    MOCK_CONST_METHOD4(getCell,const Mantid::Geometry::SignalAggregate&(int dim1Increment, int dim2Increment, int dim3Increment, int dim4Increment));

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
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> gettDimension() const
    {
      throw std::runtime_error("Not Implemented");
    }
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> getDimension(std::string id) const
    {
      throw std::runtime_error("Not Implemented");
    }
    const Mantid::Geometry::SignalAggregate& getCell(...) const
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
  RebinningXMLGenerator generator;
  boost::shared_ptr<const Mantid::API::ImplicitFunction> impFunction(new MockImplicitFunction);
  generator.setImplicitFunction(impFunction);
  TSM_ASSERT_THROWS("Cannot generate the xml without the workspace", generator.createXMLString(), std::runtime_error);
}

void testNoImplicitFunctionThrows()
{
  RebinningXMLGenerator generator;
  MockIMDWorkspace* pWorkspace = new MockIMDWorkspace;
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
  EXPECT_CALL(*pWorkspace, getGeometryXML()).Times(1).WillRepeatedly(testing::Return(""));
  EXPECT_CALL(*pWorkspace, getWSLocation()).Times(1).WillRepeatedly(testing::Return("../somelocation/somefile.sqw"));
  boost::shared_ptr<const Mantid::API::IMDWorkspace> workspace(pWorkspace);
  RebinningXMLGenerator generator;
  generator.setImplicitFunction(impFunction);
  generator.setWorkspace(workspace);

  TSM_ASSERT_THROWS("Cannot create the xml without geometry xml", generator.createXMLString(), std::runtime_error);
}

void testNoLocationThrows()
{
  boost::shared_ptr<const Mantid::API::ImplicitFunction> impFunction(new MockImplicitFunction);
  MockIMDWorkspace* pWorkspace = new MockIMDWorkspace;
  EXPECT_CALL(*pWorkspace, getGeometryXML()).Times(1).WillRepeatedly(testing::Return("<DimensionSet/>"));
  EXPECT_CALL(*pWorkspace, getWSLocation()).Times(1).WillRepeatedly(testing::Return(""));
  boost::shared_ptr<const Mantid::API::IMDWorkspace> workspace(pWorkspace);
  RebinningXMLGenerator generator;
  generator.setImplicitFunction(impFunction);
  generator.setWorkspace(workspace);

  TSM_ASSERT_THROWS("Cannot create the xml without the workspace location", generator.createXMLString(),
      std::runtime_error);
}

void testNoNameThrows()
{
  boost::shared_ptr<const Mantid::API::ImplicitFunction> impFunction(new MockImplicitFunction);
  MockIMDWorkspace* pWorkspace = new MockIMDWorkspace;
  EXPECT_CALL(*pWorkspace, getGeometryXML()).Times(1).WillRepeatedly(testing::Return("<DimensionSet/>"));
  EXPECT_CALL(*pWorkspace, getWSLocation()).Times(1).WillRepeatedly(testing::Return("..../somelocation/somefile.sqw"));
  boost::shared_ptr<const Mantid::API::IMDWorkspace> workspace(pWorkspace);
  RebinningXMLGenerator generator;
  generator.setImplicitFunction(impFunction);
  generator.setWorkspace(workspace);

  TSM_ASSERT_THROWS("Cannot create the xml without the workspace name", generator.createXMLString(),
      std::runtime_error);
}

void testCreateXMLWithWorkspace() //Uses the workspace settter.
{
  MockImplicitFunction* pImpFunction = new MockImplicitFunction;
  EXPECT_CALL(*pImpFunction, toXMLString()).Times(1).WillRepeatedly(testing::Return("<ImplicitFunction/>"));

  MockIMDWorkspace* pWorkspace = new MockIMDWorkspace("name");
  EXPECT_CALL(*pWorkspace, getGeometryXML()).Times(1).WillRepeatedly(testing::Return("<DimensionSet/>"));
  EXPECT_CALL(*pWorkspace, getWSLocation()).Times(1).WillRepeatedly(testing::Return("location"));

  boost::shared_ptr<const Mantid::API::IMDWorkspace> workspace(pWorkspace);
  boost::shared_ptr<const Mantid::API::ImplicitFunction> impFunction(pImpFunction);
  RebinningXMLGenerator generator;

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

  RebinningXMLGenerator generator;
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
  RebinningXMLGenerator generator;
  //Apply setters.
  generator.setWorkspaceName("name");
  generator.setWorkspaceLocation("location");
  generator.setGeometryXML("<DimensionSet/>");

  std::string xml = generator.createXMLString();
  TSM_ASSERT_EQUALS("The xml has been created without a function incorrectly", "<MDInstruction><MDWorkspaceName>name</MDWorkspaceName><MDWorkspaceLocation>location</MDWorkspaceLocation><DimensionSet/></MDInstruction>", xml);
}

void testGetGeometryXML()
{
  RebinningXMLGenerator generator;
  generator.setWorkspaceName("name");
  generator.setWorkspaceLocation("location");
  std::string dimensionXMLString = "<DimensionSet/>";
  generator.setGeometryXML(dimensionXMLString);

  std::string xml = generator.getWorkspaceGeometry();
  TSM_ASSERT_EQUALS("The geometry xml fetched is not the same as that provided", dimensionXMLString, generator.getWorkspaceGeometry());
}


};

#endif
