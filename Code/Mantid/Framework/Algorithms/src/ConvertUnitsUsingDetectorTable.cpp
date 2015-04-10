#include "MantidAlgorithms/ConvertUnitsUsingDetectorTable.h"

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/AlgorithmFactory.h"

#include "MantidKernel/ListValidator.h"
#include "MantidKernel/UnitFactory.h"


#include "MantidAlgorithms/ConvertUnitsUsingDetectorTable.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <cfloat>
#include <iostream>
#include <limits>
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid
{
namespace Algorithms
{

  using Mantid::Kernel::Direction;
  using Mantid::API::WorkspaceProperty;
  using namespace Kernel;
  using namespace API;
  using namespace DataObjects;
  using boost::function;
  using boost::bind;

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(ConvertUnitsUsingDetectorTable)



  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ConvertUnitsUsingDetectorTable::ConvertUnitsUsingDetectorTable()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ConvertUnitsUsingDetectorTable::~ConvertUnitsUsingDetectorTable()
  {
  }


  //----------------------------------------------------------------------------------------------

  /// Algorithms name for identification. @see Algorithm::name
  const std::string ConvertUnitsUsingDetectorTable::name() const { return "ConvertUnitsUsingDetectorTable"; }

  /// Algorithm's version for identification. @see Algorithm::version
  int ConvertUnitsUsingDetectorTable::version() const { return 1;}

  /// Algorithm's category for identification. @see Algorithm::category
  const std::string ConvertUnitsUsingDetectorTable::category() const { return "Transforms\\Units";}

  /// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
  const std::string ConvertUnitsUsingDetectorTable::summary() const { return "Performs a unit change on the X values of a workspace";}

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void ConvertUnitsUsingDetectorTable::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "An input workspace.");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
    declareProperty("Target","",boost::make_shared<StringListValidator>(UnitFactory::Instance().getKeys()),
                    "The name of the units to convert to (must be one of those registered in\n"
                    "the Unit Factory)");
    declareProperty(new WorkspaceProperty<ITableWorkspace>("DetectorParameters", "", Direction::Input, PropertyMode::Optional),
                    "Name of a TableWorkspace containing the detector parameters to use instead of the IDF.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void ConvertUnitsUsingDetectorTable::exec()
  {
      // Get the workspaces
      MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
      this->setupMemberVariables(inputWS);

      if (m_inputUnit->unitID() == m_outputUnit->unitID())
      {
          const std::string outputWSName = getPropertyValue("OutputWorkspace");
          const std::string inputWSName = getPropertyValue("InputWorkspace");
          if (outputWSName == inputWSName)
          {
              // If it does, just set the output workspace to point to the input one and be done.
              g_log.information() << "Input workspace already has target unit (" << m_outputUnit->unitID() << "), so just pointing the output workspace property to the input workspace."<< std::endl;
              setProperty("OutputWorkspace", boost::const_pointer_cast<MatrixWorkspace>(inputWS));
              return;
          }
          else
          {
              // Clone the workspace.
              IAlgorithm_sptr duplicate = createChildAlgorithm("CloneWorkspace",0.0,0.6);
              duplicate->initialize();
              duplicate->setProperty("InputWorkspace",  inputWS);
              duplicate->execute();
              Workspace_sptr temp = duplicate->getProperty("OutputWorkspace");
              auto outputWs = boost::dynamic_pointer_cast<MatrixWorkspace>(temp);
              setProperty("OutputWorkspace", outputWs);
              return;
          }
      }

      if (inputWS->dataX(0).size() < 2)
      {
          std::stringstream msg;
          msg << "Input workspace has invalid X axis binning parameters. Should have at least 2 values. Found "
              << inputWS->dataX(0).size() << ".";
          throw std::runtime_error(msg.str());
      }
      if (   inputWS->dataX(0).front() > inputWS->dataX(0).back()
             || inputWS->dataX(m_numberOfSpectra/2).front() > inputWS->dataX(m_numberOfSpectra/2).back())
          throw std::runtime_error("Input workspace has invalid X axis binning parameters. X values should be increasing.");

      MatrixWorkspace_sptr outputWS = this->setupOutputWorkspace(inputWS);


      // Check whether there is a quick conversion available
      double factor, power;
      if ( m_inputUnit->quickConversion(*m_outputUnit,factor,power) )
      // If test fails, could also check whether a quick conversion in the opposite direction has been entered
      {
        this->convertQuickly(outputWS,factor,power);
      }
      else
      {
        this->convertViaTOF(m_inputUnit,outputWS);
      }

      // If the units conversion has flipped the ascending direction of X, reverse all the vectors
      if (outputWS->dataX(0).size() && ( outputWS->dataX(0).front() > outputWS->dataX(0).back()
            || outputWS->dataX(m_numberOfSpectra/2).front() > outputWS->dataX(m_numberOfSpectra/2).back() ) )
      {
        this->reverse(outputWS);
      }

      // Need to lop bins off if converting to energy transfer.
      // Don't do for EventWorkspaces, where you can easily rebin to recover the situation without losing information
      /* This is an ugly test - could be made more general by testing for DBL_MAX
         values at the ends of all spectra, but that would be less efficient */
      if ( m_outputUnit->unitID().find("Delta")==0 && !m_inputEvents ) outputWS = this->removeUnphysicalBins(outputWS);

      // Rebin the data to common bins if requested, and if necessary
      bool alignBins = getProperty("AlignBins");
      if (alignBins && !WorkspaceHelpers::commonBoundaries(outputWS))
        outputWS = this->alignBins(outputWS);

      // If appropriate, put back the bin width division into Y/E.
      if (m_distribution && !m_inputEvents)  // Never do this for event workspaces
      {
        this->putBackBinWidth(outputWS);
      }

      // Point the output property to the right place.
      // Do right at end (workspace could could change in removeUnphysicalBins or alignBins methods)
      setProperty("OutputWorkspace",outputWS);
      return;
  }


  /** Divide by the bin width if workspace is a distribution
   *  @param outputWS The workspace to operate on
   */
  void ConvertUnitsUsingDetectorTable::putBackBinWidth(const API::MatrixWorkspace_sptr outputWS)
  {
    const size_t outSize = outputWS->blocksize();

    for (size_t i = 0; i < m_numberOfSpectra; ++i)
    {
      for (size_t j = 0; j < outSize; ++j)
      {
        const double width = std::abs( outputWS->dataX(i)[j+1] - outputWS->dataX(i)[j] );
        outputWS->dataY(i)[j] = outputWS->dataY(i)[j]/width;
        outputWS->dataE(i)[j] = outputWS->dataE(i)[j]/width;
      }
    }
  }

} // namespace Algorithms
} // namespace Mantid
