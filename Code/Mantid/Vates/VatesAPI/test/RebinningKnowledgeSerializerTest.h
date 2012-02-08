#ifndef REBINNING_XML_GENERATOR_TEST_H
#define REBINNING_XML_GENERATOR_TEST_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cxxtest/TestSuite.h>
#include <MantidGeometry/MDGeometry/IMDDimension.h>
#include "MantidVatesAPI/RebinningKnowledgeSerializer.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>

using namespace Mantid::VATES;

class RebinningKnowledgeSerializerTest: public CxxTest::TestSuite
{
private:

  //Helper class
  class MockImplicitFunction: public Mantid::Geometry::MDImplicitFunction
  {
  public:
    MOCK_METHOD1(isPointContained, bool(const Mantid::coord_t* pPoint));
    MOCK_METHOD1(isPointContained, bool(const std::vector<Mantid::coord_t>&));
    MOCK_METHOD1(isPointContained, bool(const Mantid::Kernel::VMD&));
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
    MOCK_CONST_METHOD0(getNonIntegratedDimensions, Mantid::Geometry::VecIMDDimension_const_sptr());
    MOCK_CONST_METHOD0(getGeometryXML,std::string());

    virtual void getLinePlot(const Mantid::Kernel::VMD & , const Mantid::Kernel::VMD & ,
        Mantid::API::MDNormalization , std::vector<Mantid::coord_t> & , std::vector<Mantid::signal_t> & , std::vector<Mantid::signal_t> & ) const
    {}

    virtual uint64_t getNPoints() const
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
  Mantid::Geometry::MDImplicitFunction_sptr impFunction(new MockImplicitFunction);
  generator.setImplicitFunction(impFunction);
  TSM_ASSERT_THROWS("Cannot generate the xml without the workspace", generator.createXMLString(), std::runtime_error);
}

void testNoLocationDoesNotThrow()
{
  MockIMDWorkspace* pWorkspace = new MockIMDWorkspace;
  pWorkspace->setName("someName");
  Mantid::API::IMDWorkspace_sptr workspace(pWorkspace);

  MockImplicitFunction* pImpFunction = new MockImplicitFunction;
  EXPECT_CALL(*pImpFunction, toXMLString()).Times(1).WillRepeatedly(testing::Return("<ImplicitFunction/>"));
  Mantid::Geometry::MDImplicitFunction_sptr impFunction(pImpFunction);
  
  RebinningKnowledgeSerializer generator(LocationNotRequired); //Location is not required.
  generator.setImplicitFunction(impFunction);
  generator.setWorkspace(workspace);

  TSM_ASSERT_THROWS_NOTHING("The location is not mandatory, should not throw", generator.createXMLString());
}

void testNoNameThrows()
{
  Mantid::Geometry::MDImplicitFunction_sptr impFunction(new MockImplicitFunction);
  MockIMDWorkspace* pWorkspace = new MockIMDWorkspace;
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

  boost::shared_ptr<const Mantid::API::IMDWorkspace> workspace(pWorkspace);
  Mantid::Geometry::MDImplicitFunction_sptr impFunction(pImpFunction);
  RebinningKnowledgeSerializer generator;

  //Apply setters.
  generator.setImplicitFunction(impFunction);
  generator.setWorkspace(workspace);

  std::string xml = generator.createXMLString();

  TSM_ASSERT_EQUALS("The xml has been created, but is incorrect.", "<MDInstruction><MDWorkspaceName>name</MDWorkspaceName><MDWorkspaceLocation></MDWorkspaceLocation>"
      + workspace->getGeometryXML() + "<ImplicitFunction/></MDInstruction>" ,xml)
}

void testCreateXMLWithComponents() //Uses individual setters for geometry, location and name.
{
  MockImplicitFunction* pImpFunction = new MockImplicitFunction;
  EXPECT_CALL(*pImpFunction, toXMLString()).Times(1).WillRepeatedly(testing::Return("<ImplicitFunction/>"));
  Mantid::Geometry::MDImplicitFunction_sptr impFunction(pImpFunction);

  RebinningKnowledgeSerializer generator;
  //Apply setters.
  generator.setImplicitFunction(impFunction);
  generator.setWorkspaceName("name");
  generator.setGeometryXML("<DimensionSet/>");

  std::string xml = generator.createXMLString();

  TSM_ASSERT_EQUALS("The xml has been created, but is incorrect.", "<MDInstruction><MDWorkspaceName>name</MDWorkspaceName><DimensionSet/><ImplicitFunction/></MDInstruction>" ,xml)
}

void testCreateXMLWithoutFunction()
{
  RebinningKnowledgeSerializer generator;
  //Apply setters.
  generator.setWorkspaceName("name");
  generator.setGeometryXML("<DimensionSet/>");

  std::string xml = generator.createXMLString();
  TSM_ASSERT_EQUALS("The xml has been created without a function incorrectly", "<MDInstruction><MDWorkspaceName>name</MDWorkspaceName><DimensionSet/></MDInstruction>", xml);
}

void testGetGeometryXML()
{
  RebinningKnowledgeSerializer generator;
  generator.setWorkspaceName("name");
  std::string dimensionXMLString = "<DimensionSet/>";
  generator.setGeometryXML(dimensionXMLString);

  std::string xml = generator.getWorkspaceGeometry();
  TSM_ASSERT_EQUALS("The geometry xml fetched is not the same as that provided", dimensionXMLString, generator.getWorkspaceGeometry());
}

void testHasFunction()
{
  RebinningKnowledgeSerializer withoutFunction;
  RebinningKnowledgeSerializer withFunction;
  Mantid::Geometry::MDImplicitFunction_sptr impFunction(new MockImplicitFunction);
  withFunction.setImplicitFunction(impFunction);

  TSM_ASSERT_EQUALS("A function has not been provided. ::hasFunctionInfo() should return false.", false,  withoutFunction.hasFunctionInfo());
  TSM_ASSERT_EQUALS("A function has been provided. ::hasFunctionInfo() should return true.", true, withFunction.hasFunctionInfo());
}

void testHasGeometryInfoWithoutGeometry()
{
  //Note that functions do not apply to this test set.
  RebinningKnowledgeSerializer withoutGeometry;
  withoutGeometry.setWorkspaceName("-");
  TSM_ASSERT_EQUALS("No Geometry provided. ::hasGeometryInfo() should return false.", false, withoutGeometry.hasGeometryInfo());
}

void testHasGeometryInfoWithoutWSName()
{
  RebinningKnowledgeSerializer withoutWSName;
  withoutWSName.setGeometryXML("-");
  TSM_ASSERT_EQUALS("No WS name provided. ::hasGeometryInfo() should return false.", false, withoutWSName.hasGeometryInfo());
}

void testHasGeometryAndWSInfo()
{
  RebinningKnowledgeSerializer withFullGeometryAndWSInfo;
  withFullGeometryAndWSInfo.setGeometryXML("-");
  withFullGeometryAndWSInfo.setWorkspaceName("-");
  TSM_ASSERT_EQUALS("All geometry and ws information has been provided. ::hasGeometryInfo() should return true.", true, withFullGeometryAndWSInfo.hasGeometryInfo());
}


};

#endif
