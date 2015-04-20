#include "MantidMDAlgorithms/IntegrateMDHistoWorkspace.h"
#include "MantidKernel/ArrayProperty.h"

#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDIterator.h"

#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDBoxImplicitFunction.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspaceIterator.h"

#include <algorithm>
#include <map>
#include <utility>

#include <boost/make_shared.hpp>
#include <boost/scoped_ptr.hpp>


using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

namespace {

/**
 * Check for empty binning
 * @param binning : binning
 * @return : true if binning is empty
 */
bool emptyBinning(const std::vector<double>& binning) {
    return binning.empty();
}

/**
 * Determine whether the binning provided is any good.
 * @param binning : Binning property
 * @return : string containing error. Or empty for no error.
 */
std::string checkBinning(const std::vector<double>& binning)
{
    std::string error; // No error.
    if(!emptyBinning(binning) && binning.size() != 2)
    {
       error = "You may only integrate out dimensions between limits.";
    }
    else if(binning.size() == 2)
    {
        auto min = binning[0];
        auto max = binning[1];
        if(min >= max) {
            error = "min must be < max limit for binning";
        }
    }
    return error;
}

/**
 * Create the output workspace in the right shape.
 * @param inWS : Input workspace for dimensionality
 * @param pbins : User provided binning
 * @return
 */
MDHistoWorkspace_sptr createShapedOutput(IMDHistoWorkspace const * const inWS, std::vector<std::vector<double>> pbins)
{
    const size_t nDims = inWS->getNumDims();
    std::vector<Mantid::Geometry::IMDDimension_sptr> dimensions(nDims);
    for(size_t i = 0; i < nDims; ++i) {

        IMDDimension_const_sptr inDim = inWS->getDimension(i);
        auto outDim = boost::make_shared<MDHistoDimension>(inDim.get());
        // Apply dimensions as inputs.
        if(i < pbins.size() && !emptyBinning(pbins[i]))
        {
            auto binning = pbins[i];
            outDim->setRange(1 /*single bin*/, static_cast<Mantid::coord_t>(binning.front()) /*min*/, static_cast<Mantid::coord_t>(binning.back()) /*max*/); // Set custom min, max and nbins.
        }
        dimensions[i] = outDim;
    }
    return MDHistoWorkspace_sptr(new MDHistoWorkspace(dimensions));
}

}

namespace Mantid
{
namespace MDAlgorithms
{

  using Mantid::Kernel::Direction;
  using Mantid::API::WorkspaceProperty;

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(IntegrateMDHistoWorkspace)



  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  IntegrateMDHistoWorkspace::IntegrateMDHistoWorkspace()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  IntegrateMDHistoWorkspace::~IntegrateMDHistoWorkspace()
  {
  }


  //----------------------------------------------------------------------------------------------

  /// Algorithms name for identification. @see Algorithm::name
  const std::string IntegrateMDHistoWorkspace::name() const { return "IntegrateMDHistoWorkspace"; }

  /// Algorithm's version for identification. @see Algorithm::version
  int IntegrateMDHistoWorkspace::version() const { return 1;};

  /// Algorithm's category for identification. @see Algorithm::category
  const std::string IntegrateMDHistoWorkspace::category() const { return "MDAlgorithms";}

  /// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
  const std::string IntegrateMDHistoWorkspace::summary() const { return "Performs axis aligned integration of MDHistoWorkspaces";}

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void IntegrateMDHistoWorkspace::init()
  {
    declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("InputWorkspace","",Direction::Input), "An input workspace.");

    const std::vector<double> defaultBinning;
    declareProperty(new ArrayProperty<double>("P1Bin", defaultBinning), "Projection 1 binning.");
    declareProperty(new ArrayProperty<double>("P2Bin", defaultBinning), "Projection 2 binning.");
    declareProperty(new ArrayProperty<double>("P3Bin", defaultBinning), "Projection 3 binning.");
    declareProperty(new ArrayProperty<double>("P4Bin", defaultBinning), "Projection 4 binning.");
    declareProperty(new ArrayProperty<double>("P5Bin", defaultBinning), "Projection 5 binning.");

    declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void IntegrateMDHistoWorkspace::exec()
  {
      IMDHistoWorkspace_sptr  inWS = this->getProperty("InputWorkspace");
      const size_t nDims = inWS->getNumDims();
      std::vector<std::vector<double>> pbins(5);
      pbins[0] = this->getProperty("P1Bin");
      pbins[1] = this->getProperty("P2Bin");
      pbins[2] = this->getProperty("P3Bin");
      pbins[3] = this->getProperty("P4Bin");
      pbins[4] = this->getProperty("P5Bin");

      IMDHistoWorkspace_sptr outWS;
      const size_t emptyCount  = std::count_if(pbins.begin(), pbins.end(), emptyBinning);
      if (emptyCount == pbins.size()) {
          // No work to do.
          g_log.information(this->name() + " Direct clone of input.");
          outWS = inWS->clone();
      }
      else {

          // Create the output workspace in the right shape. Required for iteration.
          outWS = createShapedOutput(inWS.get(), pbins);

          auto temp = outWS->getDimension(0)->getNBins();
          auto temp1 = inWS->getDimension(0)->getNBins();

          // Store in each dimension
          std::vector<Mantid::coord_t> binWidths(nDims);
          for(size_t i = 0; i < nDims; ++i) {
              binWidths[i] = outWS->getDimension(i)->getBinWidth();
          }

          boost::scoped_ptr<MDHistoWorkspaceIterator> outIterator(
              dynamic_cast<MDHistoWorkspaceIterator *>(outWS->createIterator()));

          boost::scoped_ptr<MDHistoWorkspaceIterator> inIterator(
              dynamic_cast<MDHistoWorkspaceIterator *>(inWS->createIterator()));

          do {
              // Getting the center will allow us to calculate only local neighbours to consider.
              Mantid::Kernel::VMD iteratorCenter = outIterator->getCenter();

              std::vector<Mantid::coord_t> mins(nDims);
              std::vector<Mantid::coord_t> maxs(nDims);
              for(size_t i = 0; i < nDims; ++i) {
                  const coord_t delta = binWidths[i]/2;
                  mins[i] = iteratorCenter[i] - delta;
                  maxs[i] = iteratorCenter[i] + delta;
              }
              MDBoxImplicitFunction box(mins, maxs);

              /* TODO a better approach to the nested loop would be to
               1) Find the closest inIterator position to the outIterator position.
               2) Move the inIterator to that position
               3) Using the ratio of outWidth[d]/inWidth[2] calculate the width in pixels of neighbours the inIterator would need to look at to cover the same region in n-d
               space as the outIterator.
               4) use inIterator->findNeighboursByWidth to get those indexes
               5) Apply the MDBoxImplicitFunction ONLY over those bins of the input workspace rather that the whole workspace.
               */

              double sumSignal = 0;
              double sumSQErrors = 0;
              const size_t iteratorIndex = outIterator->getLinearIndex();
              do {
                  size_t numVertices;
                  Mantid::coord_t* const vertexes = inIterator->getVertexesArray(numVertices);
                  const double weight = box.fraction(vertexes, numVertices); // TODO not ideal because vertices need to be sorted to get min max in this function. could we calculate this better?
                  sumSignal += weight *  inIterator->getSignal();
                  sumSQErrors += weight * (inIterator->getError() * inIterator->getError());
                  std::cout << "index " << inIterator->getLinearIndex() << " x " << weight << std::endl;
              } while(inIterator->next());

              std::cout << "outer index " << iteratorIndex << std::endl << std::endl;


              outWS->setSignalAt(iteratorIndex, sumSignal);
              outWS->setErrorSquaredAt(iteratorIndex, sumSQErrors);

          } while(outIterator->next());

      }

      this->setProperty("OutputWorkspace", outWS);
  }

  /**
   * Overriden validate inputs
   * @return map of property names to problems for bad inputs
   */
  std::map<std::string, std::string> Mantid::MDAlgorithms::IntegrateMDHistoWorkspace::validateInputs()
  {
      // Check binning parameters
      std::map<std::string, std::string> errors;

      for(int i = 1; i < 6; ++i) {
          std::stringstream propBuffer;
          propBuffer << "P" << i << "Bin";
          std::string propertyName = propBuffer.str();
          std::vector<double> binning = this->getProperty(propertyName);
          std::string result = checkBinning(binning);
          if(!result.empty()) {
              errors.insert(std::make_pair(propertyName, result));
          }
      }
      return errors;
  }



} // namespace MDAlgorithms
} // namespace Mantid
