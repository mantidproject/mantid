#include "MantidCrystal/IndexPeaks.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
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

    BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
    mustBePositive->setLower(0.0);

    this->declareProperty(new PropertyWithValue<double>( "tolerance",0.15,
          mustBePositive,Direction::Input),"Indexing Tolerance (0.15)");
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

    double tolerance = this->getProperty("tolerance");
    double average_error;

    int num_indexed = IndexingUtils::CalculateMillerIndices( UB, q_vectors, 
                                                             tolerance, 
                                                             miller_indices,
                                                             average_error );

                                   // now tell the user how many were indexed
    char logInfo[200];
    sprintf( logInfo, 
             std::string("Indexed %1d Peaks out of %1d with tolerance %5.3f").c_str(),    
             num_indexed, n_peaks, tolerance );
    g_log.notice( std::string(logInfo) );

    sprintf( logInfo,
             std::string("Average error in h,k,l for indexed peaks = %6.4f").c_str(),
             average_error);
    g_log.notice( std::string(logInfo) );

    for ( size_t i = 0; i < n_peaks; i++ )
    {
      peaks[i].setHKL( miller_indices[i] );
    } 
  }


} // namespace Mantid
} // namespace Crystal

