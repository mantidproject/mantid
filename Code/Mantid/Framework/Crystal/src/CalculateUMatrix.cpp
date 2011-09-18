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
    Matrix<double> B=o.getB();

    double H,K,L;

    PeaksWorkspace_sptr ws;
    ws = boost::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve(this->getProperty("PeaksWorkspace")) );
    if (!ws) throw std::runtime_error("Problems reading the peaks workspace");

    Matrix<double> Hi(4,4),Si(4,4),HS(4,4),zero(4,4);
    for (int i=0;i<ws->getNumberPeaks();i++)
    {
      Peak p=ws->getPeaks()[i];
      H=p.getH();
      K=p.getK();
      L=p.getL();
      if(H*H+K*K+L*L>0)
      {
        V3D Qhkl=B*V3D(H,K,L);
        Hi[0][0]=0.;        Hi[0][1]=-Qhkl.X(); Hi[0][2]=-Qhkl.Y(); Hi[0][3]=-Qhkl.Z();
        Hi[1][0]=Qhkl.X();  Hi[1][1]=0.;        Hi[1][2]=Qhkl.Z();  Hi[1][3]=-Qhkl.Y();
        Hi[2][0]=Qhkl.Y();  Hi[2][1]=-Qhkl.Z(); Hi[2][2]=0.;        Hi[2][3]=Qhkl.X();
        Hi[3][0]=Qhkl.Z();  Hi[3][1]=Qhkl.Y();  Hi[3][2]=-Qhkl.X(); Hi[3][3]=0.;

        V3D Qgon=p.getQSampleFrame();
        Si[0][0]=0.;        Si[0][1]=-Qgon.X(); Si[0][2]=-Qgon.Y(); Si[0][3]=-Qgon.Z();
        Si[1][0]=Qgon.X();  Si[1][1]=0.;        Si[1][2]=-Qgon.Z(); Si[1][3]=Qgon.Y();
        Si[2][0]=Qgon.Y();  Si[2][1]=Qgon.Z();  Si[2][2]=0.;        Si[2][3]=-Qgon.X();
        Si[3][0]=Qgon.Z();  Si[3][1]=-Qgon.Y(); Si[3][2]=Qgon.X(); Si[3][3]=0.;

        HS+=(Hi*Si);
      }
    }
    //check if HS is 0
    if (HS==zero) throw std::invalid_argument("The peaks workspace is not indexed or something really bad happened");

    Matrix<double> Eval;
    Matrix<double> Diag;
    HS.Diagonalise(Eval,Diag);
    Eval.sortEigen(Diag);
    Mantid::Kernel::Quat qR(Eval[0][0],Eval[1][0],Eval[2][0],Eval[3][0]);//the first column corresponds to the highest eigenvalue
    DblMatrix U(qR.getRotation());
    o.setU(U);

    ws->mutableSample().setOrientedLattice(new OrientedLattice(o));

  }



} // namespace Mantid
} // namespace Crystal

