#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/NamedNodeMap.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/File.h"
#include "Poco/Path.h"

#include "MantidMDAlgorithms/DynamicRebinFromXML.h"
#include "MantidMDAlgorithms/CompositeImplicitFunction.h"
#include "MantidMDAlgorithms/BoxImplicitFunction.h"
#include "MantidMDAlgorithms/CenterpieceRebinning.h"
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"
#include "MantidGeometry/MDGeometry/MDDimension.h"
#include "MantidGeometry/MDGeometry/MDDimensionRes.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidAPI/ImplicitFunctionFactory.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MDDataObjects/MDWorkspace.h"
#include "MantidAPI/MDPropertyGeometry.h"

#include "boost/scoped_ptr.hpp"
#include "boost/regex.hpp"

namespace Mantid
{
  namespace MDAlgorithms
  {
        DECLARE_ALGORITHM(DynamicRebinFromXML)

    /// Default (empty) constructor
    DynamicRebinFromXML::DynamicRebinFromXML() : API::Algorithm()
    {
    }

    /// Default (empty) destructor
    DynamicRebinFromXML::~DynamicRebinFromXML()
    {}

    struct findID//: public std::unary_function <MDDimension, bool>
    {
      const std::string m_id;
      findID( const std:: string id ): m_id( id ) 
      { 
      }
      bool operator () ( const boost::shared_ptr<Mantid::Geometry::IMDDimension> obj ) const
      { 
        return m_id == obj->getDimensionId();
      }
    };

    std::string DynamicRebinFromXML::getWorkspaceName(Poco::XML::Element* pRootElem) const 
    {
      //Extract and return workspace name.
      Poco::XML::Element* workspaceNameXML = pRootElem->getChildElement("MDWorkspaceName");
      return workspaceNameXML->innerText();
    }

    std::string DynamicRebinFromXML::getWorkspaceLocation(Poco::XML::Element* pRootElem) const
    {
      //Extract and return workspace location.
      Poco::XML::Element* workspaceLocationXML = pRootElem->getChildElement("MDWorkspaceLocation");
      return workspaceLocationXML->innerText();
    }

    Mantid::API::ImplicitFunction* DynamicRebinFromXML::getImplicitFunction(Poco::XML::Element* pRootElem) const
    {
      Poco::XML::Element* functionXML = pRootElem->getChildElement("Function");

      //de-serialise the function component.
      return Mantid::API::ImplicitFunctionFactory::Instance().createUnwrapped(functionXML);
    }


    Mantid::Geometry::IMDDimension* DynamicRebinFromXML::createDimension(Poco::XML::Element* dimensionXML) const
    {
      using namespace Mantid::Geometry;
      Poco::XML::NamedNodeMap* attributes = dimensionXML->attributes();

      //First and only attribute is the dimension id.
      Poco::XML::Node* dimensionId = attributes->item(0);
      std::string id = dimensionId->innerText();

      Poco::XML::Element* reciprocalMapping = dimensionXML->getChildElement("ReciprocalDimensionMapping");
      MDDimension* mdDimension;

      if(NULL != reciprocalMapping)
      {
        rec_dim recipPrimitiveDirection;

        //Reciprocal dimensions are either q1, q2, or  q3.
        static const boost::regex q1Match("(q1)|(qx)");
        static const boost::regex q2Match("(q2)|(qy)");

        if(regex_match(reciprocalMapping->innerText(), q1Match))
        {
          recipPrimitiveDirection = q1; //rec_dim::q1
        }
        else if(regex_match(reciprocalMapping->innerText(), q2Match))
        {
          recipPrimitiveDirection = q2; //rec_dim::q2
        }
        else
        {
          recipPrimitiveDirection = q3; //rec_dim::q3
        }
        //Create the dimension as a reciprocal dimension
        mdDimension = new MDDimensionRes(id, recipPrimitiveDirection); 
      }
      else
      {
        //Create the dimension as an orthogonal dimension.
        mdDimension = new MDDimension(id); 
      }

      std::string name = dimensionXML->getChildElement("Name")->innerText();
      mdDimension->setName(name);

      double upperBounds = atof(dimensionXML->getChildElement("UpperBounds")->innerText().c_str());
      double lowerBounds = atof(dimensionXML->getChildElement("LowerBounds")->innerText().c_str());
      unsigned int nBins = atoi(dimensionXML->getChildElement("NumberOfBins")->innerText().c_str());
      Poco::XML::Element* integrationXML = dimensionXML->getChildElement("Integrated");

      if(NULL != integrationXML)
      {
        double upperLimit = atof(integrationXML->getChildElement("UpperLimit")->innerText().c_str());
        double lowerLimit = atof(integrationXML->getChildElement("LowerLimit")->innerText().c_str());

        //As it is not currently possible to set integration ranges on a MDDimension or MDGeometryDescription, boundaries become integration ranges.
        upperBounds = upperLimit;
        lowerBounds = lowerLimit;
      }

      mdDimension->setRange(lowerBounds, upperBounds, nBins); //HACK- this method should be protected and used by geometry only, not a valid dimension 
      return mdDimension;
    }

    Mantid::Geometry::MDGeometryDescription* DynamicRebinFromXML::getMDGeometryDescriptionWithoutCuts(Poco::XML::Element* pRootElem) const
    {
      using namespace Mantid::Geometry;
      Poco::XML::Element* geometryXML = pRootElem->getChildElement("DimensionSet");

      Poco::XML::NodeList* dimensionsXML = geometryXML->getElementsByTagName("Dimension");
      std::vector<boost::shared_ptr<IMDDimension> > dimensionVec;

      //Extract dimensions
      int nDimensions = dimensionsXML->length();
      for(int i = 0; i < nDimensions; i++)
      {
        Poco::XML::Element* dimensionXML = static_cast<Poco::XML::Element*>(dimensionsXML->item(i));
        IMDDimension* dimension = createDimension(dimensionXML);
        dimensionVec.push_back(boost::shared_ptr<IMDDimension>(dimension));
      }

      //Find the requested xDimension alignment from the dimension id provided in the xml.
      Poco::XML::Element* xDimensionElement = geometryXML->getChildElement("XDimension");
      std::string xDimId = xDimensionElement->getChildElement("RefDimensionId")->innerText();
      DimensionVecIterator xDimensionIt = find_if(dimensionVec.begin(), dimensionVec.end(), findID(xDimId));

      //Find the requested yDimension alignment from the dimension id provided in the xml.
      Poco::XML::Element* yDimensionElement = geometryXML->getChildElement("YDimension");
      std::string yDimId = yDimensionElement->getChildElement("RefDimensionId")->innerText();
      DimensionVecIterator yDimensionIt = find_if(dimensionVec.begin(), dimensionVec.end(), findID(yDimId));

      //Find the requested zDimension alignment from the dimension id provided in the xml.
      Poco::XML::Element* zDimensionElement = geometryXML->getChildElement("ZDimension");
      std::string zDimId = zDimensionElement->getChildElement("RefDimensionId")->innerText();
      DimensionVecIterator zDimensionIt = find_if(dimensionVec.begin(), dimensionVec.end(), findID(zDimId));

      //Find the requested tDimension alignment from the dimension id provided in the xml.
      Poco::XML::Element* tDimensionElement = geometryXML->getChildElement("TDimension");
      std::string tDimId = tDimensionElement->getChildElement("RefDimensionId")->innerText();
      DimensionVecIterator tDimensionIt = find_if(dimensionVec.begin(), dimensionVec.end(), findID(tDimId));

      //Check that all mappings have actually been found.
      if(xDimensionIt == dimensionVec.end())
      {
        throw std::invalid_argument("Cannot determine x-dimension mapping.");
      }
      if(yDimensionIt == dimensionVec.end())
      {
        throw std::invalid_argument("Cannot determine y-dimension mapping.");
      }
      if(zDimensionIt == dimensionVec.end())
      {
        throw std::invalid_argument("Cannot determine z-dimension mapping.");
      }
      if(tDimensionIt == dimensionVec.end())
      {
        throw std::invalid_argument("Cannot determine t-dimension mapping.");
      }

      //Create A geometry Description.
      return new MDGeometryDescription(dimensionVec, *xDimensionIt, *yDimensionIt, *zDimensionIt, *tDimensionIt);
    }

    boxVec walkTree(CompositeImplicitFunction* compFunc)
    {
      using namespace Mantid::API;
      boxVec flattenedboxes;
      functionVec nestedFuncs = compFunc->getFunctions();
      for(int i = 0; i < nestedFuncs.size(); i++)
      {
        ImplicitFunction* impFunc = nestedFuncs[i].get();
        CompositeImplicitFunction* nestedFunc = dynamic_cast<Mantid::MDAlgorithms::CompositeImplicitFunction*>(impFunc);
        if(NULL != nestedFunc)
        {
          boxVec boxes = walkTree(nestedFunc); //recursive walk
          flattenedboxes.insert(flattenedboxes.end(), boxes.begin(), boxes.end());
        }
        else if(BoxImplicitFunction* box = dynamic_cast<BoxImplicitFunction*>(impFunc))
        {
          flattenedboxes.push_back(boost::shared_ptr<BoxImplicitFunction>(box));
        }
      }
      return flattenedboxes;
    }

    void DynamicRebinFromXML::ApplyImplicitFunctionToMDGeometryDescription(Mantid::Geometry::MDGeometryDescription* description, Mantid::API::ImplicitFunction* impFunction) const
    {

      Mantid::MDAlgorithms::CompositeImplicitFunction* compFunction = dynamic_cast<Mantid::MDAlgorithms::CompositeImplicitFunction*>(impFunction);
      boxVec flattenedboxes;
      if(compFunction != NULL)
      {
        //Flatten out box functions
        flattenedboxes = walkTree(compFunction); 
      }
      
      boost::shared_ptr<BoxImplicitFunction> box = flattenedboxes[0];
      double minX = box->getLowerX();
      double minY = box->getLowerY();
      double minZ = box->getLowerZ();
      double maxX = box->getUpperX();
      double maxY = box->getUpperY();
      double maxZ = box->getUpperZ();
      for(int i = 0; i < flattenedboxes.size(); i++)
      {
         box = flattenedboxes[i];
         minX = minX < box->getLowerX() ? minX : box->getLowerX();
         minY = minY < box->getLowerY() ? minY : box->getLowerY();
         minZ = minZ < box->getLowerZ() ? minZ : box->getLowerZ();
         maxX  = maxX < box->getUpperX() ? maxX : box->getUpperX();
         maxY  = maxY < box->getUpperY() ? maxY : box->getUpperY();
         maxZ  = maxZ < box->getUpperZ() ? maxZ : box->getUpperZ();
      }
      
      //Determine effective width, height, depth and origin for effective box.
      OriginParameter origin((maxX + minX)/2, (maxY + minY)/2, (maxZ + minZ)/2);
      WidthParameter width(maxX - minX);
      HeightParameter height(maxY - minY);
      DepthParameter depth(maxZ - minZ);
      //create a new box from the intesection.
      BoxImplicitFunction impBox(width, height, depth, origin);
      
      //current implmentation of geometry description uses ordering so that x, y, z, t mappings appear first in the arrangement of dimensions.

	    description->pDimDescription(0)->cut_max=impBox.getUpperX();
      description->pDimDescription(0)->cut_min=impBox.getLowerX();


      description->pDimDescription(1)->cut_max=impBox.getUpperY();
      description->pDimDescription(1)->cut_min=impBox.getLowerY();

      description->pDimDescription(2)->cut_max=impBox.getUpperZ();
      description->pDimDescription(2)->cut_min=impBox.getLowerZ();


    }

    /// Init function
    void DynamicRebinFromXML::init()
    {
      //input xml string
      Mantid::Kernel::MandatoryValidator<std::string> *v = new Mantid::Kernel::MandatoryValidator<std::string>;
      declareProperty(new Kernel::PropertyWithValue<std::string>(std::string("XMLInputString"), std::string(""), v), "XML string providing dynamic rebinning instructions."); 
      //output workspace following rebinning
      declareProperty(new Mantid::API::WorkspaceProperty<Mantid::MDDataObjects::MDWorkspace>("OutputWorkspace", "", Kernel::Direction::Output), "Rebinned Workspace.");

    }


    /// Exec function
    void DynamicRebinFromXML::exec()
    {
      using namespace Mantid::Geometry;
      using namespace Mantid::MDDataObjects;
      using namespace Mantid::API;

      const std::string xmlString = getProperty("XMLInputString");

      //Translate xml to MDDescription.
      Poco::XML::DOMParser pParser;
      std::string xmlToParse = xmlString;
      Poco::XML::Document* pDoc = pParser.parseString(xmlToParse);
      Poco::XML::Element* pRootElem = pDoc->documentElement();

      //Coarse test for xml schema.
      if(pRootElem->localName() != "MDInstruction")
      {
        throw std::invalid_argument("MDInstruction must be root node");
      }

      //de-serialise the function component.
      ImplicitFunction* function = getImplicitFunction(pRootElem);

      //get the input workspace.
      std::string name = getWorkspaceName(pRootElem);

      //get the location of the workspace. this should be redundant in future versions!
      std::string location = getWorkspaceLocation(pRootElem);

      //get the md geometry without cuts applied. Cuts are applied by the implicit functions.
      MDGeometryDescription* geomDescription = getMDGeometryDescriptionWithoutCuts(pRootElem);

      //apply cuts to the geometrydescription.
      ApplyImplicitFunctionToMDGeometryDescription(geomDescription, function);


      IAlgorithm_sptr rebinningAlg = this->createSubAlgorithm("CenterpieceRebinning", 0.001, 1, true, 1);
      rebinningAlg->setPropertyValue("Input", name);
      
      
      rebinningAlg->setPropertyValue("FileName", location);
      rebinningAlg->initialize();
      
      //HACK: only way to apply a geometry description seems to be to grab it from the algorithm and apply it after initalize!
      Geometry::MDGeometryDescription *pSlicing = dynamic_cast< Geometry::MDGeometryDescription *>((Mantid::Kernel::Property *)(rebinningAlg->getProperty("SlicingData")));
      *pSlicing = *geomDescription;
      
      rebinningAlg->execute();

      MDWorkspace_sptr output = rebinningAlg->getProperty("Result");
      setProperty("OutputWorkspace", output);
  
    }

  } // Algorithms
} // Mantid
