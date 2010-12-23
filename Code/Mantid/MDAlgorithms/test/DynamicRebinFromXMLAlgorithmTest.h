#ifndef DYNAMIC_REBINNING_FROM_XML_TEST_H_
#define DYNAMIC_REBINNING_FROM_XML_TEST_H_

#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"

#include <cxxtest/TestSuite.h>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include "MantidMDAlgorithms/CompositeImplicitFunction.h"
#include "MantidMDAlgorithms/DynamicRebinFromXML.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"


#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MDDataObjects/MDWorkspace.h"
#include "MantidMDAlgorithms/Load_MDWorkspace.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::Geometry;


class DynamicRebinFromXMLAlgorithmTest: public CxxTest::TestSuite
{


private:

  //Helper method to create the geometry.
  static Mantid::Geometry::MDGeometry* constructMDGeometry()
  {
    using namespace Mantid::Geometry;
    std::set<MDBasisDimension> basisDimensions;
    basisDimensions.insert(MDBasisDimension("q1", true, 1));
    basisDimensions.insert(MDBasisDimension("q2", true, 2));
    basisDimensions.insert(MDBasisDimension("q3", true, 3));
    basisDimensions.insert(MDBasisDimension("u1", false, 4));

    UnitCell cell;
    return new MDGeometry(MDGeometryBasis(basisDimensions, cell));
  }

  //Helper stock constructional method.
  static Mantid::MDDataObjects::MDWorkspace* constructMDWorkspace()
  {
    using namespace Mantid::MDDataObjects;
    using namespace Mantid::Geometry;

    Load_MDWorkspace loader;
    loader.initialize();
    loader.setPropertyValue("inFilename","../../../../Test/VATES/fe_demo.sqw");
    std::string targetWorkspaceName = "Input";
    loader.setPropertyValue("MDWorkspace",targetWorkspaceName);
    loader.execute();

    Workspace_sptr result=AnalysisDataService::Instance().retrieve(targetWorkspaceName);
    MDWorkspace* workspace = dynamic_cast<MDWorkspace *>(result.get());

    return workspace;
  }

  //Helper class exposes a few protected functions as an aid to development.
  class ExposedDynamicRebinFromXML : public DynamicRebinFromXML
  {
  public:
    
    std::string getWorkspaceName(Poco::XML::Element* pRootElem) const
    {
      //call method on base class.
      return DynamicRebinFromXML::getWorkspaceName(pRootElem);
    }
    
    std::string getWorkspaceLocation(Poco::XML::Element* pRootElem) const
    {
      //call method on base class.
      return DynamicRebinFromXML::getWorkspaceLocation(pRootElem);
    }
    
    Mantid::API::ImplicitFunction* getImplicitFunction(Poco::XML::Element* pRootElem) const
    {
      //call method on base class.
      return DynamicRebinFromXML::getImplicitFunction(pRootElem);
    }

    Mantid::Geometry::MDGeometryDescription* getMDGeometryDescriptionWithoutCuts(Poco::XML::Element* pRootElem) const
    {
      //call method on base class.
      return DynamicRebinFromXML::getMDGeometryDescriptionWithoutCuts(pRootElem);
    }

    void ApplyImplicitFunctionToMDGeometryDescription(Mantid::Geometry::MDGeometryDescription* description, Mantid::API::ImplicitFunction* impFunction) const
    {
      //call method on base class.
      return DynamicRebinFromXML::ApplyImplicitFunctionToMDGeometryDescription(description, impFunction);
    }
  };

  //Helper method provides sample instructional xml.
  static std::string MDInstructionXMLString()
  {
    return std::string("<?xml version=\"1.0\" encoding=\"utf-8\"?>") +
"<MDInstruction>" +
  "<MDWorkspaceName>Input</MDWorkspaceName>" +
  "<MDWorkspaceLocation>../../../fe_demo.sqw</MDWorkspaceLocation>" +
  "<DimensionSet>" +
    "<Dimension ID=\"en\">" +
      "<Name>Energy</Name>" +
      "<UpperBounds>150</UpperBounds>" +
      "<LowerBounds>0</LowerBounds>" +
      "<NumberOfBins>4</NumberOfBins>" +
    "</Dimension>" +
    "<Dimension ID=\"qx\">" +
      "<Name>Qx</Name>" +
      "<UpperBounds>5</UpperBounds>" +
      "<LowerBounds>-1.5</LowerBounds>" +
      "<NumberOfBins>7</NumberOfBins>" +
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
      "<NumberOfBins>6</NumberOfBins>" +
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

  static Poco::XML::Element* MDInstructionXML()
  {
   Poco::XML::DOMParser pParser;
   std::string xmlToParse = MDInstructionXMLString();
   Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
   return pDoc->documentElement();
  }

public:

  void testName()
  {
    Mantid::MDAlgorithms::DynamicRebinFromXML xmlRebinAlg;
    TSM_ASSERT_EQUALS  ("Algorithm name should be Power", xmlRebinAlg.name(), "DynamicRebinFromXML" )
  }

  void testVersion()
  {
    Mantid::MDAlgorithms::DynamicRebinFromXML xmlRebinAlg;
    TSM_ASSERT_EQUALS("Expected version is 1", xmlRebinAlg.version(), 1 )
  }

  
  void testInit()
  {
    Mantid::MDAlgorithms::DynamicRebinFromXML xmlRebinAlg;
    xmlRebinAlg.initialize();
    TS_ASSERT( xmlRebinAlg.isInitialized() )
  
    const std::vector<Property*> props = xmlRebinAlg.getProperties();
    TSM_ASSERT_EQUALS("There should only be 2 properties for this dyamic rebinning algorithm", props.size(), 2);
  
    TS_ASSERT_EQUALS( props[0]->name(), "XMLInputString" );
    TS_ASSERT( props[0]->isDefault() );
    TS_ASSERT( dynamic_cast<PropertyWithValue<std::string>* >(props[0]) );
  
    TS_ASSERT_EQUALS( props[1]->name(), "OutputWorkspace" );
    TS_ASSERT( props[1]->isDefault() );
  }

  void testSetProperties()
  {
    Mantid::MDAlgorithms::DynamicRebinFromXML xmlRebinAlg;
    xmlRebinAlg.initialize();
    std::string xmlString = "ss";
    xmlRebinAlg.setPropertyValue("XMLInputString", xmlString);
    TSM_ASSERT_EQUALS("Property XMLInputString cannot be set and fetched correctly.", xmlString, xmlRebinAlg.getPropertyValue("XMLInputString"));
  }

  void testGetWorkspaceName()
  {
    ExposedDynamicRebinFromXML xmlRebinAlg;
    TSM_ASSERT_EQUALS("The workspace name is not correctly extracted", "Input", xmlRebinAlg.getWorkspaceName(MDInstructionXML()));
  }

  void testGetWorkspaceLocation()
  {
    ExposedDynamicRebinFromXML xmlRebinAlg;
    TSM_ASSERT_EQUALS("The workspace location is not correctly extracted", "../../../fe_demo.sqw", xmlRebinAlg.getWorkspaceLocation(MDInstructionXML()));
  }

  void testGetImplicitFunction()
  {
    ExposedDynamicRebinFromXML xmlRebinAlg;
    boost::scoped_ptr<Mantid::API::ImplicitFunction> impFunction(xmlRebinAlg.getImplicitFunction(MDInstructionXML()));
    
    CompositeImplicitFunction* compFunction = dynamic_cast<CompositeImplicitFunction*>(impFunction.get());

    TSM_ASSERT("Has not parsed implicit function(s) correctly", NULL != compFunction); 
    TSM_ASSERT_EQUALS("Has not parsed implicit function(s) correctly", 2, compFunction->getNFunctions()); 
  }

  void testGetMDDimensionDescription()
  {
    ExposedDynamicRebinFromXML xmlRebinAlg;
    boost::scoped_ptr<Mantid::Geometry::MDGeometryDescription> geomDescription(xmlRebinAlg.getMDGeometryDescriptionWithoutCuts(MDInstructionXML()));

    //Characteristic of existing behaviour rather than correct behaviour!
    TSM_ASSERT_EQUALS("The xml generated from the dimension description did not match the expectation.", geomDescription->toXMLstring(), "TEST PROPERTY"); 

    //Note that the MDGeometry description orders dimensions passed to it internally.
	TSM_ASSERT_EQUALS("Wrong number of bins returned for first dimension", 7,  geomDescription->pDimDescription(0)->nBins);
	TSM_ASSERT_EQUALS("Wrong number of bins returned for second dimension", 5, geomDescription->pDimDescription(1)->nBins);
	TSM_ASSERT_EQUALS("Wrong number of bins returned for third dimension", 6,  geomDescription->pDimDescription(2)->nBins);
	TSM_ASSERT_EQUALS("Wrong number of bins returned for fourth dimension", 4, geomDescription->pDimDescription(3)->nBins);
 
	TSM_ASSERT_EQUALS("Incorrect axis name for first dimension", "Qx",     geomDescription->pDimDescription(0)->AxisName);
	TSM_ASSERT_EQUALS("Incorrect axis name for second dimension", "Qy",    geomDescription->pDimDescription(1)->AxisName);
	TSM_ASSERT_EQUALS("Incorrect axis name for third dimension", "Qz",     geomDescription->pDimDescription(2)->AxisName);
	TSM_ASSERT_EQUALS("Incorrect axis name for fourth dimension", "Energy",geomDescription->pDimDescription(3)->AxisName);
    
	TSM_ASSERT_EQUALS("Incorrect id for first dimension", "qx",  geomDescription->pDimDescription(0)->Tag);
    TSM_ASSERT_EQUALS("Incorrect id for second dimension", "qy", geomDescription->pDimDescription(1)->Tag);
	TSM_ASSERT_EQUALS("Incorrect id for third dimension", "qz",  geomDescription->pDimDescription(2)->Tag);
	TSM_ASSERT_EQUALS("Incorrect id for fourth dimension", "en", geomDescription->pDimDescription(3)->Tag);

  }

  void testIncorrectRootNode()
  {

    Mantid::MDAlgorithms::DynamicRebinFromXML xmlRebinAlg;
    xmlRebinAlg.setRethrows(true); 
    xmlRebinAlg.initialize();
    xmlRebinAlg.setPropertyValue("OutputWorkspace","WSCor");
    std::string xmlString = "<Other></Other>";
    xmlRebinAlg.setPropertyValue("XMLInputString", xmlString);
    
    TSM_ASSERT_THROWS("Root node must be an MDInstruction", xmlRebinAlg.execute(), std::invalid_argument);
  }

  void testApplyImplicitFunctionToMDGeometryDescription()
  {
    ExposedDynamicRebinFromXML xmlRebinAlg;

    MDGeometryDescription* description = xmlRebinAlg.getMDGeometryDescriptionWithoutCuts(MDInstructionXML());
    ImplicitFunction* impFunction = xmlRebinAlg.getImplicitFunction(MDInstructionXML());
    xmlRebinAlg.ApplyImplicitFunctionToMDGeometryDescription(description, impFunction);

	TSM_ASSERT_EQUALS("Wrong x-min set via cut box.", -2, description->pDimDescription(0)->cut_min);
    TSM_ASSERT_EQUALS("Wrong x-min set via cut box.", 0.75,  description->pDimDescription(0)->cut_max);
    TSM_ASSERT_EQUALS("Wrong y-min set via cut box.", -3, description->pDimDescription(1)->cut_min);
    TSM_ASSERT_EQUALS("Wrong y-min set via cut box.", 0.75,  description->pDimDescription(1)->cut_max);
	TSM_ASSERT_EQUALS("Wrong z-min set via cut box.", -3, description->pDimDescription(2)->cut_min);
    TSM_ASSERT_EQUALS("Wrong z-min set via cut box.", 3,  description->pDimDescription(2)->cut_max);
  }

  void testExecute()
  {
    using namespace Mantid::MDDataObjects;

    MDWorkspace_sptr baseWs = MDWorkspace_sptr(constructMDWorkspace());
    //AnalysisDataService::Instance().add("Input", baseWs); // you can not do it twice

    Mantid::MDAlgorithms::DynamicRebinFromXML xmlRebinAlg;
    xmlRebinAlg.setRethrows(true); 
    xmlRebinAlg.initialize();
    xmlRebinAlg.setPropertyValue("OutputWorkspace","MyOutputWS");
    std::string xmlString = MDInstructionXMLString();
    xmlRebinAlg.setPropertyValue("XMLInputString", xmlString);
    xmlRebinAlg.execute();
    
    MDWorkspace_sptr output = boost::dynamic_pointer_cast<MDWorkspace>(AnalysisDataService::Instance().retrieve("MyOutputWS"));

    //Check that the rebinning worked as specified in the request.
    TSM_ASSERT_EQUALS("Wrong number of bins in rebinned workspace x-dimension.", 7, output->getXDimension()->getNBins());
    TSM_ASSERT_EQUALS("Wrong number of bins in rebinned workspace y-dimension.", 5, output->getYDimension()->getNBins());
    TSM_ASSERT_EQUALS("Wrong number of bins in rebinned workspace z-dimension.", 6, output->getZDimension()->getNBins());
    TSM_ASSERT_EQUALS("Wrong number of bins in rebinned workspace t-dimension.", 4, output->gettDimension()->getNBins());
    //840 = 7 * 5* 6 * 4
    TSM_ASSERT_EQUALS("The image size should be the product of the number of bins accross dimensions", 840, output->get_spMDImage()->getDataSize());

  }

};

#endif
