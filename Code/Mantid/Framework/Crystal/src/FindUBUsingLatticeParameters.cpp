/*WIKI* 

Given a set of peaks, and given lattice parameters (<math>a,b,c,alpha,beta,gamma</math>), this algorithm
will find the UB matrix, that best fits the data.  The algorithm searches over a large range of possible
orientations for the orientation for which the rotated B matrix best fits the data.  The search for the
best orientation involves several steps. 

During the first step, a reduced set of peaks typically at lower |Q| are used, since it is easier 
to index peaks at low |Q|.  Specifically, if there are at least 5 peaks, the peaks are shifted to 
be centered at the strongest peaks and then sorted in order of increasing distance from the 
strongest peak.  If there are fewer than 5 peaks the list is just sorted in order of increasing |Q|.  
Only peaks from the initial portion of this sorted list are used in the first step.  The number of 
peaks from this list to be used initially is specified by the user with the parameter NumInitial. 
The search first finds a list of possible orientations for which the UB matrix will index the 
maximum number of peaks from the initial set of peaks to within the specified tolerance on h,k,l values.  
Subsequently, only the UB matrix that indexes that maximum number of peaks with the minimum distance 
between the calculated h,k,l values and integers is kept and passed on to the second step.

During the second step, additional peaks are gradually added to the initial list of peaks.  Each time
peaks are added to the list, the subset of peaks from the new list that are indexed within the specified
tolerance on k,k,l are used in a least squares calculation to optimize the UB matrix to best index those 
peaks.  The process of gradually adding more peaks from the sorted list and optimizing the 
UB based on the peaks that are indexed, continues until all peaks have been added to the list.  
Finally, one last optimization of the UB matrix is carried out using the full list of peaks.


*WIKI*/
#include "MantidCrystal/FindUBUsingLatticeParameters.h"
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

    boost::shared_ptr<BoundedValidator<double> > mustBePositive(new BoundedValidator<double>());
    mustBePositive->setLower(0.0);

    boost::shared_ptr<BoundedValidator<int> > moreThan2Int(new BoundedValidator<int>());
    moreThan2Int->setLower(2);

    boost::shared_ptr<BoundedValidator<double> > reasonable_angle(new BoundedValidator<double>());
    reasonable_angle->setLower(5.0);
    reasonable_angle->setUpper(175.0);

    // use negative values, force user to input all parameters
    this->declareProperty("a",-1.0, mustBePositive, "Lattice parameter a");
    this->declareProperty("b",-1.0, mustBePositive, "Lattice parameter b");
    this->declareProperty("c",-1.0, mustBePositive, "Lattice parameter c");
    this->declareProperty("alpha",-1.0,reasonable_angle,"Lattice parameter alpha");
    this->declareProperty("beta",-1.0, reasonable_angle,"Lattice parameter beta");
    this->declareProperty("gamma",-1.0,reasonable_angle,"Lattice parameter gamma");
    this->declareProperty("NumInitial", 15, moreThan2Int, "Number of Peaks to Use on First Pass(15)");
    this->declareProperty("Tolerance",0.15, mustBePositive,"Indexing Tolerance (0.15)");
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

      for (size_t i = 0; i < n_peaks; i++)
        q_vectors.push_back(peaks[i].getQSampleFrame());

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

        std::vector<double> sigabc(7);
        std::vector<V3D> miller_ind;
        std::vector<V3D> indexed_qs;
        double fit_error;
        miller_ind.reserve( q_vectors.size() );
        indexed_qs.reserve( q_vectors.size() );
        IndexingUtils::GetIndexedPeaks( UB, q_vectors, tolerance,
                               miller_ind, indexed_qs, fit_error );

        //IndexingUtils::Optimize_UB(UB, miller_ind,indexed_qs,sigabc);

        char logInfo[200];
        int num_indexed = IndexingUtils::NumberIndexed(UB, q_vectors, tolerance);
        sprintf(logInfo,
            std::string("New UB will index %1d Peaks out of %1d with tolerance %5.3f").c_str(),
            num_indexed, n_peaks, tolerance);
        g_log.notice(std::string(logInfo));

        OrientedLattice o_lattice;
        o_lattice.setUB(UB);
        o_lattice.setError(sigabc[0], sigabc[1], sigabc[2], sigabc[3], sigabc[4], sigabc[5]);

        o_lattice.setUB(UB);
        double calc_a = o_lattice.a();
        double calc_b = o_lattice.b();
        double calc_c = o_lattice.c();
        double calc_alpha = o_lattice.alpha();
        double calc_beta = o_lattice.beta();
        double calc_gamma = o_lattice.gamma();
        // Show the modified lattice parameters
        sprintf(logInfo, std::string("Lattice Parameters: %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f").c_str(),
            calc_a, calc_b, calc_c, calc_alpha, calc_beta, calc_gamma);
        g_log.notice(std::string(logInfo));


        g_log.notice()<<"Parameter Errors  :"<<std::fixed<<std::setprecision(3)<<std::setw(9)<<sigabc[0]
                                              <<std::fixed<<std::setprecision(3)<<std::setw(9)<<sigabc[1]
                                              <<std::fixed<<std::setprecision(3)<<std::setw(9)<<sigabc[2]
                                              <<std::fixed<<std::setprecision(3)<<std::setw(9)<<sigabc[3]
                                              <<std::fixed<<std::setprecision(3)<<std::setw(9)<<sigabc[4]
                                              <<std::fixed<<std::setprecision(3)<<std::setw(9)<<sigabc[5]
                                              <<std::endl;

        sprintf(logInfo,
            std::string("Lattice Parameters (Refined - Input): %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f").c_str(),
            calc_a - a, calc_b - b, calc_c - c, calc_alpha - alpha, calc_beta - beta,
            calc_gamma - gamma);
        g_log.notice(std::string(logInfo));
        ws->mutableSample().setOrientedLattice(new OrientedLattice(o_lattice));
      }
  }


} // namespace Mantid
} // namespace Crystal

