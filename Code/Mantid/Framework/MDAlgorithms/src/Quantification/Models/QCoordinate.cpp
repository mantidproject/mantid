// Includes
#include "MantidMDAlgorithms/Quantification/Models/QCoordinate.h"
#include <cmath>

namespace Mantid {
namespace MDAlgorithms {
DECLARE_FOREGROUNDMODEL(QCoordinate)

namespace // anonymous
    {
/// N attrs
const unsigned int NATTS = 1;
/// Attribute names
const char *ATTR_NAMES[NATTS] = {"Coord"};
/// 2 \pi
const double TWO_PI = 2. * M_PI;
}

/**
 * Initialize the model
 */
void QCoordinate::init() {
  declareAttribute(ATTR_NAMES[0], API::IFunction::Attribute("H"));
}

/**
 * Called when an attribute is set from the Fit string
 * @param name :: The name of the attribute
 * @param attr :: The value of the attribute
 */
void QCoordinate::setAttribute(const std::string &name,
                               const API::IFunction::Attribute &attr) {
  if (name == ATTR_NAMES[0]) {
    std::string value = attr.asString();
    if (value == "QcX")
      m_coord = 0;
    else if (value == "QcY")
      m_coord = 1;
    else if (value == "QcZ")
      m_coord = 2;
    else if (value == "H")
      m_coord = 3;
    else if (value == "K")
      m_coord = 4;
    else if (value == "L")
      m_coord = 5;
    else if (value == "En")
      m_coord = 6;
    else if (value == "Unity")
      m_coord = 7;
    else
      throw std::invalid_argument(
          "Unknown coordinate name passed to QCoordinate model");
  } else
    ForegroundModel::setAttribute(name, attr); // pass it on the base
}

/**
 * Calculates the scattering intensity
 * @param exptSetup :: Details of the current experiment
 * @param point :: The axis values for the current point in Q-W space: Qx, Qy,
 * Qz, DeltaE. They
 * are in the cartesian crystal frame
 * @return The weight contributing from this point
 */
double
QCoordinate::scatteringIntensity(const API::ExperimentInfo &exptSetup,
                                 const std::vector<double> &point) const {
  // unity
  if (m_coord == 7)
    return 1.0;

  // energy
  const double eps(point[3]);
  if (m_coord == 6)
    return eps;

  // crystal frame
  const double qx(point[0]), qy(point[1]), qz(point[2]);
  if (m_coord == 0)
    return qx;
  else if (m_coord == 1)
    return qy;
  else if (m_coord == 2)
    return qz;

  // HKL coords
  // Transform the HKL only requires B matrix & goniometer (R) as ConvertToMD
  // should have already
  // handled addition of U matrix
  // qhkl = (1/2pi)(RUB)^-1(qxyz)
  const Geometry::OrientedLattice &lattice =
      exptSetup.sample().getOrientedLattice();
  const Kernel::DblMatrix &gr = exptSetup.run().getGoniometerMatrix();
  const Kernel::DblMatrix &bmat = lattice.getUB();

  // Avoid doing inversion with Matrix class as it forces memory allocations
  // M^-1 = (1/|M|)*M^T
  double rb00(0.0), rb01(0.0), rb02(0.0), rb10(0.0), rb11(0.0), rb12(0.0),
      rb20(0.0), rb21(0.0), rb22(0.0);
  for (unsigned int i = 0; i < 3; ++i) {
    rb00 += gr[0][i] * bmat[i][0];
    rb01 += gr[0][i] * bmat[i][1];
    rb02 += gr[0][i] * bmat[i][2];

    rb10 += gr[1][i] * bmat[i][0];
    rb11 += gr[1][i] * bmat[i][1];
    rb12 += gr[1][i] * bmat[i][2];

    rb20 += gr[2][i] * bmat[i][0];
    rb21 += gr[2][i] * bmat[i][1];
    rb22 += gr[2][i] * bmat[i][2];
  }
  // 2pi*determinant. The tobyFit definition of rl vector has extra 2pi factor
  // in it
  const double twoPiDet = TWO_PI * (rb00 * (rb11 * rb22 - rb12 * rb21) -
                                    rb01 * (rb10 * rb22 - rb12 * rb20) +
                                    rb02 * (rb10 * rb21 - rb11 * rb20));

  // qh
  if (m_coord == 3)
    return ((rb11 * rb22 - rb12 * rb21) * qx +
            (rb02 * rb21 - rb01 * rb22) * qy +
            (rb01 * rb12 - rb02 * rb11) * qz) /
           twoPiDet;
  // qk
  else if (m_coord == 4)
    return ((rb12 * rb20 - rb10 * rb22) * qx +
            (rb00 * rb22 - rb02 * rb20) * qy +
            (rb02 * rb10 - rb00 * rb12) * qz) /
           twoPiDet;
  // ql
  else if (m_coord == 5)
    return ((rb10 * rb21 - rb11 * rb20) * qx +
            (rb01 * rb20 - rb00 * rb21) * qy +
            (rb00 * rb11 - rb01 * rb10) * qz) /
           twoPiDet;
  else
    throw std::invalid_argument("Logical error. Invalid coord type " +
                                boost::lexical_cast<std::string>(m_coord));
}
}
}
