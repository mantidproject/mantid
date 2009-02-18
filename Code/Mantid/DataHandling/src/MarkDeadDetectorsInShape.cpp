//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/MarkDeadDetectorsInShape.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid
{
	namespace DataHandling
	{
		// Register the algorithm into the algorithm factory
		DECLARE_ALGORITHM(MarkDeadDetectorsInShape)

		using namespace Kernel;
		using namespace API;
		using namespace DataObjects;

		// Initialise the logger
		Kernel::Logger& MarkDeadDetectorsInShape::g_log = Kernel::Logger::get("MarkDeadDetectorsInShape");

		/// (Empty) Constructor
		MarkDeadDetectorsInShape::MarkDeadDetectorsInShape() {}

		/// Destructor
		MarkDeadDetectorsInShape::~MarkDeadDetectorsInShape() {}

		void MarkDeadDetectorsInShape::init()
		{
			declareProperty(new WorkspaceProperty<Workspace2D>("Workspace","",Direction::InOut));
			declareProperty("ShapeXML","",new MandatoryValidator<std::string>());
			declareProperty("IncludeMonitors",false);
			declareProperty("DetectorList",std::vector<int>(),Direction::Output);
		}

		void MarkDeadDetectorsInShape::exec()
		{
			// Get the input workspace
			Workspace2D_sptr WS = getProperty("Workspace");
			
			const bool includeMonitors = getProperty("IncludeMonitors");
			const std::string shapeXML = getProperty("ShapeXML");


			std::vector<int> foundDets = runFindDetectorsInShape(WS,shapeXML,includeMonitors);
			runMarkDeadDetectors(WS,foundDets);
			setProperty("Workspace",WS);
			setProperty("DetectorList",foundDets);
		}

		/// Run the FindDetectorsInShape sub-algorithm
    std::vector<int> MarkDeadDetectorsInShape::runFindDetectorsInShape(Workspace2D_sptr workspace,
			const std::string shapeXML, const bool includeMonitors)
    {
      Algorithm_sptr alg = createSubAlgorithm("FindDetectorsInShape",0,85);
			alg->setPropertyValue("IncludeMonitors", includeMonitors?"1":"0");
			alg->setPropertyValue("ShapeXML",shapeXML);
      alg->setProperty<MatrixWorkspace_sptr>("Workspace",workspace);
      try
      {
        if (!alg->execute())
				{
					throw std::runtime_error("FindDetectorsInShape sub-algorithm has not executed successfully\n");
				}
      }
      catch (std::runtime_error& err)
      {
    	  g_log.error("Unable to successfully execute FindDetectorsInShape sub-algorithm");
				throw;
      }

			//extract the results
			return alg->getProperty("DetectorList");
    }

		void MarkDeadDetectorsInShape::runMarkDeadDetectors(Workspace2D_sptr workspace, const std::vector<int> detectorIds)
    {
      Algorithm_sptr alg = createSubAlgorithm("MarkDeadDetectors",85,100);
			alg->setProperty<std::vector<int> >("DetectorList", detectorIds);
      alg->setProperty<Workspace2D_sptr>("Workspace",workspace);
      try
      {
        if (!alg->execute())
				{
					throw std::runtime_error("MarkDeadDetectors sub-algorithm has not executed successfully\n");
				}
      }
      catch (std::runtime_error& err)
      {
    	  g_log.error("Unable to successfully execute MarkDeadDetectors sub-algorithm");
				throw;
      }
    }

	} // namespace DataHandling
} // namespace Mantid

