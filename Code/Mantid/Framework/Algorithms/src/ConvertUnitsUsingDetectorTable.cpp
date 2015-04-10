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
  const std::string ConvertUnitsUsingDetectorTable::category() const { return "Utility\\Development";}

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


//      // Check whether there is a quick conversion available
//      double factor, power;
//      if ( m_inputUnit->quickConversion(*m_outputUnit,factor,power) )
//      // If test fails, could also check whether a quick conversion in the opposite direction has been entered
//      {
//        this->convertQuickly(outputWS,factor,power);
//      }
//      else
//      {
        this->convertViaTOF(m_inputUnit,outputWS);
//      }

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

  /** Convert the workspace units using TOF as an intermediate step in the conversion
   * @param fromUnit :: The unit of the input workspace
   * @param outputWS :: The output workspace
   */
  void ConvertUnitsUsingDetectorTable::convertViaTOF(Kernel::Unit_const_sptr fromUnit, API::MatrixWorkspace_sptr outputWS)
  {
    using namespace Geometry;

      // Let's see if we are using a TableWorkspace to override parameters
      ITableWorkspace_sptr paramWS = getProperty("DetectorParameters");

      // See if we have supplied a DetectorParameters Workspace
      // TODO: Check if paramWS is NULL and if so throw an exception

      // Some variables to hold our values
      Column_const_sptr l1Column;
      Column_const_sptr l2Column;
      Column_const_sptr spectraColumn;
      Column_const_sptr twoThetaColumn;
      Column_const_sptr efixedColumn;
      Column_const_sptr emodeColumn;

      std::vector<std::string> columnNames = paramWS->getColumnNames();

      // Now lets read the parameters
      try {
          l1Column = paramWS->getColumn("l1");
          l2Column = paramWS->getColumn("l2");
          spectraColumn = paramWS->getColumn("spectra");
          twoThetaColumn = paramWS->getColumn("twotheta");
          efixedColumn = paramWS->getColumn("efixed");
          emodeColumn = paramWS->getColumn("emode");
      } catch (...) {
          throw Exception::InstrumentDefinitionError("DetectorParameter TableWorkspace is not defined correctly.");
      }


      EventWorkspace_sptr eventWS = boost::dynamic_pointer_cast<EventWorkspace>(outputWS);
      assert ( static_cast<bool>(eventWS) == m_inputEvents ); // Sanity check

      Progress prog(this,0.2,1.0,m_numberOfSpectra);
      int64_t numberOfSpectra_i = static_cast<int64_t>(m_numberOfSpectra); // cast to make openmp happy

      // Get the unit object for each workspace
      Kernel::Unit_const_sptr outputUnit = outputWS->getAxis(0)->unit();

      int emode = 0;
      double l1, l2, twoTheta, efixed;

      std::vector<double> emptyVec;
      int failedDetectorCount = 0;

      std::vector<std::string> parameters = outputWS->getInstrument()->getStringParameter("show-signed-theta");
      bool bUseSignedVersion = (!parameters.empty()) && find(parameters.begin(), parameters.end(), "Always") != parameters.end();
      function<double(IDetector_const_sptr)> thetaFunction = bUseSignedVersion ? bind(&MatrixWorkspace::detectorSignedTwoTheta, outputWS, _1) : bind(&MatrixWorkspace::detectorTwoTheta, outputWS, _1);


      // Loop over the histograms (detector spectra)
      PARALLEL_FOR1(outputWS)
              for (int64_t i = 0; i < numberOfSpectra_i; ++i)
      {
          PARALLEL_START_INTERUPT_REGION

          std::size_t wsid = i;

          try
          {
              specid_t spectraNumber = static_cast<specid_t>(spectraColumn->toDouble(i));
              wsid = outputWS->getIndexFromSpectrumNumber(spectraNumber);
              g_log.debug() << "###### Spectra #" << spectraNumber << " ==> Workspace ID:" << wsid << std::endl;
              l1 = l1Column->toDouble(wsid);
              l2 = l2Column->toDouble(wsid);
              twoTheta = twoThetaColumn->toDouble(wsid);
              efixed = efixedColumn->toDouble(wsid);
              emode = static_cast<int>(emodeColumn->toDouble(wsid));


              // Make local copies of the units. This allows running the loop in parallel
              Unit * localFromUnit = fromUnit->clone();
              Unit * localOutputUnit = outputUnit->clone();

              /// @todo Don't yet consider hold-off (delta)
              const double delta = 0.0;
              // Convert the input unit to time-of-flight
              localFromUnit->toTOF(outputWS->dataX(wsid),emptyVec,l1,l2,twoTheta,emode,efixed,delta);
              // Convert from time-of-flight to the desired unit
              localOutputUnit->fromTOF(outputWS->dataX(wsid),emptyVec,l1,l2,twoTheta,emode,efixed,delta);

              // EventWorkspace part, modifying the EventLists.
              if ( m_inputEvents )
              {
                  eventWS->getEventList(wsid).convertUnitsViaTof(localFromUnit, localOutputUnit);
              }
              // Clear unit memory
              delete localFromUnit;
              delete localOutputUnit;

          } catch (Exception::NotFoundError&) {
              // Get to here if exception thrown when calculating distance to detector
              failedDetectorCount++;
              // Since you usually (always?) get to here when there's no attached detectors, this call is
              // the same as just zeroing out the data (calling clearData on the spectrum)
              outputWS->maskWorkspaceIndex(i);
          }

          prog.report("Convert to " + m_outputUnit->unitID());
          PARALLEL_END_INTERUPT_REGION
      } // loop over spectra
      PARALLEL_CHECK_INTERUPT_REGION

              if (failedDetectorCount != 0)
      {
          g_log.information() << "Unable to calculate sample-detector distance for " << failedDetectorCount << " spectra. Masking spectrum." << std::endl;
      }
      if (m_inputEvents)
          eventWS->clearMRU();

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
