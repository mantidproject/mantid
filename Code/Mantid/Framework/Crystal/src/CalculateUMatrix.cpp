#include "MantidCrystal/CalculateUMatrix.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalculateUMatrix)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::Geometry::OrientedLattice;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
CalculateUMatrix::CalculateUMatrix() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
CalculateUMatrix::~CalculateUMatrix() {}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CalculateUMatrix::init() {
  this->declareProperty(new WorkspaceProperty<PeaksWorkspace>(
                            "PeaksWorkspace", "", Direction::InOut),
                        "An input workspace.");
  boost::shared_ptr<BoundedValidator<double>> mustBePositive =
      boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  boost::shared_ptr<BoundedValidator<double>> reasonable_angle =
      boost::make_shared<BoundedValidator<double>>();
  reasonable_angle->setLower(5.0);
  reasonable_angle->setUpper(175.0);
  // put in negative values, so user is forced to input all parameters. no
  // shortcuts :)
  this->declareProperty("a", -1.0, mustBePositive, "Lattice parameter a");
  this->declareProperty("b", -1.0, mustBePositive, "Lattice parameter b");
  this->declareProperty("c", -1.0, mustBePositive, "Lattice parameter c");
  this->declareProperty("alpha", -1.0, reasonable_angle,
                        "Lattice parameter alpha");
  this->declareProperty("beta", -1.0, reasonable_angle,
                        "Lattice parameter beta");
  this->declareProperty("gamma", -1.0, reasonable_angle,
                        "Lattice parameter gamma");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CalculateUMatrix::exec() {
  double a = this->getProperty("a");
  double b = this->getProperty("b");
  double c = this->getProperty("c");
  double alpha = this->getProperty("alpha");
  double beta = this->getProperty("beta");
  double gamma = this->getProperty("gamma");
  OrientedLattice o(a, b, c, alpha, beta, gamma);
  Matrix<double> B = o.getB();

  PeaksWorkspace_sptr ws = this->getProperty("PeaksWorkspace");
  if (!ws)
    throw std::runtime_error("Problems reading the peaks workspace");

  size_t nIndexedpeaks = 0;
  bool found2nc = false;
  V3D old(0, 0, 0);
  Matrix<double> Hi(4, 4), Si(4, 4), HS(4, 4), zero(4, 4);
  for (int i = 0; i < ws->getNumberPeaks(); i++) {
    Peak p = ws->getPeaks()[i];
    double H = p.getH();
    double K = p.getK();
    double L = p.getL();
    if (H * H + K * K + L * L > 0) {
      nIndexedpeaks++;
      if (!found2nc) {
        if (nIndexedpeaks == 1) {
          old = V3D(H, K, L);
        } else {
          if (!old.coLinear(V3D(0, 0, 0), V3D(H, K, L)))
            found2nc = true;
        }
      }
      V3D Qhkl = B * V3D(H, K, L);
      Hi[0][0] = 0.;
      Hi[0][1] = -Qhkl.X();
      Hi[0][2] = -Qhkl.Y();
      Hi[0][3] = -Qhkl.Z();
      Hi[1][0] = Qhkl.X();
      Hi[1][1] = 0.;
      Hi[1][2] = Qhkl.Z();
      Hi[1][3] = -Qhkl.Y();
      Hi[2][0] = Qhkl.Y();
      Hi[2][1] = -Qhkl.Z();
      Hi[2][2] = 0.;
      Hi[2][3] = Qhkl.X();
      Hi[3][0] = Qhkl.Z();
      Hi[3][1] = Qhkl.Y();
      Hi[3][2] = -Qhkl.X();
      Hi[3][3] = 0.;

      V3D Qgon = p.getQSampleFrame();
      Si[0][0] = 0.;
      Si[0][1] = -Qgon.X();
      Si[0][2] = -Qgon.Y();
      Si[0][3] = -Qgon.Z();
      Si[1][0] = Qgon.X();
      Si[1][1] = 0.;
      Si[1][2] = -Qgon.Z();
      Si[1][3] = Qgon.Y();
      Si[2][0] = Qgon.Y();
      Si[2][1] = Qgon.Z();
      Si[2][2] = 0.;
      Si[2][3] = -Qgon.X();
      Si[3][0] = Qgon.Z();
      Si[3][1] = -Qgon.Y();
      Si[3][2] = Qgon.X();
      Si[3][3] = 0.;

      HS += (Hi * Si);
    }
  }
  // check if enough peaks are indexed or if HS is 0
  if ((nIndexedpeaks < 2) || (found2nc == false))
    throw std::invalid_argument("Less then two non-colinear peaks indexed");
  if (HS == zero)
    throw std::invalid_argument("Something really bad happened");

  Matrix<double> Eval;
  Matrix<double> Diag;
  HS.Diagonalise(Eval, Diag);
  Eval.sortEigen(Diag);
  Mantid::Kernel::Quat qR(
      Eval[0][0], Eval[1][0], Eval[2][0],
      Eval[3][0]); // the first column corresponds to the highest eigenvalue
  DblMatrix U(qR.getRotation());
  o.setU(U);

  ws->mutableSample().setOrientedLattice(&o);
}

} // namespace Mantid
} // namespace Crystal
