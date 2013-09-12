/*WIKI* 


Given a PeaksWorkspace with a UB matrix stored with the sample, this algoritm will accept a 3x3 transformation matrix M, change UB to UB*M-inverse and map each (HKL) vector to M*(HKL). For example, the transformation with elements 0,1,0,1,0,0,0,0,-1 will interchange the H and K values and negate L.  This algorithm should allow the usr to perform any required transformation of the Miller indicies, provided that transformation has a positive determinant.  If a transformation with a negative or zero determinant is entered, the algorithm with throw an exception.  The 9 elements of the transformation must be specified as a comma separated list of numbers.


*WIKI*/
#include "MantidCrystal/TransformHKL.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/Matrix.h"
#include <cstdio>
#include "MantidCrystal/SelectCellWithForm.h"

namespace Mantid
{
namespace Crystal
{
  Kernel::Logger& TransformHKL::g_log = Kernel::Logger::get("TransformHKL");

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(TransformHKL)

  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  using namespace Mantid::DataObjects;
  using namespace Mantid::Geometry;

  //--------------------------------------------------------------------------
  /** Constructor
   */
  TransformHKL::TransformHKL()
  {
  }
    
  //--------------------------------------------------------------------------
  /** Destructor
   */
  TransformHKL::~TransformHKL()
  {
  }

  const std::string TransformHKL::name() const
  {
    return "TransformHKL";
  }

  int TransformHKL::version() const
  {
    return 1;
  }

  const std::string TransformHKL::category() const
  {
    return "Crystal";
  }

  //--------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void TransformHKL::initDocs()
  {
    std::string summary("Adjust the UB stored with the sample to map the peak's  ");
    summary += "(HKL) vectors to M*(HKL)";
    this->setWikiSummary( summary );

    std::string message("Specify a 3x3 matrix to apply to (HKL) vectors.");
    message += " as a list of 9 comma separated numbers.";
    message += " Both the UB and HKL values will be updated";
    this->setOptionalMessage( message );
  }

  //--------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void TransformHKL::init()
  {
    this->declareProperty(
          new WorkspaceProperty<PeaksWorkspace>("PeaksWorkspace","",Direction::InOut),
          "Input Peaks Workspace");

    boost::shared_ptr<BoundedValidator<double> > mustBePositive(new BoundedValidator<double>());
    mustBePositive->setLower(0.0);

    this->declareProperty(
           new PropertyWithValue<double>("Tolerance",0.15, mustBePositive,Direction::Input),
           "Indexing Tolerance (0.15)");

    std::vector<double>  identity_matrix(9,0.0);
    identity_matrix[0] = 1;
    identity_matrix[4] = 1;
    identity_matrix[8] = 1;
    auto threeBythree = boost::make_shared<ArrayLengthValidator<double> >(9);
    this->declareProperty(
          new ArrayProperty<double>("HKL_Transform",identity_matrix,threeBythree),
          "Specify 3x3 HKL transform matrix as a comma separated list of 9 numbers");

    this->declareProperty(
          new PropertyWithValue<int>( "NumIndexed", 0, Direction::Output),
          "Gets set with the number of indexed peaks.");

    this->declareProperty(
          new PropertyWithValue<double>( "AverageError", 0.0, Direction::Output),
          "Gets set with the average HKL indexing error.");
  }

  //--------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void TransformHKL::exec()
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

    std::vector<double> tran_vec = getProperty("HKL_Transform");
    DblMatrix hkl_tran( tran_vec );

    std::ostringstream str_stream;
    str_stream << hkl_tran;
    std::string hkl_tran_string = str_stream.str();
    g_log.notice() << "Applying Tranformation " << hkl_tran_string << std::endl;

    if ( hkl_tran.numRows() != 3 || hkl_tran.numCols() != 3 )
    {
       throw std::runtime_error(
             "ERROR: The specified transform must be a 3 X 3 matrix.\n" +
              hkl_tran_string );
    }

    DblMatrix hkl_tran_inverse(hkl_tran);
    double det = hkl_tran_inverse.Invert();

    if ( fabs(det) < 1.0e-5 )
    {
      throw std::runtime_error(
             "ERROR: The specified matrix is invalid (essentially singular.)" +
              hkl_tran_string );
    }
    if ( det < 0 )
    {
      std::ostringstream str_stream;
      str_stream << hkl_tran;
      std::string hkl_tran_string = str_stream.str();
      throw std::runtime_error(
             "ERROR: The determinant of the matrix is negative.\n" +
              hkl_tran_string ); 
    }
    double tolerance = this->getProperty("Tolerance");

                                        // Transform looks OK so update UB and
                                        // transform the hkls
    UB = UB * hkl_tran_inverse;
    o_lattice.setUB( UB );
    std::vector<double> sigabc(6);
    SelectCellWithForm::DetermineErrors(sigabc,UB,ws, tolerance);
    o_lattice.setError( sigabc[0],sigabc[1],sigabc[2],sigabc[3],sigabc[4],sigabc[5]);
    ws->mutableSample().setOrientedLattice( new OrientedLattice(o_lattice) );

    std::vector<Peak> &peaks = ws->getPeaks();
    size_t n_peaks = ws->getNumberPeaks();



                                      // transform all the HKLs and record the new HKL
                                      // and q-vectors for peaks ORIGINALLY indexed
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

    double average_error = IndexingUtils::IndexingError( UB, miller_indices, q_vectors );

    // Tell the user what happened.
    g_log.notice() << o_lattice << "\n";
    g_log.notice() << "Transformed Miller indices on previously valid indexed Peaks.\n";
    g_log.notice() << "Set hkl to 0,0,0 on peaks previously indexed out of tolerance.\n";
    g_log.notice() << "Now, " << num_indexed << " are indexed with average error " << average_error << "\n";

    // Save output properties
    this->setProperty("NumIndexed", num_indexed);
    this->setProperty("AverageError", average_error);
  }


} // namespace Mantid
} // namespace Crystal

