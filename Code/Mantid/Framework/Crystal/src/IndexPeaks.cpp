/*WIKI* 


Given a PeaksWorkspace with a UB matrix stored with the sample, this algorithm will use UB inverse to index the peaks.
Any peak with any Miller index more than the specified tolerance away from an integer will have its (h,k,l) set to (0,0,0).


*WIKI*/
#include "MantidCrystal/IndexPeaks.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/BoundedValidator.h"
#include <cstdio>

namespace Mantid
{
namespace Crystal
{
  Kernel::Logger& IndexPeaks::g_log = Kernel::Logger::get("IndexPeaks");

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(IndexPeaks)

  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  using namespace Mantid::DataObjects;
  using namespace Mantid::Geometry;

  //--------------------------------------------------------------------------
  /** Constructor
   */
  IndexPeaks::IndexPeaks()
  {
  }
    
  //--------------------------------------------------------------------------
  /** Destructor
   */
  IndexPeaks::~IndexPeaks()
  {
  }

  //--------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void IndexPeaks::initDocs()
  {
    std::string summary("Index the peaks in the PeaksWorkspace using the UB ");
    summary += "stored with the sample.";
    this->setWikiSummary( summary );

    this->setOptionalMessage("Index the peaks using the UB from the sample.");
  }

  //--------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void IndexPeaks::init()
  {
    this->declareProperty(new WorkspaceProperty<PeaksWorkspace>(
          "PeaksWorkspace","",Direction::InOut), "Input Peaks Workspace");

    boost::shared_ptr<BoundedValidator<double> > mustBePositive(new BoundedValidator<double>());
    mustBePositive->setLower(0.0);

    this->declareProperty(new PropertyWithValue<double>( "Tolerance",0.15,
          mustBePositive,Direction::Input),"Indexing Tolerance (0.15)");

    this->declareProperty(new PropertyWithValue<int>( "NumIndexed", 0,
          Direction::Output), "Gets set with the number of indexed peaks.");

    this->declareProperty(new PropertyWithValue<double>( "AverageError", 0.0,
          Direction::Output), "Gets set with the average HKL indexing error.");
  }

  //--------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void IndexPeaks::exec()
  {
                                          
    PeaksWorkspace_sptr ws;
    ws = boost::dynamic_pointer_cast<PeaksWorkspace>(
         AnalysisDataService::Instance().retrieve(this->getProperty("PeaksWorkspace")) );

    if (!ws) 
    { 
      throw std::runtime_error("Could not read the peaks workspace");
    }

    OrientedLattice o_lattice = ws->mutableSample().getOrientedLattice();
    Matrix<double> UB = o_lattice.getUB();

    if ( ! IndexingUtils::CheckUB( UB ) )
    {
       throw std::runtime_error(
             "ERROR: The stored UB is not a valid orientation matrix");
    }

    std::vector<Peak> &peaks = ws->getPeaks();
    size_t n_peaks = ws->getNumberPeaks();

    std::vector<V3D> miller_indices;
    std::vector<V3D> q_vectors;

    q_vectors.reserve( n_peaks );
    for ( size_t i = 0; i < n_peaks; i++ )
    {
      q_vectors.push_back( peaks[i].getQSampleFrame() );
    }

    double tolerance = this->getProperty("Tolerance");
    double average_error;

    int num_indexed = IndexingUtils::CalculateMillerIndices( UB, q_vectors, 
                                                             tolerance, 
                                                             miller_indices,
                                                             average_error );

    // now tell the user how many were indexed
    g_log.notice() << "Indexed " << num_indexed << " Peaks out of " << n_peaks << " with tolerance of " << tolerance << std::endl;
    g_log.notice() << "Average error in h,k,l for indexed peaks =  " << average_error << std::endl;

    for ( size_t i = 0; i < n_peaks; i++ )
      peaks[i].setHKL( miller_indices[i] );

    // Save output properties
    this->setProperty("NumIndexed", num_indexed);
    this->setProperty("AverageError", average_error);
  }


} // namespace Mantid
} // namespace Crystal

