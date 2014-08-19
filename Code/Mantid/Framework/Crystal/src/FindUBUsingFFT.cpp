#include "MantidCrystal/FindUBUsingFFT.h"
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
  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(FindUBUsingFFT)

  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  using namespace Mantid::DataObjects;
  using namespace Mantid::Geometry;

  //--------------------------------------------------------------------------
  /** Constructor
   */
  FindUBUsingFFT::FindUBUsingFFT()
  {
  }
    
  //--------------------------------------------------------------------------
  /** Destructor
   */
  FindUBUsingFFT::~FindUBUsingFFT()
  {
  }

  const std::string FindUBUsingFFT::name() const
  {
    return "FindUBUsingFFT";
  }

  int FindUBUsingFFT::version() const
  {
    return 1;
  }

  const std::string FindUBUsingFFT::category() const
  {
    return "Crystal";
  }


  //--------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void FindUBUsingFFT::init()
  {
    this->declareProperty(new WorkspaceProperty<PeaksWorkspace>(
          "PeaksWorkspace","",Direction::InOut), "Input Peaks Workspace");

    boost::shared_ptr<BoundedValidator<double> > mustBePositive(new BoundedValidator<double>());
    mustBePositive->setLower(0.0);

    // use negative values, force user to input all parameters
    this->declareProperty("MinD",-1.0, mustBePositive, "Lower Bound on Lattice Parameters a, b, c");
    this->declareProperty("MaxD",-1.0, mustBePositive, "Upper Bound on Lattice Parameters a, b, c");
    this->declareProperty("Tolerance",0.15, mustBePositive, "Indexing Tolerance (0.15)");
  }


  //--------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void FindUBUsingFFT::exec()
  {
    double min_d       = this->getProperty("MinD");
    double max_d       = this->getProperty("MaxD");
    double tolerance   = this->getProperty("Tolerance");
                                          
    double degrees_per_step =  1.5;

    PeaksWorkspace_sptr ws = this->getProperty("PeaksWorkspace");

    const std::vector<Peak> &peaks = ws->getPeaks();
    size_t n_peaks = ws->getNumberPeaks();

    std::vector<V3D>  q_vectors;
    for ( size_t i = 0; i < n_peaks; i++ )
    {
      q_vectors.push_back( peaks[i].getQSampleFrame() );
    }

    Matrix<double> UB(3,3,false);
    double error = IndexingUtils::Find_UB( UB, q_vectors, 
                                           min_d, max_d,
                                           tolerance, 
                                           degrees_per_step );

    g_log.notice() << "Error = " << error << std::endl;
    g_log.notice() << "UB = " << UB << std::endl;

    if ( ! IndexingUtils::CheckUB( UB ) ) // UB not found correctly
    {
      g_log.notice( std::string(
         "Found Invalid UB...peaks used might not be linearly independent") );
      g_log.notice( std::string(
         "UB NOT SAVED.") );
    }
    else                                  // tell user how many would be indexed
    {
      std::vector<double> sigabc(7);
      std::vector<V3D> miller_ind;
      std::vector<V3D> indexed_qs;
      double fit_error;
      miller_ind.reserve( q_vectors.size() );
      indexed_qs.reserve( q_vectors.size() );
      IndexingUtils::GetIndexedPeaks( UB, q_vectors, tolerance,
                          miller_ind, indexed_qs, fit_error );
      IndexingUtils::Optimize_UB(UB, miller_ind,indexed_qs,sigabc);

     int num_indexed = IndexingUtils::NumberIndexed(UB, q_vectors, tolerance);
     g_log.notice() << "New UB will index " << num_indexed << " Peaks out of " << n_peaks
                    << " with tolerance of " << std::setprecision(3) << std::setw(5) << tolerance
                    << "\n";

      OrientedLattice o_lattice;
      o_lattice.setUB( UB );
      o_lattice.setError(sigabc[0],sigabc[1],sigabc[2],sigabc[3],sigabc[4],sigabc[5]);


      // Show the modified lattice parameters
      g_log.notice() << o_lattice << "\n";


      ws->mutableSample().setOrientedLattice( &o_lattice );
    }
  }


} // namespace Mantid
} // namespace Crystal
