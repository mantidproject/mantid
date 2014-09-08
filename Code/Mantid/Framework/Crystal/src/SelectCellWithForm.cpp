#include "MantidCrystal/SelectCellWithForm.h"
#include "MantidCrystal/IndexPeaks.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/ScalarUtils.h"
#include "MantidGeometry/Crystal/ReducedCell.h"
#include "MantidGeometry/Crystal/ConventionalCell.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/BoundedValidator.h"
#include <boost/lexical_cast.hpp>
#include <cstdio>

namespace Mantid
{
namespace Crystal
{
  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SelectCellWithForm)

  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  using namespace Mantid::DataObjects;
  using namespace Mantid::Geometry;

  //--------------------------------------------------------------------------
  /** Constructor
   */
  SelectCellWithForm::SelectCellWithForm()
  {
  }
    
  //--------------------------------------------------------------------------
  /** Destructor
   */
  SelectCellWithForm::~SelectCellWithForm()
  {
  }


  //--------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SelectCellWithForm::init()
  {
    this->declareProperty(new WorkspaceProperty<PeaksWorkspace>(
          "PeaksWorkspace","",Direction::InOut), "Input Peaks Workspace");

    auto mustBePositive = boost::make_shared<BoundedValidator<int> >();
    mustBePositive->setLower(1);

    this->declareProperty(
          new PropertyWithValue<int>("FormNumber",0,mustBePositive,Direction::Input),
         "Form number for the desired cell");
    this->declareProperty( "Apply", false, "Update UB and re-index the peaks");
    this->declareProperty( "Tolerance", 0.12, "Indexing Tolerance");

    this->declareProperty(new PropertyWithValue<int>( "NumIndexed", 0,
          Direction::Output), "The number of indexed peaks if apply==true.");

    this->declareProperty(new PropertyWithValue<double>( "AverageError", 0.0,
          Direction::Output), "The average HKL indexing error if apply==true.");

    this->declareProperty( "AllowPermutations", true,
                            "Allow permutations of conventional cells" );
  }

  Kernel::Matrix<double> SelectCellWithForm::DetermineErrors( std::vector<double> &sigabc, const Kernel::Matrix<double> &UB,
                                             const PeaksWorkspace_sptr &ws, double tolerance)
  {

        std::vector<V3D> miller_ind;
        std::vector<V3D> q_vectors;
        std::vector<V3D>q_vectors0;
        int npeaks = ws->getNumberPeaks();
        double fit_error;
        miller_ind.reserve( npeaks );
        q_vectors.reserve( npeaks );
        q_vectors0.reserve(npeaks);
        for( int i=0;i<npeaks;i++)
          q_vectors0.push_back(ws->getPeak(i).getQSampleFrame());

        Kernel::Matrix<double>newUB1(3,3);
        IndexingUtils::GetIndexedPeaks(UB, q_vectors0, tolerance,
                            miller_ind, q_vectors, fit_error );
        IndexingUtils::Optimize_UB(newUB1, miller_ind,q_vectors,sigabc);

        int nindexed_old = (int)q_vectors.size();
        int nindexed_new = IndexingUtils::NumberIndexed( newUB1,q_vectors0,tolerance);
        bool latErrorsValid =true;
        if( nindexed_old < .8*nindexed_new || .8*nindexed_old >  nindexed_new )
           latErrorsValid = false;
        else
        {
          double maxDiff=0;
          double maxEntry =0;
          for( int row=0; row <3; row++)
            for( int col=0; col< 3; col++)
            {
              double diff= fabs( UB[row][col]- newUB1[row][col]);
              double V = std::max<double>(fabs(UB[row][col]), fabs(newUB1[row][col]));
              if( diff > maxDiff)
                maxDiff = diff;
              if(V > maxEntry)
                maxEntry = V;
            }
          if( maxEntry==0 || maxDiff/maxEntry >.1)
            latErrorsValid=false;
        }

        if( !latErrorsValid)
        {
          for( size_t i = 0; i < sigabc.size(); i++ )
            sigabc[i]=0;
          return UB;

        }else

          return newUB1;

  }

  //--------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SelectCellWithForm::exec()
  {
    PeaksWorkspace_sptr ws = this->getProperty("PeaksWorkspace");
    if (!ws) 
    { 
      throw std::runtime_error("Could not read the peaks workspace");
    }

    OrientedLattice o_lattice = ws->mutableSample().getOrientedLattice();
    Matrix<double> UB = o_lattice.getUB();

    bool   allowPermutations        = this->getProperty("AllowPermutations");

    if ( ! IndexingUtils::CheckUB( UB ) )
    {
       throw std::runtime_error(
             "ERROR: The stored UB is not a valid orientation matrix");
    }

    int    form_num  = this->getProperty("FormNumber");
    bool   apply     = this->getProperty("Apply");
    double tolerance = this->getProperty("Tolerance");

    ConventionalCell info = ScalarUtils::GetCellForForm( UB, form_num, allowPermutations );

    DblMatrix newUB = info.GetNewUB();

    std::string message = info.GetDescription() + " Lat Par:" +
                          IndexingUtils::GetLatticeParameterString( newUB );

    g_log.notice( std::string(message) );

    Kernel::Matrix<double>T(UB);
    T.Invert();
    T = newUB * T;
    g_log.notice() << "Transformation Matrix =  " << T.str() << std::endl;

    if ( apply )
    {
    //----------------------------------- Try to optimize(LSQ) to find lattice errors ------------------------
    //                       UB matrix may NOT have been found by unconstrained least squares optimization

     //----------------------------------------------
    o_lattice.setUB( newUB );
    std::vector<double> sigabc(6);
    DetermineErrors(sigabc,newUB,ws, tolerance);

    o_lattice.setError( sigabc[0],sigabc[1],sigabc[2],sigabc[3],sigabc[4],sigabc[5]);

    ws->mutableSample().setOrientedLattice( &o_lattice );

    std::vector<Peak> &peaks = ws->getPeaks();
    size_t n_peaks = ws->getNumberPeaks();

    int    num_indexed   = 0;
    double average_error = 0.0;
    std::vector<V3D> miller_indices;
    std::vector<V3D> q_vectors;
    for ( size_t i = 0; i < n_peaks; i++ )
    {
      q_vectors.push_back( peaks[i].getQSampleFrame() );
    }

    num_indexed = IndexingUtils::CalculateMillerIndices( newUB, q_vectors,
                                                         tolerance,
                                                         miller_indices,
                                                         average_error );

    for ( size_t i = 0; i < n_peaks; i++ )
    {
      peaks[i].setHKL( miller_indices[i] );
    }

    // Tell the user what happened.
    g_log.notice() << "Re-indexed the peaks with the new UB. " << std::endl;
    g_log.notice() << "Now, " << num_indexed << " are indexed with average error " << average_error << std::endl;

    // Save output properties

    this->setProperty("NumIndexed", num_indexed);
    this->setProperty("AverageError", average_error);
    }
  }


} // namespace Mantid
} // namespace Crystal
