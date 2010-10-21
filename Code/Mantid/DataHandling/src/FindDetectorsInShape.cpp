//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/FindDetectorsInShape.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
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
    using namespace Geometry;

		/// (Empty) Constructor
		FindDetectorsInShape::FindDetectorsInShape() {}

		/// Destructor
		FindDetectorsInShape::~FindDetectorsInShape() {}

		void FindDetectorsInShape::init()
		{
			declareProperty(
        new WorkspaceProperty<MatrixWorkspace>("Workspace","",Direction::Input),
        "Name of the input workspace" );
			declareProperty("ShapeXML", "", new MandatoryValidator<std::string>(),
        "The XML definition of the shape");
			declareProperty("IncludeMonitors", false,
        "Whether monitors should be included if they are contained in the\n"
        "shape (default false)");
			declareProperty("DetectorList", std::vector<int>(),
        new NullValidator< std::vector<int> >,                                      //the null validator always returns valid, there is no validation
        "The list of detector ids included within the shape",
        Direction::Output);
		}

		void FindDetectorsInShape::exec()
		{
			// Get the input workspace
			const MatrixWorkspace_sptr WS = getProperty("Workspace");
			
			bool includeMonitors = getProperty("IncludeMonitors");

			std::string shapeXML = getProperty("ShapeXML");

			//convert into a Geometry object
			Geometry::ShapeFactory sFactory;
			boost::shared_ptr<Geometry::Object> shape_sptr = sFactory.createShape(shapeXML);

			//get the instrument out of the workspace
			IInstrument_sptr instrument_sptr = WS->getInstrument();
      IInstrument::plottables_const_sptr objCmptList = instrument_sptr->getPlottable();

			std::vector<int> foundDets;

			//progress
			int objCmptCount = objCmptList->size();
			int iprogress_step = objCmptCount / 100;
			if (iprogress_step == 0) iprogress_step = 1;

			//for every plottable item
			for (IInstrument::plottables::const_iterator it = objCmptList->begin(); it!=objCmptList->end(); ++it) 
			{
				//attempt to dynamic cast up to an IDetector
				boost::shared_ptr<const Geometry::IDetector> detector_sptr =
					boost::dynamic_pointer_cast<const Geometry::IDetector>((*it));

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

				int i=objCmptList->end()-it;
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

