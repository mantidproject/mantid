/*WIKI* 


Given a set of peaks, and given lattice parameters (<math>a,b,c,alpha,beta,gamma</math>), this algorithm will find the UB matrix, that best fits the data.  The algorithm searches over a large range of possible orientations for the orientation for which the rotated B matrix best fits the data.  It then uses a least squares approach to optimize the complete UB matrix.


*WIKI*/
#include "MantidCrystal/FindUBUsingLatticeParameters.h"
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
  Kernel::Logger& FindUBUsingLatticeParameters::g_log = 
                        Kernel::Logger::get("FindUBUsingLatticeParameters");

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(FindUBUsingLatticeParameters)

  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  using namespace Mantid::DataObjects;
  using namespace Mantid::Geometry;

  //--------------------------------------------------------------------------
  /** Constructor
   */
  FindUBUsingLatticeParameters::FindUBUsingLatticeParameters()
  {
  }
    
  //--------------------------------------------------------------------------
  /** Destructor
   */
  FindUBUsingLatticeParameters::~FindUBUsingLatticeParameters()
  {
  }

  //--------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void FindUBUsingLatticeParameters::initDocs()
  {
    std::string summary("Calculate the UB matrix from a peaks workspace, ");
    summary += "given lattice parameters.";
    this->setWikiSummary( summary );

    std::string message("Calculate the UB matrix from a peaks workspace, ");
    message += "given lattice parameters.";
    this->setOptionalMessage( message );
  }

  //--------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void FindUBUsingLatticeParameters::init()
  {
    this->declareProperty(new WorkspaceProperty<PeaksWorkspace>(
          "PeaksWorkspace","",Direction::InOut), "Input Peaks Workspace");

    BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
    mustBePositive->setLower(0.0);

    BoundedValidator<int> *moreThan2Int = new BoundedValidator<int>();
    moreThan2Int->setLower(2);

    BoundedValidator<double> *reasonable_angle = new BoundedValidator<double>();
    reasonable_angle->setLower(5.0);
    reasonable_angle->setUpper(175.0);

    // use negative values, force user to input all parameters
    this->declareProperty(new PropertyWithValue<double>( "a",-1.0,
          mustBePositive,Direction::Input),"Lattice parameter a");

    this->declareProperty(new PropertyWithValue<double>( "b",-1.0,
          mustBePositive->clone(),Direction::Input),"Lattice parameter b");

    this->declareProperty(new PropertyWithValue<double>( "c",-1.0,
          mustBePositive->clone(),Direction::Input),"Lattice parameter c");

    this->declareProperty(new PropertyWithValue<double>( "alpha",-1.0,
          reasonable_angle,Direction::Input),"Lattice parameter alpha");

    this->declareProperty(new PropertyWithValue<double>("beta",-1.0,
          reasonable_angle->clone(),Direction::Input),"Lattice parameter beta");

    this->declareProperty(new PropertyWithValue<double>("gamma",-1.0,
          reasonable_angle->clone(),Direction::Input),"Lattice parameter gamma");

    this->declareProperty(new PropertyWithValue<int>( "NumInitial", 15,
          moreThan2Int,Direction::Input), 
          "Number of Peaks to Use on First Pass(15)");

    this->declareProperty(new PropertyWithValue<double>( "Tolerance",0.15,
          mustBePositive->clone(),Direction::Input),"Indexing Tolerance (0.15)");
  }

  //--------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void FindUBUsingLatticeParameters::exec()
  {
    double a           = this->getProperty("a");
    double b           = this->getProperty("b");
    double c           = this->getProperty("c");
    double alpha       = this->getProperty("alpha");
    double beta        = this->getProperty("beta");
    double gamma       = this->getProperty("gamma");
    int    num_initial = this->getProperty("NumInitial");
    double tolerance   = this->getProperty("Tolerance");
                                          
    int    base_index         = -1;   // these "could" be properties if need be
    double degrees_per_step   = 1.5;

    PeaksWorkspace_sptr ws;
    ws = boost::dynamic_pointer_cast<PeaksWorkspace>(
         AnalysisDataService::Instance().retrieve(this->getProperty("PeaksWorkspace")) );

    if (!ws) throw std::runtime_error("Could not read the peaks workspace");

    std::vector<Peak> &peaks = ws->getPeaks();
    size_t n_peaks = ws->getNumberPeaks();

    std::vector<V3D>  q_vectors;
    q_vectors.reserve( n_peaks );
    for ( size_t i = 0; i < n_peaks; i++ )
      q_vectors.push_back( peaks[i].getQSampleFrame() );

    Matrix<double> UB(3,3,false);
    double error = IndexingUtils::Find_UB( UB, q_vectors, 
                                           a, b, c, alpha, beta, gamma,
                                           tolerance, 
                                           base_index, 
                                           num_initial, 
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
    else                                 // tell user how many would be indexed
    {                                    // and save the UB in the sample 
      char logInfo[200];
      int num_indexed = IndexingUtils::NumberIndexed(UB, q_vectors, tolerance);
      sprintf( logInfo,
               std::string("New UB will index %1d Peaks out of %1d with tolerance %5.3f").c_str(),
               num_indexed, n_peaks, tolerance);
      g_log.notice( std::string(logInfo) );

      OrientedLattice o_lattice;
      o_lattice.setUB( UB );
      double calc_a = o_lattice.a();
      double calc_b = o_lattice.b();
      double calc_c = o_lattice.c();
      double calc_alpha = o_lattice.alpha();
      double calc_beta  = o_lattice.beta();
      double calc_gamma = o_lattice.gamma();
                                        // Show the modified lattice parameters
      sprintf( logInfo, 
               std::string("Lattice Parameters: %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f").c_str(),
               calc_a, calc_b, calc_c, calc_alpha, calc_beta, calc_gamma);
      g_log.notice( std::string(logInfo) );
      sprintf( logInfo, 
               std::string("Lattice Parameters (Refined - Input): %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f").c_str(),
               calc_a-a, calc_b-b, calc_c-c, calc_alpha-alpha, calc_beta-beta, calc_gamma-gamma);
      g_log.notice( std::string(logInfo) );

      ws->mutableSample().setOrientedLattice( new OrientedLattice(o_lattice) );
    }
  }


} // namespace Mantid
} // namespace Crystal

