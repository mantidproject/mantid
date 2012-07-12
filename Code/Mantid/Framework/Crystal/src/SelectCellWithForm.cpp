/*WIKI* 


Given a PeaksWorkspace with a UB matrix corresponding to a Niggli reduced cell,
this algorithm will allow the user to select a conventional cell corresponding
to a specific form number from the Mighell paper.  If the apply flag is not set,
the information about the selected cell will just be displayed.  If the apply
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
  Kernel::Logger& SelectCellWithForm::g_log = 
                                      Kernel::Logger::get("SelectCellWithForm");

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
  /// Sets documentation strings for this algorithm
  void SelectCellWithForm::initDocs()
  {
    std::string summary("Select a conventional cell with a specific ");
    summary += "form number, corresponding to the UB ";
    summary += "stored with the sample for this peaks works space.";
    this->setWikiSummary( summary );

    std::string message("NOTE: The current UB must correspond to a ");
    message += "Niggli reduced cell.";
    this->setOptionalMessage(message);
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
  }

  //--------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SelectCellWithForm::exec()
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

    int    form_num  = this->getProperty("FormNumber");
    bool   apply     = this->getProperty("Apply");
    double tolerance = this->getProperty("Tolerance");

    ConventionalCell info = ScalarUtils::GetCellForForm( UB, form_num );

    DblMatrix newUB = info.GetNewUB();

    std::string message = info.GetDescription() + " Lat Par:" +
                          IndexingUtils::GetLatticeParameterString( newUB );

    g_log.notice( std::string(message) );

    if ( apply )
    {
      o_lattice.setUB( newUB );
      ws->mutableSample().setOrientedLattice( new OrientedLattice(o_lattice) ); 

      std::vector<Peak> &peaks = ws->getPeaks();
      size_t n_peaks = ws->getNumberPeaks();

                                       // transform the HKLs and record the new HKL
                                       // and q-vectors for peaks ORIGINALLY indexed
      DblMatrix hkl_tran = info.GetHKL_Tran();
      int num_indexed = 0;
      std::vector<V3D> miller_indices;
      std::vector<V3D> q_vectors;
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

