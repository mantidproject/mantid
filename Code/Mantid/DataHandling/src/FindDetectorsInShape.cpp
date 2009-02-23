//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/FindDetectorsInShape.h"
#include "MantidDataHandling/ShapeFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/MatrixWorkspace.h"

#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"

using Poco::XML::DOMParser;
using Poco::XML::Document;
using Poco::XML::Element;

namespace Mantid
{
	namespace DataHandling
	{
		// Register the algorithm into the algorithm factory
		DECLARE_ALGORITHM(FindDetectorsInShape)

		using namespace Kernel;
		using namespace API;

		// Initialise the logger
		Kernel::Logger& FindDetectorsInShape::g_log = Kernel::Logger::get("FindDetectorsInShape");

		/// (Empty) Constructor
		FindDetectorsInShape::FindDetectorsInShape() {}

		/// Destructor
		FindDetectorsInShape::~FindDetectorsInShape() {}

		void FindDetectorsInShape::init()
		{
			declareProperty(new WorkspaceProperty<MatrixWorkspace>("Workspace","",Direction::Input));
			declareProperty("ShapeXML","",new MandatoryValidator<std::string>());
			declareProperty("IncludeMonitors",false);
			declareProperty("DetectorList",std::vector<int>(),Direction::Output);
		}

		void FindDetectorsInShape::exec()
		{
			// Get the input workspace
			const MatrixWorkspace_sptr WS = getProperty("Workspace");
			
			bool includeMonitors = getProperty("IncludeMonitors");

			std::string shapeXML = getProperty("ShapeXML");
			//wrap in a type tag
			shapeXML = "<type name=\"userShape\"> " + shapeXML + " </type>";

			// Set up the DOM parser and parse xml string
			DOMParser pParser;
			Document* pDoc;
			try
			{
				pDoc = pParser.parseString(shapeXML);
			}
			catch(...)
			{
				g_log.error("Unable to parse ShapeXML " + shapeXML);
				throw Kernel::Exception::InstrumentDefinitionError("Unable to parse ShapeXML" , shapeXML);
			}
			// Get pointer to root element
			Element* pRootElem = pDoc->documentElement();

			//convert into a Geometry object
			ShapeFactory sFactory;
			boost::shared_ptr<Geometry::Object> shape_sptr = sFactory.createShape(pRootElem);
			pDoc->release();

			//get the instrument out of the workspace
			IInstrument_sptr instrument_sptr = WS->getInstrument();
			std::vector<Geometry::IObjComponent_sptr> objCmptList = instrument_sptr->getPlottable();

			std::vector<int> foundDets;

			//progress
			int objCmptCount = objCmptList.size();
			int iprogress_step = objCmptCount / 100;
			if (iprogress_step == 0) iprogress_step = 1;

			//for every plottable item
			for (std::vector<Geometry::IObjComponent_sptr>::iterator it = objCmptList.begin(); it!=objCmptList.end(); ++it) 
			{
				//attempt to dynamic cast up to an IDetector
				boost::shared_ptr<Geometry::IDetector> detector_sptr =
					boost::dynamic_pointer_cast<Geometry::IDetector>((*it));

				if (detector_sptr)
				{
					if ((includeMonitors) || (!detector_sptr->isMonitor()))
					{
						//check if the centre of this item is within the user defined shape
						if (shape_sptr->isValid(detector_sptr->getPos()))
						{
							//shape encloses this objectComponent
              g_log.debug()<<"Detector contained in shape " << detector_sptr->getID() << std::endl;
							foundDets.push_back(detector_sptr->getID());
						}
					}
					
				}

				int i=objCmptList.end()-it;
				if (i % iprogress_step == 0)
				{
					progress(double(i)/objCmptCount);
					interruption_point();
				}
			}
			setProperty("DetectorList",foundDets);
		}

	} // namespace DataHandling
} // namespace Mantid

