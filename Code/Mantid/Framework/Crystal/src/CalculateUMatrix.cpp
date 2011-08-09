#include "MantidCrystal/CalculateUMatrix.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

namespace Mantid
{
namespace Crystal
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(CalculateUMatrix)

  using namespace Mantid::Geometry;
  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  using namespace Mantid::DataObjects;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  CalculateUMatrix::CalculateUMatrix()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  CalculateUMatrix::~CalculateUMatrix()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void CalculateUMatrix::initDocs()
  {
    this->setWikiSummary("Calculate the U matrix from a peaks workspace, given lattice parameters.");
    this->setOptionalMessage("Calculate the U matrix from a peaks workspace, given lattice parameters.");
    this->setWikiDescription("Given a set of peaks (Q in sample frame, HKL values), and given lattice parameters (<math>a,b,c,/alpha,/beta,gamma</math>), it will try to find the U matrix, using least squares approach and quaternions");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void CalculateUMatrix::init()
  {
    this->declareProperty(new WorkspaceProperty<PeaksWorkspace>("PeaksWorkspace","",Direction::InOut), "An input workspace.");
    BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
    mustBePositive->setLower(0.0);
    BoundedValidator<double> *reasonable_angle = new BoundedValidator<double>();
    reasonable_angle->setLower(5.0);
    reasonable_angle->setUpper(175.0);
    // put in negative values, so user is forced to input all parameters. no shortcuts :)
    this->declareProperty(new PropertyWithValue<double>("a",-1.0,mustBePositive->clone(),Direction::Input),"Lattice parameter a");
    this->declareProperty(new PropertyWithValue<double>("b",-1.0,mustBePositive->clone(),Direction::Input),"Lattice parameter b");
    this->declareProperty(new PropertyWithValue<double>("c",-1.0,mustBePositive->clone(),Direction::Input),"Lattice parameter c");
    this->declareProperty(new PropertyWithValue<double>("alpha",-1.0,reasonable_angle->clone(),Direction::Input),"Lattice parameter alpha");
    this->declareProperty(new PropertyWithValue<double>("beta",-1.0,reasonable_angle->clone(),Direction::Input),"Lattice parameter beta");
    this->declareProperty(new PropertyWithValue<double>("gamma",-1.0,reasonable_angle->clone(),Direction::Input),"Lattice parameter gamma");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void CalculateUMatrix::exec()
  {
    double a=this->getProperty("a");
    double b=this->getProperty("b");
    double c=this->getProperty("c");
    double alpha=this->getProperty("alpha");
    double beta=this->getProperty("beta");
    double gamma=this->getProperty("gamma");
    OrientedLattice o(a,b,c,alpha,beta,gamma);
    DblMatrix B=o.getB();

    double H,K,L;

    PeaksWorkspace_sptr ws;
    ws = boost::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve(this->getProperty("PeaksWorkspace")) );
    if (!ws) throw std::runtime_error("Problems reading the peaks workspace");

    DblMatrix Pi(4,4),Qi(4,4),PQ(4,4),zero(4,4);

    for (int i=0;i<ws->getNumberPeaks();i++)
    {
      Peak p=ws->getPeaks()[i];
      H=p.getH();
      K=p.getK();
      L=p.getL();
      if(H*H+K*K+L*L>0)
      {
        V3D QSampleFrame=p.getQSampleFrame();
        Pi[0][0]=0.;Pi[0][1]=-QSampleFrame.X();Pi[0][2]=-QSampleFrame.Y();Pi[0][3]=-QSampleFrame.Z();
        Pi[1][0]=QSampleFrame.X();Pi[1][1]=0.;Pi[1][2]=QSampleFrame.Z();Pi[1][3]=-QSampleFrame.Y();
        Pi[2][0]=QSampleFrame.Y();Pi[2][1]=-QSampleFrame.Z();Pi[2][2]=0.;Pi[2][3]=QSampleFrame.X();
        Pi[3][0]=QSampleFrame.Z();Pi[3][1]=QSampleFrame.Y();Pi[3][2]=-QSampleFrame.X();Pi[3][3]=0.;

        V3D Qhkl=B*V3D(H,K,L);
        Qi[0][0]=0.;Qi[0][1]=-Qhkl.X();Qi[0][2]=-Qhkl.Y();Qi[0][3]=-Qhkl.Z();
        Qi[1][0]=Qhkl.X();Qi[1][1]=0.;Qi[1][2]=-Qhkl.Z();Qi[1][3]=Qhkl.Y();
        Qi[2][0]=Qhkl.Y();Qi[2][1]=Qhkl.Z();Qi[2][2]=0.;Qi[2][3]=-Qhkl.X();
        Qi[3][0]=Qhkl.Z();Qi[3][1]=-Qhkl.Y();Qi[3][2]=Qhkl.X();Qi[3][3]=0.;

        PQ+=(Pi*Qi);
      }
    }
    //check if PQ is 0
    if (PQ==zero) throw std::invalid_argument("The peaks workspace is not indexed or something really bad happened");

    Matrix<double> Eval;
    Matrix<double> Diag;
    PQ.Diagonalise(Eval,Diag);
    Eval.sortEigen(Diag);
    Mantid::Kernel::Quat qR(Eval[0][0],Eval[1][0],Eval[2][0],Eval[3][0]);
    DblMatrix U(qR.getRotation());
    o.setU(U.Transpose());

    ws->mutableSample().setOrientedLattice(new OrientedLattice(o));

  }



} // namespace Mantid
} // namespace Crystal

