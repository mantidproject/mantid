#ifndef REBINNING_CUTTER_TEST_H_
#define REBINNING_CUTTER_TEST_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cxxtest/TestSuite.h>
#include <boost/shared_ptr.hpp>
#include <cmath>
#include <typeinfo>

#include "MantidMDAlgorithms/CompositeImplicitFunction.h"
#include "MantidMDAlgorithms/PlaneImplicitFunction.h"
#include "MantidAPI/Point3D.h"
#include <vtkFieldData.h>
#include <vtkCharArray.h>
#include <vtkDataSet.h>
#include <vtkUnstructuredGrid.h>
#include "InterfaceVatesMantid.h"
#include <boost/algorithm/string.hpp>

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

//helper method;
std::string getXMLInstructions()
{
  return std::string("<Function><Type>PlaneImplicitFunction</Type><ParameterList><Parameter><Type>NormalParameter</Type><Value>1.0000, 1.0000, 1.0000</Value></Parameter><Parameter><Type>OriginParameter</Type><Value>2.0000, 3.0000, 4.0000</Value></Parameter></ParameterList></Function>");
}

//helper method
std::string getComplexXMLInstructions()
{
  return std::string("<Function><Type>CompositeImplicitFunction</Type><Function><Type>PlaneImplicitFunction</Type><ParameterList><Parameter><Type>NormalParameter</Type><Value>0.0000, 1.0000, 1.0000</Value></Parameter><Parameter><Type>OriginParameter</Type><Value>0.0000, 0.0000, 0.0000</Value></Parameter></ParameterList></Function><Function><Type>CompositeImplicitFunction</Type><Function><Type>PlaneImplicitFunction</Type><ParameterList><Parameter><Type>NormalParameter</Type><Value>0.0000, 0.0000, -1.0000</Value></Parameter><Parameter><Type>OriginParameter</Type><Value>0.0000, 0.0000, 0.0000</Value></Parameter></ParameterList></Function></Function></Function>");
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
public:

void testgetMetaDataID()
{
  using namespace Mantid::VATES;
  RebinningCutterPresenter presenter;
  TSM_ASSERT_EQUALS("The expected id for the slicing metadata was not found", "1", std::string(presenter.getMetadataID()));
}

//construction should yeild a composite function at the top level, with a single plane.
//Hierachy with existing/previous functions nested below. top.
void testConstructionGivesComposite()
{
  using namespace Mantid::VATES;
  using namespace Mantid::API;
  using namespace Mantid::MDAlgorithms;

  std::vector<double> origin;
  std::vector<double> normal;
  normal.push_back(1);
  normal.push_back(1);
  normal.push_back(1);
  origin.push_back(1);
  origin.push_back(1);
  origin.push_back(1);

  vtkDataSet* dataset = vtkUnstructuredGrid::New();
  std::string id = "1";
  vtkFieldData* fieldData = createFieldDataWithCharArray(getXMLInstructions(), id.c_str());
  dataset->SetFieldData(fieldData);

  RebinningCutterPresenter presenter;
  ImplicitFunction* func = presenter.constructReductionKnowledge(dataset, normal, origin, id.c_str());

  CompositeImplicitFunction* compFunction = dynamic_cast<CompositeImplicitFunction*>(func);
  TSM_ASSERT("Composite Functions should always be generated as the root.", NULL != func);
  TSM_ASSERT_EQUALS("Two sub-functions should exist on this composite", 2, compFunction->getNFunctions());

  dataset->Delete();
  fieldData->Delete();
  delete compFunction;
}


void testMetaDataToFieldData()
{
  using namespace Mantid::VATES;
  RebinningCutterPresenter presenter;

  std::string testData = "<test data/>%s";
  std::string id = "1";

  vtkFieldData* fieldData = vtkFieldData::New();
  vtkCharArray* charArray = vtkCharArray::New();
  charArray->SetName(id.c_str());
  fieldData->AddArray(charArray);

  presenter.metaDataToFieldData(fieldData, testData.c_str() , id.c_str() );

  //convert vtkchararray back into a string.
  vtkCharArray* carry = dynamic_cast<vtkCharArray*>(fieldData->GetArray(id.c_str()));


  TSM_ASSERT_EQUALS("The result does not match the input. Metadata not properly converted.", testData, convertCharArrayToString(carry));
  fieldData->Delete();
}

void testMetaDataToFieldDataWithEmptyFieldData()
{
  using namespace Mantid::VATES;
  RebinningCutterPresenter presenter;

  std::string testData = "<test data/>%s";
  std::string id = "1";

  vtkFieldData* emptyFieldData = vtkFieldData::New();
  presenter.metaDataToFieldData(emptyFieldData, testData.c_str() , id.c_str() );

  //convert vtkchararray back into a string.
  vtkCharArray* carry = dynamic_cast<vtkCharArray*>(emptyFieldData->GetArray(id.c_str()));

  TSM_ASSERT_EQUALS("The result does not match the input. Metadata not properly converted.", testData, convertCharArrayToString(carry));
  emptyFieldData->Delete();
}

void testFieldDataToMetaData()
{
  using namespace Mantid::VATES;
  RebinningCutterPresenter presenter;

  std::string testData = "test data";
  std::string id = "1";

  vtkFieldData* fieldData = createFieldDataWithCharArray(testData, id);

  std::string metaData = presenter.fieldDataToMetaData(fieldData, id.c_str());
  TSM_ASSERT_EQUALS("The result does not match the input. Field data not properly converted.", testData, metaData);
  fieldData->Delete();
}

void testFindExistingRebinningDefinitions()
{
  using namespace Mantid::VATES;
  using namespace Mantid::API;
  using namespace Mantid::MDAlgorithms;
  vtkDataSet* dataset = vtkUnstructuredGrid::New();
  std::string id = "1";
  dataset->SetFieldData(createFieldDataWithCharArray(getComplexXMLInstructions(), id.c_str()));
  RebinningCutterPresenter presenter;
  ImplicitFunction* func = presenter.findExistingRebinningDefinitions(dataset, id.c_str());


  TSM_ASSERT("There was a previous definition of a plane that should have been recognised and generated."
      , CompositeImplicitFunction::functionName() == func->getName());
}


void testNoExistingRebinningDefinitions()
{
  using namespace Mantid::VATES;
  using namespace Mantid::API;

  vtkDataSet* dataset = vtkUnstructuredGrid::New();
  RebinningCutterPresenter presenter;
  ImplicitFunction* func = presenter.findExistingRebinningDefinitions(dataset, "1");

  TSM_ASSERT("There were no previous definitions carried through.", NULL == func);
}

void testConstructWithoutValidNormalThrows()
{
  using namespace Mantid::VATES;

    RebinningCutterPresenter presenter;
    std::vector<double> badNormal;
    std::vector<double> goodOrigin;
    goodOrigin.push_back(1);
    goodOrigin.push_back(1);
    goodOrigin.push_back(1);
    TSM_ASSERT_THROWS("The normal vector is the wrong size. Should have thrown.",
        presenter.constructReductionKnowledge(vtkUnstructuredGrid::New(), badNormal, goodOrigin,  "1"), std::invalid_argument);
}

void testPersistance()
{
  using namespace Mantid::VATES;
  using namespace Mantid::API;
  using namespace Mantid::MDAlgorithms;

  std::string id = "1";
  CompositeImplicitFunction* compFunc = new CompositeImplicitFunction;
  NormalParameter n1(1, 2, 3);
  NormalParameter n2(4, 5, 6);
  OriginParameter o1(7, 8, 9);
  OriginParameter o2(10, 11, 12);
  PlaneImplicitFunction* planeFuncA = new PlaneImplicitFunction(n1, o1);
  PlaneImplicitFunction* planeFuncB = new PlaneImplicitFunction(n2, o2);
  compFunc->addFunction(boost::shared_ptr<ImplicitFunction>(planeFuncA));
  compFunc->addFunction(boost::shared_ptr<ImplicitFunction>(planeFuncB));

  Mantid::VATES::RebinningCutterPresenter presenter;
  vtkUnstructuredGrid* outdataset = vtkUnstructuredGrid::New();
  presenter.persistReductionKnowledge(outdataset, compFunc,id.c_str());

  vtkFieldData* writtenFieldData = outdataset->GetFieldData();
  vtkDataArray* dataArry = writtenFieldData->GetArray(id.c_str());
  vtkCharArray* charArry = dynamic_cast<vtkCharArray*>(dataArry);
  std::string xmlString = convertCharArrayToString(charArry);
  TSM_ASSERT_EQUALS("The persistence of the reduction knowledge into the output dataset has not worked correctly.",
      "<Function><Type>CompositeImplicitFunction</Type><Function><Type>PlaneImplicitFunction</Type><ParameterList><Parameter><Type>NormalParameter</Type><Value>1.0000, 2.0000, 3.0000</Value></Parameter><Parameter><Type>OriginParameter</Type><Value>7.0000, 8.0000, 9.0000</Value></Parameter></ParameterList></Function><Function><Type>PlaneImplicitFunction</Type><ParameterList><Parameter><Type>NormalParameter</Type><Value>4.0000, 5.0000, 6.0000</Value></Parameter><Parameter><Type>OriginParameter</Type><Value>10.0000, 11.0000, 12.0000</Value></Parameter></ParameterList></Function></Function>"
      ,xmlString);
  delete compFunc;
  outdataset->Delete();
}

void testConstructionWithoutValidOriginThrows()
{
  using namespace Mantid::VATES;
  RebinningCutterPresenter presenter;
      std::vector<double> badOrigin;
      std::vector<double> goodNormal;
      goodNormal.push_back(1);
      goodNormal.push_back(1);
      goodNormal.push_back(1);
      TSM_ASSERT_THROWS("The origin vector is the wrong size. Should have thrown.",
          presenter.constructReductionKnowledge(vtkUnstructuredGrid::New(), goodNormal, badOrigin,  "1"), std::invalid_argument);
  }

void testApplyReduction()
{
    using namespace Mantid::VATES;
    using namespace Mantid::API;
    using namespace Mantid::MDAlgorithms;

    MockClipper* clipper = new MockClipper;
    EXPECT_CALL(*clipper, SetInput(testing::_)).Times(2);
    EXPECT_CALL(*clipper, SetClipFunction(testing::_)).Times(2);
    EXPECT_CALL(*clipper, SetInsideOut(true)).Times(2);
    EXPECT_CALL(*clipper, SetRemoveWholeCells(true)).Times(2);
    EXPECT_CALL(*clipper, SetOutput(testing::_)).Times(2);
    EXPECT_CALL(*clipper, Update()).Times(2);

    CompositeImplicitFunction* compFunc = new CompositeImplicitFunction;
    NormalParameter n(1, 1, 1);
    OriginParameter o(1, 1, 1);
    PlaneImplicitFunction* planeFuncA = new PlaneImplicitFunction(n, o);
    PlaneImplicitFunction* planeFuncB = new PlaneImplicitFunction(n, o);
    compFunc->addFunction(boost::shared_ptr<ImplicitFunction>(planeFuncA));
    compFunc->addFunction(boost::shared_ptr<ImplicitFunction>(planeFuncB));

    Mantid::VATES::RebinningCutterPresenter presenter;
    vtkDataSet* dataset = vtkUnstructuredGrid::New();
    presenter.applyReductionKnowledge(clipper, dataset, compFunc);
    TSM_ASSERT("The clipper has not been used as expected.", testing::Mock::VerifyAndClearExpectations(clipper));

    delete compFunc;
    delete clipper;
    dataset->Delete();
}

void testConstructionGivesPlaneBasedOnInputs()
{
  using namespace Mantid::VATES;
  using namespace Mantid::API;
  using namespace Mantid::MDAlgorithms;

  std::vector<double> origin;
  std::vector<double> normal;
  normal.push_back(1);
  normal.push_back(2);
  normal.push_back(3);
  origin.push_back(4);
  origin.push_back(5);
  origin.push_back(6);

  vtkDataSet* dataset = vtkUnstructuredGrid::New();
  std::string id = "1";

  RebinningCutterPresenter presenter;
  ImplicitFunction* func = presenter.constructReductionKnowledge(dataset, normal, origin, id.c_str());

  CompositeImplicitFunction* compFunction = dynamic_cast<CompositeImplicitFunction*>(func);
  PlaneImplicitFunction* planeFunction =  dynamic_cast<PlaneImplicitFunction*>(compFunction->getFunctions().at(0).get());
  TSM_ASSERT_EQUALS("Only a single nested function should be present.", 1, compFunction->getNFunctions());
  TSM_ASSERT_EQUALS("Origin x miss-match. Plane function has not been correctly created from inputs.", origin.at(0), planeFunction->getOriginX());
  TSM_ASSERT_EQUALS("Origin y miss-match. Plane function has not been correctly created from inputs.", origin.at(1), planeFunction->getOriginY());
  TSM_ASSERT_EQUALS("Origin z miss-match. Plane function has not been correctly created from inputs.", origin.at(2), planeFunction->getOriginZ());
  TSM_ASSERT_EQUALS("Normal x miss-match. Plane function has not been correctly created from inputs.", normal.at(0), planeFunction->getNormalX());
  TSM_ASSERT_EQUALS("Normal y miss-match. Plane function has not been correctly created from inputs.", normal.at(1), planeFunction->getNormalY());
  TSM_ASSERT_EQUALS("Normal z miss-match. Plane function has not been correctly created from inputs.", normal.at(2), planeFunction->getNormalZ());
  delete compFunction;
  dataset->Delete();
}



};


#endif 
