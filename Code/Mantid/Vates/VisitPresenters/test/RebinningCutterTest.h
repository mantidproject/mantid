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
#include <vtkDataSet.h>
#include "MantidVisitPresenters/RebinningCutterPresenter.h"
#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>

using namespace Mantid::VATES;

class RebinningCutterTest: public CxxTest::TestSuite
{

private:

  class PsudoFilter
  {
  private:

    std::vector<double> m_origin;

  public:
    PsudoFilter(std::vector<double> origin) :
      m_origin(origin)
    {
    }

    vtkDataSet* Execute(vtkDataSet* in_ds)
    {
      using namespace Mantid::VATES;
      using namespace Mantid::Geometry;
      using Mantid::VATES::DimensionVec;
      using Mantid::VATES::Dimension_sptr;
      RebinningCutterPresenter presenter(in_ds, 1);

      DimensionVec vec;
      MDDimensionRes* pDimQx = new MDDimensionRes("qx", q1); //In reality these commands come from UI inputs.
      pDimQx->setRange(-1.5, 5, 5);
      Dimension_sptr dimX = Dimension_sptr(pDimQx);

      MDDimensionRes* pDimQy = new MDDimensionRes("qy", q2); //In reality these commands come from UI inputs.
      pDimQy->setRange(-6.6, 6.6, 5);
      Dimension_sptr dimY = Dimension_sptr(pDimQy);

      MDDimensionRes* pDimQz = new MDDimensionRes("qz", q3); //In reality these commands come from UI inputs.
      pDimQz->setRange(-6.6, 6.6, 5);
      Dimension_sptr dimZ = Dimension_sptr(pDimQz);

      MDDimension* pDimEn = new MDDimension("en");
      pDimEn->setRange(0, 150, 5);
      Dimension_sptr dimT = Dimension_sptr(pDimEn);

      vec.push_back(dimX);
      vec.push_back(dimY);
      vec.push_back(dimZ);
      vec.push_back(dimT);

      presenter.constructReductionKnowledge(vec, dimX, dimY, dimZ, dimT, 1, 2, 3, m_origin);

      vtkDataSet *ug = presenter.applyReductionKnowledge("signal", false);

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
    return std::string("<?xml version=\"1.0\" encoding=\"utf-8\"?>") +
"<MDInstruction>" +
  "<MDWorkspaceName>Input</MDWorkspaceName>" +
  "<MDWorkspaceLocation>fe_demo.sqw</MDWorkspaceLocation>" +
  "<DimensionSet>" +
    "<Dimension ID=\"en\">" +
      "<Name>Energy</Name>" +
      "<UpperBounds>150</UpperBounds>" +
      "<LowerBounds>0</LowerBounds>" +
      "<NumberOfBins>5</NumberOfBins>" +
    "</Dimension>" +
    "<Dimension ID=\"qx\">" +
      "<Name>Qx</Name>" +
      "<UpperBounds>5</UpperBounds>" +
      "<LowerBounds>-1.5</LowerBounds>" +
      "<NumberOfBins>5</NumberOfBins>" +
      "<ReciprocalDimensionMapping>q1</ReciprocalDimensionMapping>" +
    "</Dimension>" +
    "<Dimension ID=\"qy\">" +
      "<Name>Qy</Name>" +
      "<UpperBounds>6.6</UpperBounds>" +
      "<LowerBounds>-6.6</LowerBounds>" +
      "<NumberOfBins>5</NumberOfBins>" +
      "<ReciprocalDimensionMapping>q2</ReciprocalDimensionMapping>" +
    "</Dimension>" +
    "<Dimension ID=\"qz\">" +
      "<Name>Qz</Name>" +
      "<UpperBounds>6.6</UpperBounds>" +
      "<LowerBounds>-6.6</LowerBounds>" +
      "<NumberOfBins>5</NumberOfBins>" +
      "<ReciprocalDimensionMapping>q3</ReciprocalDimensionMapping>" +
    "</Dimension>" +
    "<XDimension>" +
      "<RefDimensionId>qx</RefDimensionId>" +
    "</XDimension>" +
    "<YDimension>" +
      "<RefDimensionId>qy</RefDimensionId>" +
    "</YDimension>" +
    "<ZDimension>" +
      "<RefDimensionId>qz</RefDimensionId>" +
    "</ZDimension>" +
    "<TDimension>" +
      "<RefDimensionId>en</RefDimensionId>" +
    "</TDimension>" +
  "</DimensionSet>" +
  "<Function>" +
    "<Type>CompositeImplicitFunction</Type>" +
    "<ParameterList/>" +
    "<Function>" +
      "<Type>BoxImplicitFunction</Type>" +
      "<ParameterList>" +
        "<Parameter>" +
          "<Type>HeightParameter</Type>" +
          "<Value>6</Value>" +
       "</Parameter>" +
        "<Parameter>" +
          "<Type>WidthParameter</Type>" +
          "<Value>1.5</Value>" +
       "</Parameter>" +
       "<Parameter>" +
          "<Type>DepthParameter</Type>" +
          "<Value>6</Value>" +
       "</Parameter>" +
        "<Parameter>" +
          "<Type>OriginParameter</Type>" +
          "<Value>0, 0, 0</Value>" +
        "</Parameter>" +
      "</ParameterList>" +
    "</Function>" +
    "<Function>" +
          "<Type>CompositeImplicitFunction</Type>" +
          "<ParameterList/>" +
            "<Function>" +
              "<Type>BoxImplicitFunction</Type>" +
              "<ParameterList>" +
                "<Parameter>" +
                  "<Type>WidthParameter</Type>" +
                  "<Value>4</Value>" +
                "</Parameter>" +
                "<Parameter>" +
                  "<Type>HeightParameter</Type>" +
                  "<Value>1.5</Value>" +
               "</Parameter>" +
               "<Parameter>" +
                  "<Type>DepthParameter</Type>" +
                  "<Value>6</Value>" +
               "</Parameter>" +
               "<Parameter>" +
                  "<Type>OriginParameter</Type>" +
                  "<Value>0, 0, 0</Value>" +
               "</Parameter>" +
              "</ParameterList>" +
            "</Function>" +
      "</Function>" +
  "</Function>" +
"</MDInstruction>";
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
  std::string id =  RebinningCutterPresenter::metaDataId;
  dataset->SetFieldData(createFieldDataWithCharArray(getComplexXMLInstructions(), id.c_str()));
  return dataset;
}

public:

//Simple schenario testing end-to-end working of this presenter.
void testExecution()
{
  std::string input;
  std::cin >> input;
  //Create an input dataset with the field data.
    vtkDataSet* in_ds = constructInputDataSet();

    PsudoFilter filter(std::vector<double>(3,0));

    vtkDataSet* out_ds = filter.Execute(in_ds);

    //NB 125 = 5 * 5 * 5 see pseudo filter execution method's number of bins above.
    TSM_ASSERT_EQUALS("An empty visualisation data set has been generated.", out_ds->GetNumberOfPoints() , 125);

}

//A more complex version of the above testExecution. Uses filter chaining as would occur in real pipeline.
void testExecutionInChainedSchenario()
{
  //Create an input dataset with the field data.
  vtkDataSet* in_ds = constructInputDataSet();

  PsudoFilter a(std::vector<double>(3,0));
  PsudoFilter b(std::vector<double>(3,0));
  PsudoFilter c(std::vector<double>(3,0));

  vtkDataSet* out_ds = c.Execute(b.Execute(a.Execute(in_ds)));
}

void testgetMetaDataID()
{

  TSM_ASSERT_EQUALS("The expected id for the slicing metadata was not found", "1", RebinningCutterPresenter::metaDataId);
}


void testMetaDataToFieldData()
{
  std::string testData = "<test data/>%s";
  std::string id = RebinningCutterPresenter::metaDataId;

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
  std::string testData = "<test data/>%s";
  std::string id = RebinningCutterPresenter::metaDataId;

  vtkFieldData* emptyFieldData = vtkFieldData::New();
  metaDataToFieldData(emptyFieldData, testData.c_str() , id.c_str() );

  //convert vtkchararray back into a string.
  vtkCharArray* carry = dynamic_cast<vtkCharArray*>(emptyFieldData->GetArray(id.c_str()));

  TSM_ASSERT_EQUALS("The result does not match the input. Metadata not properly converted.", testData, convertCharArrayToString(carry));
  emptyFieldData->Delete();
}

void testFieldDataToMetaData()
{
  std::string testData = "test data";
  std::string id = RebinningCutterPresenter::metaDataId;

  vtkFieldData* fieldData = createFieldDataWithCharArray(testData, id);

  std::string metaData = fieldDataToMetaData(fieldData, id.c_str());
  TSM_ASSERT_EQUALS("The result does not match the input. Field data not properly converted.", testData, metaData);
  fieldData->Delete();
}

void testFindExistingRebinningDefinitions()
{
  using namespace Mantid::API;
  using namespace Mantid::MDAlgorithms;
  std::string id = RebinningCutterPresenter::metaDataId;
  vtkDataSet* dataset = constructInputDataSet();

  ImplicitFunction* func = findExistingRebinningDefinitions(dataset, id.c_str());

  TSM_ASSERT("There was a previous definition of a function that should have been recognised and generated."
      , CompositeImplicitFunction::functionName() == func->getName());

  delete func;
}


void testNoExistingRebinningDefinitions()
{
  using namespace Mantid::API;

  vtkDataSet* dataset = vtkUnstructuredGrid::New();

  ImplicitFunction* func = findExistingRebinningDefinitions(dataset, RebinningCutterPresenter::metaDataId.c_str());

  TSM_ASSERT("There were no previous definitions carried through.", NULL == func);
}


void testConstructionWithoutValidOriginThrows()
{
  using namespace Mantid::Geometry;
  RebinningCutterPresenter presenter(vtkUnstructuredGrid::New(), 1);
  using Mantid::VATES::DimensionVec;
  using Mantid::VATES::Dimension_sptr;

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
    using namespace Mantid::API;
    using namespace Mantid::MDAlgorithms;

    Mantid::VATES::RebinningCutterPresenter presenter(vtkUnstructuredGrid::New(), 1);

    TSM_ASSERT_THROWS("Should have thrown if constructReductionKnowledge not called first.", presenter.applyReductionKnowledge("", false), std::runtime_error);
}

void testFindWorkspaceName()
{
  std::string id = RebinningCutterPresenter::metaDataId;
  vtkDataSet* dataset = constructInputDataSet();

  std::string name = findExistingWorkspaceNameFromXML(dataset, id.c_str());

  TSM_ASSERT_EQUALS("The workspace name is different from the xml value.", "Input", name );
}

void testFindWorkspaceLocation()
{
  std::string id = RebinningCutterPresenter::metaDataId;
  vtkDataSet* dataset = constructInputDataSet();

  std::string location = findExistingWorkspaceLocationFromXML(dataset, id.c_str());

  TSM_ASSERT_EQUALS("The workspace location is differrent from the xml value.", "fe_demo.sqw", location);
}

void testFindWorkspaceNameThrows()
{
  vtkDataSet* dataset = vtkUnstructuredGrid::New();
  std::string id =  RebinningCutterPresenter::metaDataId;
  dataset->SetFieldData(createFieldDataWithCharArray("<IncorrectXML></IncorrectXML>", id.c_str()));

  TSM_ASSERT_THROWS("The xml does not contain a name element, so should throw.", findExistingWorkspaceNameFromXML(dataset, id.c_str()), std::runtime_error);
}

void testFindWorkspaceLocationThrows()
{
  vtkDataSet* dataset = vtkUnstructuredGrid::New();
  std::string id =  RebinningCutterPresenter::metaDataId;
  dataset->SetFieldData(createFieldDataWithCharArray("<IncorrectXML></IncorrectXML>", id.c_str()));

  TSM_ASSERT_THROWS("The xml does not contain a location element, so should throw.", findExistingWorkspaceLocationFromXML(dataset, id.c_str()), std::runtime_error);
}



};


#endif 
