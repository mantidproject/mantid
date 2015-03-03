#ifndef MANTID_CURVEFITTING_PSEUDOVOIGTTEST_H_
#define MANTID_CURVEFITTING_PSEUDOVOIGTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/PseudoVoigt.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidCurveFitting/Jacobian.h"
#include <boost/make_shared.hpp>

using namespace Mantid::CurveFitting;
using namespace Mantid::API;

class PseudoVoigtTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PseudoVoigtTest *createSuite() { return new PseudoVoigtTest(); }
  static void destroySuite(PseudoVoigtTest *suite) { delete suite; }

  PseudoVoigtTest()
      : m_xValues(), m_yValues(), m_dfdh(), m_dfda(), m_dfdx0(), m_dfdf() {
    m_xValues.push_back(0.991491491491491);
    m_xValues.push_back(0.992492492492492);
    m_xValues.push_back(0.993493493493493);
    m_xValues.push_back(0.994494494494494);
    m_xValues.push_back(0.995495495495496);
    m_xValues.push_back(0.996496496496497);
    m_xValues.push_back(0.997497497497497);
    m_xValues.push_back(0.998498498498498);
    m_xValues.push_back(0.999499499499499);
    m_xValues.push_back(1.000500500500501);
    m_xValues.push_back(1.001501501501501);
    m_xValues.push_back(1.002502502502503);
    m_xValues.push_back(1.003503503503504);
    m_xValues.push_back(1.004504504504504);
    m_xValues.push_back(1.005505505505506);
    m_xValues.push_back(1.006506506506506);
    m_xValues.push_back(1.007507507507508);
    m_xValues.push_back(1.008508508508509);
    m_xValues.push_back(1.009509509509509);
    m_xValues.push_back(1.010510510510511);
    m_xValues.push_back(1.011511511511511);

    m_yValues.push_back(4.372997125267132);
    m_yValues.push_back(4.458629118465070);
    m_yValues.push_back(4.535563492585204);
    m_yValues.push_back(4.603064037523992);
    m_yValues.push_back(4.660455187114265);
    m_yValues.push_back(4.707139614264023);
    m_yValues.push_back(4.742615179498014);
    m_yValues.push_back(4.766490204566635);
    m_yValues.push_back(4.778496044066421);
    m_yValues.push_back(4.778496044066421);
    m_yValues.push_back(4.766490204566637);
    m_yValues.push_back(4.742615179498014);
    m_yValues.push_back(4.707139614264019);
    m_yValues.push_back(4.660455187114265);
    m_yValues.push_back(4.603064037523992);
    m_yValues.push_back(4.535563492585212);
    m_yValues.push_back(4.458629118465070);
    m_yValues.push_back(4.372997125267132);
    m_yValues.push_back(4.279447055100300);
    m_yValues.push_back(4.178785512380577);
    m_yValues.push_back(4.071831485496261);

    m_dfdh.push_back(0.914852955076807);
    m_dfdh.push_back(0.932767598005245);
    m_dfdh.push_back(0.948862655352554);
    m_dfdh.push_back(0.962984108268618);
    m_dfdh.push_back(0.974990624919302);
    m_dfdh.push_back(0.984757241477829);
    m_dfdh.push_back(0.992178907844773);
    m_dfdh.push_back(0.997173682963731);
    m_dfdh.push_back(0.999685364867452);
    m_dfdh.push_back(0.999685364867452);
    m_dfdh.push_back(0.997173682963731);
    m_dfdh.push_back(0.992178907844773);
    m_dfdh.push_back(0.984757241477829);
    m_dfdh.push_back(0.974990624919302);
    m_dfdh.push_back(0.962984108268618);
    m_dfdh.push_back(0.948862655352554);
    m_dfdh.push_back(0.932767598005245);
    m_dfdh.push_back(0.914852955076807);
    m_dfdh.push_back(0.895281810690438);
    m_dfdh.push_back(0.874222910539870);
    m_dfdh.push_back(0.851847591108002);

    m_dfda.push_back(0.127423417613684);
    m_dfda.push_back(0.105761666867053);
    m_dfda.push_back(0.083998491075912);
    m_dfda.push_back(0.063081569151440);
    m_dfda.push_back(0.043939766110092);
    m_dfda.push_back(0.027438762645369);
    m_dfda.push_back(0.014336810534878);
    m_dfda.push_back(0.005243855136706);
    m_dfda.push_back(0.000587294644077);
    m_dfda.push_back(0.000587294644077);
    m_dfda.push_back(0.005243855136706);
    m_dfda.push_back(0.014336810534878);
    m_dfda.push_back(0.027438762645369);
    m_dfda.push_back(0.043939766110092);
    m_dfda.push_back(0.063081569151440);
    m_dfda.push_back(0.083998491075912);
    m_dfda.push_back(0.105761666867053);
    m_dfda.push_back(0.127423417613684);
    m_dfda.push_back(0.148058862985728);
    m_dfda.push_back(0.166802486088368);
    m_dfda.push_back(0.182878080915878);

    m_dfdx0.push_back(-8.963400576569903e+01);
    m_dfdx0.push_back(-8.132865068366561e+01);
    m_dfdx0.push_back(-7.226335976168113e+01);
    m_dfdx0.push_back(-6.248995205947752e+01);
    m_dfdx0.push_back(-5.207782518137794e+01);
    m_dfdx0.push_back(-4.111379724585275e+01);
    m_dfdx0.push_back(-2.970095613292614e+01);
    m_dfdx0.push_back(-1.795646367180882e+01);
    m_dfdx0.push_back(-6.008372247750958e+00);
    m_dfdx0.push_back(6.008372247750958e+00);
    m_dfdx0.push_back(1.795646367180882e+01);
    m_dfdx0.push_back(2.970095613292614e+01);
    m_dfdx0.push_back(4.111379724585275e+01);
    m_dfdx0.push_back(5.207782518137794e+01);
    m_dfdx0.push_back(6.248995205947752e+01);
    m_dfdx0.push_back(7.226335976168113e+01);
    m_dfdx0.push_back(8.132865068366561e+01);
    m_dfdx0.push_back(8.963400576569903e+01);
    m_dfdx0.push_back(9.714448961626630e+01);
    m_dfdx0.push_back(1.038406984991238e+02);
    m_dfdx0.push_back(1.097169693748341e+02);

    m_dfdf.push_back(1.525303401418302e+01);
    m_dfdf.push_back(1.221150911166150e+01);
    m_dfdf.push_back(9.403640409427975e+00);
    m_dfdf.push_back(6.880775502044572e+00);
    m_dfdf.push_back(4.691695962286301e+00);
    m_dfdf.push_back(2.880846653863556e+00);
    m_dfdf.push_back(1.486534340987295e+00);
    m_dfdf.push_back(5.392331432975621e-01);
    m_dfdf.push_back(6.014386634385344e-02);
    m_dfdf.push_back(6.014386634385344e-02);
    m_dfdf.push_back(5.392331432975621e-01);
    m_dfdf.push_back(1.486534340987295e+00);
    m_dfdf.push_back(2.880846653863556e+00);
    m_dfdf.push_back(4.691695962286301e+00);
    m_dfdf.push_back(6.880775502044572e+00);
    m_dfdf.push_back(9.403640409427975e+00);
    m_dfdf.push_back(1.221150911166150e+01);
    m_dfdf.push_back(1.525303401418302e+01);
    m_dfdf.push_back(1.847592895604664e+01);
    m_dfdf.push_back(2.182837505987588e+01);
    m_dfdf.push_back(2.526016311933117e+01);
  }

  void testPseudoVoigtValues() {
    IFunction_sptr pv = getInitializedPV(1.0, 4.78, 0.05, 0.7);

    FunctionDomain1DVector domain(m_xValues);
    FunctionValues values(domain);

    pv->function(domain, values);

    for (size_t i = 0; i < values.size(); ++i) {
      TS_ASSERT_DELTA(values[i], m_yValues[i], 1e-13);
    }
  }

  void testPseudoVoigtDerivatives() {
    IFunction_sptr pv = getInitializedPV(1.0, 4.78, 0.05, 0.7);

    FunctionDomain1DVector domain(m_xValues);
    Mantid::CurveFitting::Jacobian jacobian(domain.size(), 4);

    pv->functionDeriv(domain, jacobian);

    for (size_t i = 0; i < domain.size(); ++i) {
      TS_ASSERT_DELTA(jacobian.get(i, 0), m_dfda[i], 1e-13);
      TS_ASSERT_DELTA(jacobian.get(i, 1), m_dfdh[i], 1e-13);
      TS_ASSERT_DELTA(jacobian.get(i, 2), m_dfdx0[i], 1e-11);
      TS_ASSERT_DELTA(jacobian.get(i, 3), m_dfdf[i], 1e-11);
    }
  }

private:
  IFunction_sptr getInitializedPV(double center, double height, double fwhm,
                                  double mixing) {
    IFunction_sptr pv = boost::make_shared<PseudoVoigt>();
    pv->initialize();
    pv->setParameter("PeakCentre", center);
    pv->setParameter("FWHM", fwhm);
    pv->setParameter("Height", height);
    pv->setParameter("Mixing", mixing);

    return pv;
  }

  std::vector<double> m_xValues;
  std::vector<double> m_yValues;
  std::vector<double> m_dfdh;
  std::vector<double> m_dfda;
  std::vector<double> m_dfdx0;
  std::vector<double> m_dfdf;
};

#endif /* MANTID_CURVEFITTING_PSEUDOVOIGTTEST_H_ */
