#ifndef DIFFSPHERETEST_H_
#define DIFFSPHERETEST_H_

#include <numeric>
#include <cxxtest/TestSuite.h>

// Include local copy of Valgrind header to avoid creating a dependency
#include "valgrind.h"

#include "MantidCurveFitting/Functions/DiffSphere.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;

class DiffSphereTest : public CxxTest::TestSuite {
public:
  bool skipTests() override {
    // Skip this test suite if running under valgrind as the Bessel function
    // calls in DiffSphere sometimes return NaN in this situation.
    // It's something to do with boost using 80 bit precision where valgrind
    // drops this to 64. See
    // https://www.mail-archive.com/valgrind-users@lists.sourceforge.net/msg00974.html
    return (RUNNING_ON_VALGRIND);
  }

  /* The weighted sum of the A_{n,l} coefficients is one
   * \sum_{n=0,l=0}^{n=\infty,l=\infty} (2*l+1) * A_{n,l}(Q*Radius) = 1, for all
   * values of parameter Q and Radius
   * We don't have infinity terms, but 99 (including A_{0,0}) thus the sum will
   * be close to one. The
   * sum is closer to 1 as the product Q*Radius decreases.
   */
  void testNormalization() {
    const double I(1.0);
    const double Q(1.0);
    const double D(1.0);

    // We vary parameter R while keeping the other constant, same as varying
    // Q*Radius
    double R(0.1);
    const double dR(0.1);

    // Suggested value by Volino for the approximation
    // of the 99 coefficients to break down
    const double QR_max(20);

    // initialize the elastic part
    boost::shared_ptr<ElasticDiffSphere> elastic_part =
        boost::make_shared<ElasticDiffSphere>();
    elastic_part->setParameter("Height", I);
    elastic_part->setParameter("Radius", R);
    elastic_part->setAttributeValue("Q", Q);
    elastic_part->init();

    // initialize the inelastic part
    boost::shared_ptr<InelasticDiffSphere> inelastic_part(
        new InelasticDiffSphere());
    inelastic_part->setParameter("Intensity", I);
    inelastic_part->setParameter("Radius", R);
    inelastic_part->setParameter("Diffusion", D);
    inelastic_part->setAttributeValue("Q", Q);
    inelastic_part->init();

    // calculate the normalization over different values of Q*R
    while (Q * R < QR_max) {
      elastic_part->setParameter("Radius", R);
      double elastic_intensity =
          elastic_part->HeightPrefactor(); // A_{0,0} coefficient
      inelastic_part->setParameter("Radius", R);
      std::vector<double> YJ = inelastic_part->LorentzianCoefficients(
          Q * R); // (2*l+1) * A_{n,l} coefficients
      double inelastic_intensity = std::accumulate(YJ.begin(), YJ.end(), 0.0);
      TS_ASSERT_DELTA(elastic_intensity + inelastic_intensity, 1.0,
                      0.02); // Allow for a 2% deviation
      R += dR;
    }
  }

}; // class DiffSphereTest

#endif /*DIFFSPHERETEST_H_*/
