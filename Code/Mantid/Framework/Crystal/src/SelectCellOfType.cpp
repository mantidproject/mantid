/*WIKI* 


Given a PeaksWorkspace with a UB matrix corresponding to a Niggli reduced cell,
this algorithm will allow the user to select a conventional cell with a
specified cell type and centering.  If the apply flag is not set, the 
information about the selected cell will just be displayed.  If the apply
flag is set, the UB matrix associated with the sample in the PeaksWorkspace
will be updated to a UB corresponding to the selected cell AND the peaks will
be re-indexed using the new UB matrix.  NOTE: The possible conventional cells, 
together with the corresponding errors in the cell scalars can be seen by 
running the ShowPossibleCells algorithm, provided the stored UB matrix 
corresponds to a Niggli reduced cell.

This algorithm is based on the paper: "Lattice Symmetry and Identification 
-- The Fundamental Role of Reduced Cells in Materials Characterization", 
Alan D. Mighell, Vol. 106, Number 6, Nov-Dec 2001, Journal of Research of 
the National Institute of Standards and Technology, available from: 
nvlpubs.nist.gov/nistpubs/jres/106/6/j66mig.pdf.


*WIKI*/
#include "MantidCrystal/SelectCellOfType.h"
#include "MantidCrystal/IndexPeaks.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/ScalarUtils.h"
#include "MantidGeometry/Crystal/ReducedCell.h"
#include "MantidGeometry/Crystal/ConventionalCell.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/ListValidator.h"
#include <boost/lexical_cast.hpp>
#include <cstdio>

namespace Mantid
{
namespace Crystal
{
  Kernel::Logger& SelectCellOfType::g_log = 
                                      Kernel::Logger::get("SelectCellOfType");

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SelectCellOfType)

  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  using namespace Mantid::DataObjects;
  using namespace Mantid::Geometry;

  //--------------------------------------------------------------------------
  /** Constructor
   */
  SelectCellOfType::SelectCellOfType()
  {
  }
    
  //--------------------------------------------------------------------------
  /** Destructor
   */
  SelectCellOfType::~SelectCellOfType()
  {
  }

  //--------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void SelectCellOfType::initDocs()
  {
    std::string summary("Select a conventional cell with a specific ");
    summary += "lattice type and centering, corresponding to the UB ";
    summary += "stored with the sample for this peaks works space.";
    this->setWikiSummary( summary );

    std::string message("NOTE: The current UB must correspond to a ");
    message += "Niggli reduced cell.";
    this->setOptionalMessage(message);
  }

  //--------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SelectCellOfType::init()
  {
    this->declareProperty(new WorkspaceProperty<PeaksWorkspace>(
          "PeaksWorkspace","",Direction::InOut), "Input Peaks Workspace");

    std::vector< std::string > type_list;
    type_list.push_back( ReducedCell::CUBIC() );
    type_list.push_back( ReducedCell::HEXAGONAL() );
    type_list.push_back( ReducedCell::RHOMBOHEDRAL() );
    type_list.push_back( ReducedCell::TETRAGONAL() );
    type_list.push_back( ReducedCell::ORTHORHOMBIC() );
    type_list.push_back( ReducedCell::MONOCLINIC() );
    type_list.push_back( ReducedCell::TRICLINIC() );

    declareProperty("CellType", type_list[0],
                    boost::make_shared<Kernel::StringListValidator>(type_list),
                    "The conventional cell type to use");

    std::vector< std::string > centering_list;
    centering_list.push_back( ReducedCell::F_CENTERED() );
    centering_list.push_back( ReducedCell::I_CENTERED() );
    centering_list.push_back( ReducedCell::C_CENTERED() );
    centering_list.push_back( ReducedCell::P_CENTERED() );
    centering_list.push_back( ReducedCell::R_CENTERED() );

    declareProperty("Centering", centering_list[3],
                    boost::make_shared<Kernel::StringListValidator>(centering_list),
                    "The centering for the conventional cell");

    this->declareProperty( "Apply", false, "Update UB and re-index the peaks");
    this->declareProperty( "Tolerance", 0.12, "Indexing Tolerance");

    this->declareProperty(new PropertyWithValue<int>( "NumIndexed", 0,
          Direction::Output), "The number of indexed peaks if apply==true.");

    this->declareProperty(new PropertyWithValue<double>( "AverageError", 0.0,
          Direction::Output), "The average HKL indexing error if apply==true.");
  }

  //--------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SelectCellOfType::exec()
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

    std::string cell_type = this->getProperty("CellType");
    std::string centering = this->getProperty("Centering");
    bool   apply          = this->getProperty("Apply");
    double tolerance      = this->getProperty("Tolerance");

    std::vector<ConventionalCell> list = 
                          ScalarUtils::GetCells( UB, cell_type, centering );

    ConventionalCell info = ScalarUtils::GetCellBestError( list, true );

    DblMatrix newUB = info.GetNewUB();

    std::string message = info.GetDescription() + " Lat Par:" +
                          IndexingUtils::GetLatticeParameterString( newUB );

    g_log.notice( std::string(message) );




    if ( apply )
    {
       bool latErrorsValid=true;
      //----------------------------------- Try to optimize(LSQ) to find lattice errors ------------------------
      //                       UB matrix may NOT have been found by unconstrained least squares optimization
       std::vector<double> sigabc(7);
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
       IndexingUtils::GetIndexedPeaks(newUB, q_vectors0, tolerance,
                           miller_ind, q_vectors, fit_error );
       IndexingUtils::Optimize_UB(newUB1, miller_ind,q_vectors,sigabc);

       int nindexed_old = (int)q_vectors.size();
       int nindexed_new = IndexingUtils::NumberIndexed( newUB1,q_vectors0,tolerance);
       if( nindexed_old<.8*nindexed_new || .8*nindexed_old>  nindexed_new )
          latErrorsValid = false;
       else
       {
         double maxDiff=0;
         double maxEntry =0;
         for( int row=0; row <3; row++)
           for( int col=0; col< 3; col++)
           {
             double diff= fabs( newUB[row][col]- newUB1[row][col]);
             double V = std::max<double>(fabs(newUB[row][col]), fabs(newUB1[row][col]));
             if( diff > maxDiff)
               maxDiff = diff;
             if(V > maxEntry)
               maxEntry = V;
           }
         if( maxEntry==0 || maxDiff/maxEntry >.1)
           latErrorsValid=false;
         else
           newUB = newUB1;

       }


       //----------------------------------------------
      o_lattice.setUB( newUB );
      if( latErrorsValid)
         o_lattice.setError( sigabc[0],sigabc[1],sigabc[2],sigabc[3],sigabc[4],sigabc[5]);

      ws->mutableSample().setOrientedLattice( new OrientedLattice(o_lattice) ); 

      std::vector<Peak> &peaks = ws->getPeaks();
      size_t n_peaks = ws->getNumberPeaks();

                                       // transform the HKLs and record the new HKL
                                       // and q-vectors for peaks ORIGINALLY indexed
      DblMatrix hkl_tran = info.GetHKL_Tran(); 
      int num_indexed = 0;
      std::vector<V3D> miller_indices;
      q_vectors.clear();
      for ( size_t i = 0; i < n_peaks; i++ )
      {
        V3D hkl( peaks[i].getHKL() );
        if ( IndexingUtils::ValidIndex(hkl,tolerance ) )
        {
          num_indexed++;
          miller_indices.push_back( hkl_tran * hkl );
          q_vectors.push_back( peaks[i].getQSampleFrame() );
          peaks[i].setHKL( hkl_tran * hkl );
        }
        else                            // mark as NOT indexed
          peaks[i].setHKL( V3D(0.0,0.0,0.0) );
      }

      double average_error = IndexingUtils::IndexingError( newUB, miller_indices, q_vectors );

      // Tell the user what happened.
      g_log.notice() << "Transformed Miller indices on previously valid indexed Peaks. " << std::endl;   
      g_log.notice() << "Set hkl to 0,0,0 on peaks previously indexed out of tolerance. " << std::endl;   
      g_log.notice() << "Now, " << num_indexed << " are indexed with average error " << average_error << std::endl;

      // Save output properties
      this->setProperty("NumIndexed", num_indexed);
      this->setProperty("AverageError", average_error);
    }
  }

} // namespace Mantid
} // namespace Crystal

