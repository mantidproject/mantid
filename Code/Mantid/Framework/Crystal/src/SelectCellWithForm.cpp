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
#include "MantidKernel/System.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/ScalarUtils.h"
#include "MantidGeometry/Crystal/ReducedCell.h"
#include "MantidGeometry/Crystal/ConventionalCell.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
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

    BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
    mustBePositive->setLower(1);

    this->declareProperty(
          new PropertyWithValue<int>("FormNumber",0,mustBePositive->clone(),Direction::Input),
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

    char buffer[200];

    DblMatrix newUB = info.GetNewUB();
    std::vector<double> lat_par;
    IndexingUtils::GetLatticeParameters( newUB, lat_par );

    sprintf( buffer, std::string("Form #%2d").c_str(), info.GetFormNum());
    std::string message( buffer );

    sprintf( buffer, std::string(" Error: %7.4f").c_str(), info.GetError());
    message += std::string( buffer );

    sprintf(buffer,std::string(" %12s").c_str(), info.GetCellType().c_str());
    message += std::string( buffer );

    sprintf(buffer,std::string(" %2s ").c_str(),info.GetCentering().c_str());
    message += std::string( buffer );

    sprintf( buffer,
             std::string("Lattice Params: %8.4f %8.4f %8.4f  %8.3f %8.3f %8.3f  %9.2f").c_str(),
             lat_par[0], lat_par[1], lat_par[2], lat_par[3], lat_par[4], lat_par[5], lat_par[6]);
    message += std::string( buffer );

    g_log.notice( std::string(message) );

    if ( apply )
    {
      o_lattice.setUB( newUB );
      ws->mutableSample().setOrientedLattice( new OrientedLattice(o_lattice) ); 

      std::vector<Peak> &peaks = ws->getPeaks();
      size_t n_peaks = ws->getNumberPeaks();

      std::vector<V3D> miller_indices;
      std::vector<V3D> q_vectors;

      q_vectors.reserve( n_peaks );
      for ( size_t i = 0; i < n_peaks; i++ )
      {
        q_vectors.push_back( peaks[i].getQSampleFrame() );
      }
 
      double average_error;

      int num_indexed = IndexingUtils::CalculateMillerIndices( newUB, 
                                                               q_vectors,
                                                               tolerance,
                                                               miller_indices,
                                                               average_error );
      g_log.notice() << "Indexed " << num_indexed 
                     << " Peaks out of " << n_peaks 
                     << " with tolerance of " << tolerance << std::endl;

      g_log.notice() << "Average error in h,k,l for indexed peaks =  "
                     << average_error << std::endl;

      for ( size_t i = 0; i < n_peaks; i++ )
        peaks[i].setHKL( miller_indices[i] );

      this->setProperty("NumIndexed", num_indexed);
      this->setProperty("AverageError", average_error);
    }
  }


} // namespace Mantid
} // namespace Crystal

