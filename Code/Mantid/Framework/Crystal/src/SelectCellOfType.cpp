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
#include "MantidCrystal/SelectCellWithForm.h"

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


    Kernel::Matrix<double>T(UB);
    T.Invert();
    T = newUB * T;
    g_log.notice() << "Transformation Matrix =  " << T.str() << std::endl;


    if ( apply )
    {
      std::vector<double> sigabc(6);
      SelectCellWithForm::DetermineErrors(sigabc,newUB,ws, tolerance);
       //----------------------------------------------
      o_lattice.setUB( newUB );

      o_lattice.setError( sigabc[0],sigabc[1],sigabc[2],sigabc[3],sigabc[4],sigabc[5]);

      ws->mutableSample().setOrientedLattice( new OrientedLattice(o_lattice) ); 

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

