/*WIKI* 

Given a set of peaks (Q in the goniometer frame, HKL values), and given lattice parameters <math>(a,b,c,\alpha,\beta,\gamma)</math>, it will try to find the U matrix, using least squares approach and quaternions [http://www.cs.iastate.edu/~cs577/handouts/quaternion.pdf].
Units of length are in in <math>\rm \AA</math>, angles are in degrees.

The algorithm calculates first the B matrix according to Busing and Levi.

Given a set of peaks in the reference frame of the inner axis of the goniometer, <math>\rm Q_{gon}</math>, indexed by <math>(h_i, k_i, l_i)</math>, we want to find the U matrix that maps peaks in the reciprocal space of the sample to the peaks in the goniometer frame

<math> \rm U \rm B \left(

                               \begin{array}{c}

                                 h_i \\

                                 k_i \\

                                 l_i \\

                               \end{array}

                             \right) = \rm Q_{gon,i} </math>          (1)

For simplicity, we define 

<math>\rm Q_{hkl,i} = \rm B \left(

                               \begin{array}{c}

                                 h_i \\

                                 k_i \\

                                 l_i \\

                               \end{array}

                             \right)</math>          (2)

In the real world, such a matrix is not always possible to find. Therefore we just try minimize the difference between the two sets of p

<math>\sum_i |\rm U \rm Q_{hkl,i} - \rm Q_{gon,i}|^2 = \sum_i \left(|\rm U \rm Q_{hkl,i}|^2 + |\rm Q_{gon,i}|^2 -2 \rm U \rm Q_{hkl,i} \cdot \rm Q_{gon,i}\right)</math>           (3)

In equation (3) <math>\left|\rm U \rm Q_{hkl,i}\right|^2 = |\rm Q_{hkl,i}|^2</math>, so the first two terms on the left side are U independent. Therefore we want to maximize 

<math>\sum_i \left(\rm U \rm Q_{hkl,i} \cdot \rm Q_{gon,i}\right) </math>     (4)

We are going to write the scalar product of the vectors in terms of quaternions[http://en.wikipedia.org/wiki/Quaternion]. We define 
<math>q_{hkl,i} = \left(0, Q_{hkl,i}\right)</math>, <math>  q_{gon,i} = \left(0, Q_{gon,i}\right)</math> and the rotation U is described by quaternion <math>u = \left(w,x,y,z\right)</math>

Then equation (4) will be written as

<math>\sum_i \left(\rm U \rm Q_{hkl,i} \cdot \rm Q_{gon,i}\right) = 0.5 \cdot \left(u q_{hkl,i} u^*\right) q_{gon,i}\ + 0.5 \cdot q_{gon,i} \left(u q_{hkl,i} u^*\right)</math>     (5)

We define matrices <math> H_i= \left(\begin{array}{cccc}
0 & -q_{hkl,i,x} & -q_{hkl,i,y} & -q_{hkl,i,z} \\
q_{hkl,i,x} & 0 & q_{hkl,i,z} & -q_{hkl,i,y} \\
q_{hkl,i,y} & -q_{hkl,i,z} & 0 & q_{hkl,i,x} \\
q_{hkl,i,z} & q_{hkl,i,y} & -q_{hkl,i,x} & 0 
\end{array} \right)</math>     (6)

and 
<math> S_i= \left(\begin{array}{cccc}
0 & -q_{gonl,i,x} & -q_{gon,i,y} & -q_{gon,i,z} \\
q_{gon,i,x} & 0 & -q_{gon,i,z} & q_{gon,i,y} \\
q_{gon,i,y} & q_{gon,i,z} & 0 & -q_{gon,i,x} \\
q_{gon,i,z} & -q_{gon,i,y} & q_{gon,i,x} & 0 
\end{array} \right)</math>     (7)

Then, we can rewrite equation (5) using matrices[http://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation#Pairs_of_unit_quaternions_as_rotations_in_4D_space],[http://www.cs.iastate.edu/~cs577/handouts/quaternion.pdf]:

<math>\sum_i \left(\rm U \rm Q_{hkl,i} \cdot \rm Q_{gon,i}\right) = \left(\begin{array}{cccc}
w & x & y & z\end{array} \right)  \sum_i H_i S_i \left(
                               \begin{array}{c}
                                 w \\
                                 x \\
                                 y \\
z
                               \end{array}

                             \right)</math>          (8)

The problem  of finding <math>\left(w,x,y,z\right)</math> that maximizes the sum can now be rewritten in terms of eigenvectors of <math> HS= \sum_i \left(H_i S_i\right) </math> .
Let <math>\epsilon_j</math> and <math>\nu_j</math> be the eigenvalues and corresponding eigenvectors of <math> HS</math>, with <math>\epsilon_0 > \epsilon_1 > \epsilon_2 > \epsilon_3 </math>. We can write any vector <math>(w,x,y,z)</math> as a linear combination of the eigenvectors of <math> HS</math>:

<math>\left(w,x,y,z\right) = \delta_0 \nu_0 +\delta_1 \nu_1 +\delta_2 \nu_2 +\delta_3 \nu_3</math>       (9)

<math>\left(\begin{array}{cccc}
w & x & y & z\end{array} \right)  HS \left(
                               \begin{array}{c}
                                 w \\
                                 x \\
                                 y \\
z
                               \end{array}

                             \right) = \delta_0^2 \nu_0 HS \nu_0 + \delta_1^2 \nu_1 HS \nu_1 +\delta_2^2 \nu_2 HS \nu_2 +\delta_3 \nu_3 HS \nu_3 </math>       (10)

<math> = \delta_0^2 \epsilon_0 + \delta_1^2 \epsilon_1 +\delta_2^2 \epsilon_2 +\delta_3 ^2 \epsilon_3</math>       (11)

<math>u</math> is a unit quaternion,  <math> \delta_0^2  + \delta_1^2 +\delta_2^2 +\delta_3 ^2=1</math>       (12)

Then the sum in equation (11) is maximized for <math>\epsilon_0 =1, \epsilon_1 =0,  \epsilon_2 =0 \epsilon_3 =0</math> (13)

Therefore U is the rotation represented by the quaternion <math> u</math>, which is the eigenvector corresponding to the largest eigenvalue of  <math>HS</math>.

*WIKI*/
#include "MantidCrystal/CalculateUMatrix.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid
{
namespace Crystal
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(CalculateUMatrix)

  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  using namespace Mantid::DataObjects;
  using Mantid::Geometry::OrientedLattice;


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
    this->setWikiSummary("Calculate the U matrix from a [[PeaksWorkspace]], given lattice parameters.");
    this->setOptionalMessage("Calculate the U matrix from a peaks workspace, given lattice parameters.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void CalculateUMatrix::init()
  {
    this->declareProperty(new WorkspaceProperty<PeaksWorkspace>("PeaksWorkspace","",Direction::InOut), "An input workspace.");
    boost::shared_ptr<BoundedValidator<double> > mustBePositive = boost::make_shared<BoundedValidator<double> >();
    mustBePositive->setLower(0.0);
    boost::shared_ptr<BoundedValidator<double> > reasonable_angle = boost::make_shared<BoundedValidator<double> >();
    reasonable_angle->setLower(5.0);
    reasonable_angle->setUpper(175.0);
    // put in negative values, so user is forced to input all parameters. no shortcuts :)
    this->declareProperty("a",-1.0,mustBePositive,"Lattice parameter a");
    this->declareProperty("b",-1.0,mustBePositive,"Lattice parameter b");
    this->declareProperty("c",-1.0,mustBePositive,"Lattice parameter c");
    this->declareProperty("alpha",-1.0,reasonable_angle,"Lattice parameter alpha");
    this->declareProperty("beta",-1.0,reasonable_angle,"Lattice parameter beta");
    this->declareProperty("gamma",-1.0,reasonable_angle,"Lattice parameter gamma");
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

    PeaksWorkspace_sptr ws;
    ws = AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>(this->getProperty("PeaksWorkspace") );
    if (!ws) throw std::runtime_error("Problems reading the peaks workspace");

    size_t nIndexedpeaks=0;
    bool found2nc=false;
    V3D old(0,0,0);
    Matrix<double> Hi(4,4),Si(4,4),HS(4,4),zero(4,4);
    for (int i=0;i<ws->getNumberPeaks();i++)
    {
      Peak p=ws->getPeaks()[i];
      double H=p.getH();
      double K=p.getK();
      double L=p.getL();
      if(H*H+K*K+L*L>0)
      {
        nIndexedpeaks++;
        if (!found2nc)
        {
          if (nIndexedpeaks==1)
          {
            old=V3D(H,K,L);
          }
          else
          {
            if (!old.coLinear(V3D(0,0,0),V3D(H,K,L))) found2nc=true;
          }
        }
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
    //check if enough peaks are indexed or if HS is 0
    if ((nIndexedpeaks<2) || (found2nc==false)) throw std::invalid_argument("Less then two non-colinear peaks indexed");
    if (HS==zero) throw std::invalid_argument("Something really bad happened");

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

