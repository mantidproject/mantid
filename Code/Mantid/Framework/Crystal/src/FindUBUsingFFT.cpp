/*WIKI* 


Given a set of peaks, and given a range of possible a,b,c values, this algorithm will attempt to find a UB matrix, corresponding to the Niggli reduced cell, that fits the data.  
The algorithm projects the peaks on many possible direction vectors and calculates a Fast Fourier Transform of the projections to identify regular patterns in the collection of peaks.  
Based on the calcuated FFTs, a list of directions corresponding to possible real space unit cell edge vectors is formed.  
The directions and lengths of the vectors in this list are optimized (using a least squares approach) to index the maximum number of peaks, after which the list is sorted in order of increasing length and duplicate vectors are removed from the list.

The algorithm then chooses three of the remaining vectors with the shortest lengths that are linearly independent, form a unit cell with at least a minimum volume and for which the corresponding UB matrix indexes at least 80% of the maximum number of indexed using any set of three vectors chosen from the list.

A UB matrix is formed using these three vectors and the resulting UB matrix is again optimized using a least squares method. Finally, starting from this matrix,
a matrix corresponding to the Niggli reduced cell is calculated and returned as the UB matrix.
If the specified peaks are accurate and belong to a single crystal, this method should produce the UB matrix corresponding to the Niggli reduced cell. However, other software will usually be needed to adjust this UB to match a desired conventional cell.
While this algorithm will occasionally work for as few as four peaks, it works quite consistently with at least ten peaks, and in general works best with a larger number of peaks.


*WIKI*/
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
  Kernel::Logger& FindUBUsingFFT::g_log = 
                        Kernel::Logger::get("FindUBUsingFFT");

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
  /// Sets documentation strings for this algorithm
  void FindUBUsingFFT::initDocs()
  {
    std::string summary("Calculate the UB matrix from a peaks workspace, ");
    summary += "given estimates of the min and max real space unit cell ";
    summary += "edge lengths.";
    this->setWikiSummary( summary );

    std::string message("Calculate the UB matrix from a peaks workspace, ");
    message += "given min(a,b,c) and max(a,b,c).";
    this->setOptionalMessage( message );
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

    std::cout << "Error = " << error << std::endl;
    std::cout << "UB = " << UB << std::endl;

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


      ws->mutableSample().setOrientedLattice( new OrientedLattice(o_lattice) );
    }
  }


} // namespace Mantid
} // namespace Crystal

