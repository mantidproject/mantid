//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include"mantidDataHandling/LoadCanSAS1D.h"
#include "MantidKernel/FileProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/UnitFactory.h"

#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/Node.h"
#include "Poco/DOM/Text.h"
//-----------------------------------------------------------------------

using Poco::XML::DOMParser;
using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::NodeList;
using Poco::XML::Node;
using Poco::XML::Text;
namespace Mantid

{
	namespace DataHandling
	{
		// Register the algorithm into the AlgorithmFactory
		DECLARE_ALGORITHM(LoadCanSAS1D)
		/// constructor
		LoadCanSAS1D::LoadCanSAS1D()
		{
		}
		/// destructor
		LoadCanSAS1D::~LoadCanSAS1D()
		{
		}

		/// Overwrites Algorithm Init method.
		void LoadCanSAS1D::init()
		{
			std::vector<std::string> exts;
			exts.push_back("xml");
			declareProperty(new Kernel::FileProperty("Filename","", Kernel::FileProperty::Load,exts),
				"The name of the input  xml file to load");
			declareProperty(new API::WorkspaceProperty<>("OutputWorkspace","",Kernel::Direction::Output),
				"The name of the Output workspace");

		}

		/// Overwrites Algorithm exec method
		void LoadCanSAS1D::exec()
		{
			std::string fileName=getPropertyValue("Filename");
			// Set up the DOM parser and parse xml file
			DOMParser pParser;
			Document* pDoc;
			try
			{
				pDoc = pParser.parse(fileName);
			}
			catch(...)
			{				
				throw Kernel::Exception::FileError("Unable to parse File:" , fileName);
			}
			// Get pointer to root element
			Element* pRootElem = pDoc->documentElement();
			if ( !pRootElem->hasChildNodes() )
			{
				throw Kernel::Exception::NotFoundError("No root element in CanSAS1D XML file", fileName);
			}
			Element* sasEntryElem = pRootElem->getChildElement("SASentry");
			throwException(sasEntryElem,"SASentry",fileName);
			Element*titleElem=sasEntryElem->getChildElement("Title");
			throwException(titleElem,"Title",fileName);
			std::string wsTitle=titleElem->innerText();
			Element* sasDataElem = sasEntryElem->getChildElement("SASdata");
			throwException(sasDataElem,"SASdata",fileName);
			// getting number of Idata elements in the xml file
			NodeList* idataElemList= sasDataElem->childNodes();
			unsigned long idataCount=idataElemList->length();
			//no.of bins
			int nBins=idataCount/2;
			const int numSpectra=1;
			// Create the output workspace
			API::MatrixWorkspace_sptr ws =
				(API::WorkspaceFactory::Instance().create("Workspace2D",numSpectra,nBins,nBins));
			ws->setTitle(wsTitle);
			ws->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
			ws->setYUnit("Counts");
			setProperty("OutputWorkspace",ws);

			//load workspace data
			MantidVec& X = ws->dataX(0);
			MantidVec& Y = ws->dataY(0);
			MantidVec& E = ws->dataE(0);
			int vecindex=0;
			//iterate through each Idata element  and get the values of "Q",
			//"I" and "Idev" text nodes and fill X,Y,E vectors
			for(int index=0;index<idataCount;++index)
			{
				Node* idataElem=idataElemList->item(index);
				Element* elem=dynamic_cast<Element*>(idataElem);
				if(elem)
				{
					//setting X vector
					std::string nodeVal;
					Element*qElem=elem->getChildElement("Q");
					throwException(qElem,"Q",fileName);
					nodeVal=qElem->innerText();
					std::stringstream x(nodeVal);
					double d;
					x>>d;
					X[vecindex]=d;

					//setting Y vector
					Element*iElem=elem->getChildElement("I");
					throwException(qElem,"I",fileName);
					nodeVal=iElem->innerText();
					std::stringstream y(nodeVal);
					y>>d;
					Y[vecindex]=d;

					//setting the error vector
					Element*idevElem=elem->getChildElement("Idev");
					throwException(qElem,"Idev",fileName);
					nodeVal=idevElem->innerText();
					std::stringstream e(nodeVal);
					e>>d;
					E[vecindex]=d;
					++vecindex;
				}

			}
			idataElemList->release();

		}

		 /* This method throws not found error if a element is not found in the xml file
		  * @param elem pointer to  element
		  * @param name  element name
		  * @param fileName xml file name
		 */
		void LoadCanSAS1D::throwException(Element* elem,const std::string & name,const std::string& fileName)
		{
			if(!elem)
			{
				throw Kernel::Exception::NotFoundError(name+" element not found in CanSAS1D XML file", fileName);
			}
		}
	}
}