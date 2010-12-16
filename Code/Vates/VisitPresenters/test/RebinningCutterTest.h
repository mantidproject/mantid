#ifndef REBINNING_CUTTER_TEST_H_
#define REBINNING_CUTTER_TEST_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cxxtest/TestSuite.h>
#include <boost/shared_ptr.hpp>
#include <cmath>
#include <typeinfo>

#include "MantidGeometry/MDGeometry/MDDimensionRes.h"
#include "MantidMDAlgorithms/CompositeImplicitFunction.h"
#include "MantidMDAlgorithms/BoxImplicitFunction.h"
#include "MantidAPI/Point3D.h"
#include <vtkFieldData.h>
#include <vtkCharArray.h>
#include <vtkDataSet.h>
#include <vtkUnstructuredGrid.h>
#include "MantidVisitPresenters/RebinningCutterPresenter.h"
#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>

class RebinningCutterTest: public CxxTest::TestSuite
{
private:

  class MockClipper: public Mantid::VATES::Clipper
  {
  public:
  MOCK_METHOD1(SetInput, void(vtkDataSet* in_ds));
  MOCK_METHOD1(SetClipFunction, void(vtkImplicitFunction* func));
  MOCK_METHOD1(SetInsideOut, void(bool insideout));
  MOCK_METHOD1(SetRemoveWholeCells, void(bool removeWholeCells));
  MOCK_METHOD1(SetOutput, void(vtkUnstructuredGrid* out_ds));
  MOCK_METHOD0(Update, void());
  MOCK_METHOD0(Delete,void());
};

class PsudoFilter
{
private:
  std::vector<double> m_normal;
  std::vector<double> m_origin;

public:
  PsudoFilter(std::vector<double> normal, std::vector<double> origin)
  : m_origin(origin),
    m_normal(normal)
  {
  }

  vtkDataSet* Execute(Mantid::VATES::Clipper* clipper, vtkDataSet* in_ds)
  {
    using namespace Mantid::VATES;
    using namespace Mantid::Geometry;
    RebinningCutterPresenter presenter(in_ds);

    DimensionVec vec;
    Dimension_sptr dimX = Dimension_sptr(new MDDimensionRes("1", q1));
    Dimension_sptr dimY = Dimension_sptr(new MDDimensionRes("2", q2));
    Dimension_sptr dimZ = Dimension_sptr(new MDDimensionRes("3", q3));
    Dimension_sptr dimT = Dimension_sptr(new MDDimension("4"));

    vec.push_back(dimX);
    vec.push_back(dimY);
    vec.push_back(dimZ);
    vec.push_back(dimT);

    std::vector<double> origin;
    origin.push_back(1);
    origin.push_back(1);
    origin.push_back(1);

    presenter.constructReductionKnowledge(vec, dimX, dimY, dimZ, dimT, 1, 2, 3, origin);

    vtkUnstructuredGrid *ug = presenter.applyReductionKnowledge(clipper);

//    std::cout << "---- Ouput --- " << std::endl;
//    std::cout << presenter.getFunction()->toXMLString() << std::endl;
//    std::cout << "---- Ouput End --- " << std::endl;

    in_ds->Delete();
    return ug;
  }
};

//helper method;
std::string getXMLInstructions()
{
  return std::string("<Function><Type>BoxImplicitFunction</Type><ParameterList><Parameter><Type>WidthParameter</Type><Value>1.0000</Value></Parameter><Parameter><Type>DepthParameter</Type><Value>3.0000</Value></Parameter><Parameter><Type>HeightParameter</Type><Value>2.0000</Value></Parameter><Parameter><Type>OriginParameter</Type><Value>2.0000, 3.0000, 4.0000</Value></Parameter></ParameterList></Function>");
}

//helper method
std::string getComplexXMLInstructions()
{
  return std::string("<MDInstruction>"
      "<MDWorkspaceName>name</MDWorkspaceName>"
      "<MDWorkspaceLocation>location</MDWorkspaceLocation>"
      "<Function>"
		  "<Type>CompositeImplicitFunction</Type>"
		  "<ParameterList/>"
		  "<Function>"
		  "<Type>BoxImplicitFunction</Type>"
		  "<ParameterList>"
      "<Parameter>"
      "<Type>WidthParameter</Type>"
      "<Value>1.0000</Value>"
      "</Parameter>"
      "<Parameter>"
      "<Type>HeightParameter</Type>"
      "<Value>2.0000</Value>"
      "</Parameter>"
      "<Parameter>"
      "<Type>DepthParameter</Type>"
      "<Value>3.0000</Value>"
      "</Parameter>"
      "<Parameter>"
		  "<Type>OriginParameter</Type>"
		  "<Value>1.0000, 0.0000, 1.0000</Value>"
		  "</Parameter>"
		  "</ParameterList>"
		  "</Function>"
		  "<Function>"
		  "<Type>CompositeImplicitFunction</Type>"
		  "<ParameterList></ParameterList>"
		  "<Function>"
		  "<Type>BoxImplicitFunction</Type>"
		  "<ParameterList>"
		  "<Parameter>"
		  "<Type>WidthParameter</Type>"
		  "<Value>1.0000</Value>"
		  "</Parameter>"
		  "<Parameter>"
		  "<Type>HeightParameter</Type>"
		  "<Value>2.0000</Value>"
		  "</Parameter>"
      "<Parameter>"
      "<Type>DepthParameter</Type>"
      "<Value>3.0000</Value>"
      "</Parameter>"
      "<Parameter>"
      "<Type>OriginParameter</Type>"
      "<Value>0.0000, 0.0000, 0.0000</Value>"
      "</Parameter>"
		  "</ParameterList>"
		  "</Function>"
		  "</Function>"
		  "</Function></MDInstruction>");
}

//helper method
std::string convertCharArrayToString(vtkCharArray* carry)
{
  std::string sResult;
  for (int i = 0; i < carry->GetSize(); i++)
  {
    char c = carry->GetValue(i);
    if(int(c) > 1)
    {
      sResult.push_back(c);
    }
  }
  boost::trim(sResult);
  return sResult;
}

//helper method
vtkFieldData* createFieldDataWithCharArray(std::string testData, std::string id)
{
  vtkFieldData* fieldData = vtkFieldData::New();
  vtkCharArray* charArray = vtkCharArray::New();
  charArray->SetName(id.c_str());
  charArray->Allocate(100);
  for(int i = 0; i < testData.size(); i++)
  {
    char cNextVal = testData.at(i);
    if(int(cNextVal) > 1)
    {
      charArray->InsertNextValue(cNextVal);

    }
  }
  fieldData->AddArray(charArray);
  return fieldData;
}

//Helper method to construct a dataset identical to what would be expected as the input to a RebinningCutterFilter without the any geometric/topological data.
vtkDataSet* constructInputDataSet()
{
  vtkDataSet* dataset = vtkUnstructuredGrid::New();
  std::string id = "1";
  dataset->SetFieldData(createFieldDataWithCharArray(getComplexXMLInstructions(), id.c_str()));
  return dataset;
}

public:

void testInChainedFilterSchenario()
{
  using namespace Mantid::VATES;

  //Create an input dataset with the field data.
  vtkDataSet* in_ds = constructInputDataSet();


  MockClipper* clipper = new MockClipper;
  EXPECT_CALL(*clipper, SetInput(testing::_)).Times(testing::AtLeast(1));
  EXPECT_CALL(*clipper, SetClipFunction(testing::_)).Times(testing::AtLeast(1));
  EXPECT_CALL(*clipper, SetInsideOut(true)).Times(testing::AtLeast(1));
  EXPECT_CALL(*clipper, SetRemoveWholeCells(true)).Times(testing::AtLeast(1));
  EXPECT_CALL(*clipper, SetOutput(testing::_)).Times(testing::AtLeast(1));
  EXPECT_CALL(*clipper, Update()).Times(testing::AtLeast(1));
  boost::scoped_ptr<MockClipper> smrtClipper(clipper); //Exception safety during stack unwinding.

  PsudoFilter a(std::vector<double>(3,1),std::vector<double>(3,1));
  PsudoFilter b(std::vector<double>(3,2),std::vector<double>(3,2));
  PsudoFilter c(std::vector<double>(3,3),std::vector<double>(3,3));

  vtkDataSet* out_ds = c.Execute(clipper, b.Execute(clipper, a.Execute(clipper, in_ds)));
}

void testgetMetaDataID()
{
  using namespace Mantid::VATES;
  TSM_ASSERT_EQUALS("The expected id for the slicing metadata was not found", "1", std::string(getMetadataID()));
}


void testMetaDataToFieldData()
{
  using namespace Mantid::VATES;

  std::string testData = "<test data/>%s";
  std::string id = "1";

  vtkFieldData* fieldData = vtkFieldData::New();
  vtkCharArray* charArray = vtkCharArray::New();
  charArray->SetName(id.c_str());
  fieldData->AddArray(charArray);

  metaDataToFieldData(fieldData, testData.c_str() , id.c_str() );

  //convert vtkchararray back into a string.
  vtkCharArray* carry = dynamic_cast<vtkCharArray*>(fieldData->GetArray(id.c_str()));


  TSM_ASSERT_EQUALS("The result does not match the input. Metadata not properly converted.", testData, convertCharArrayToString(carry));
  fieldData->Delete();
}

void testMetaDataToFieldDataWithEmptyFieldData()
{
  using namespace Mantid::VATES;

  std::string testData = "<test data/>%s";
  std::string id = "1";

  vtkFieldData* emptyFieldData = vtkFieldData::New();
  metaDataToFieldData(emptyFieldData, testData.c_str() , id.c_str() );

  //convert vtkchararray back into a string.
  vtkCharArray* carry = dynamic_cast<vtkCharArray*>(emptyFieldData->GetArray(id.c_str()));

  TSM_ASSERT_EQUALS("The result does not match the input. Metadata not properly converted.", testData, convertCharArrayToString(carry));
  emptyFieldData->Delete();
}

void testFieldDataToMetaData()
{
  using namespace Mantid::VATES;

  std::string testData = "test data";
  std::string id = "1";

  vtkFieldData* fieldData = createFieldDataWithCharArray(testData, id);

  std::string metaData = fieldDataToMetaData(fieldData, id.c_str());
  TSM_ASSERT_EQUALS("The result does not match the input. Field data not properly converted.", testData, metaData);
  fieldData->Delete();
}

void testFindExistingRebinningDefinitions()
{

  using namespace Mantid::VATES;
  using namespace Mantid::API;
  using namespace Mantid::MDAlgorithms;
  std::string id = "1";
  vtkDataSet* dataset = constructInputDataSet();

  ImplicitFunction* func = findExistingRebinningDefinitions(dataset, id.c_str());

  TSM_ASSERT("There was a previous definition of a function that should have been recognised and generated."
      , CompositeImplicitFunction::functionName() == func->getName());

  delete func;
}


void testNoExistingRebinningDefinitions()
{
  using namespace Mantid::VATES;
  using namespace Mantid::API;

  vtkDataSet* dataset = vtkUnstructuredGrid::New();

  ImplicitFunction* func = findExistingRebinningDefinitions(dataset, "1");

  TSM_ASSERT("There were no previous definitions carried through.", NULL == func);
}


void testConstructionWithoutValidOriginThrows()
{
  using namespace Mantid::VATES;
  using namespace Mantid::Geometry;
  RebinningCutterPresenter presenter(vtkUnstructuredGrid::New());

  DimensionVec vec;
  Dimension_sptr dimX = Dimension_sptr(new MDDimension("1"));
  Dimension_sptr dimY = Dimension_sptr(new MDDimension("2"));
  Dimension_sptr dimZ = Dimension_sptr(new MDDimension("3"));
  Dimension_sptr dimT = Dimension_sptr(new MDDimension("4"));

  vec.push_back(dimX);
  vec.push_back(dimY);
  vec.push_back(dimZ);
  vec.push_back(dimT);

  std::vector<double> badOrigin;

  TSM_ASSERT_THROWS("The origin vector is the wrong size. Should have thrown.",
      presenter.constructReductionKnowledge(vec, dimX, dimY, dimZ, dimT, 1, 2, 3, badOrigin), std::invalid_argument);
}

void testApplyReductionThrows()
{
    using namespace Mantid::VATES;
    using namespace Mantid::API;
    using namespace Mantid::MDAlgorithms;

    MockClipper* clipper = new MockClipper;
    EXPECT_CALL(*clipper, SetInput(testing::_)).Times(0);
    EXPECT_CALL(*clipper, SetClipFunction(testing::_)).Times(0);
    EXPECT_CALL(*clipper, SetInsideOut(true)).Times(0);
    EXPECT_CALL(*clipper, SetRemoveWholeCells(true)).Times(0);
    EXPECT_CALL(*clipper, SetOutput(testing::_)).Times(0);
    EXPECT_CALL(*clipper, Update()).Times(0);

    Mantid::VATES::RebinningCutterPresenter presenter(vtkUnstructuredGrid::New());

    TSM_ASSERT_THROWS("Should have thrown if constructReductionKnowledge not called first.", presenter.applyReductionKnowledge(clipper), std::runtime_error);
    delete clipper;
}

void testFindWorkspaceName()
{
  using namespace Mantid::VATES;
  std::string id = "1";
  vtkDataSet* dataset = constructInputDataSet();

  std::string name = findExistingWorkspaceNameFromXML(dataset, id.c_str());

  TSM_ASSERT_EQUALS("The workspace name is different from the xml value.", "name", name );
}

void testFindWorkspaceLocation()
{
  using namespace Mantid::VATES;
  std::string id = "1";
  vtkDataSet* dataset = constructInputDataSet();

  std::string location = findExistingWorkspaceLocationFromXML(dataset, id.c_str());

  TSM_ASSERT_EQUALS("The workspace location is differrent from the xml value.", "location", location);
}

void testFindWorkspaceNameThrows()
{
}

void testFindWorkspaceLocationThrows()
{
}



};


#endif 
