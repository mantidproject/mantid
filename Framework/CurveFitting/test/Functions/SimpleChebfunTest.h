#ifndef MANTID_CURVEFITTING_SIMPLECHEBFUNTEST_H_
#define MANTID_CURVEFITTING_SIMPLECHEBFUNTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/SimpleChebfun.h"
#include <cmath>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;

namespace {
double Sin(double x) { return sin(x); }
double Cos(double x) { return cos(x); }
double Exp(double x) { return exp(-x * x / 2); }
} // namespace

class SimpleChebfunTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SimpleChebfunTest *createSuite() { return new SimpleChebfunTest(); }
  static void destroySuite(SimpleChebfunTest *suite) { delete suite; }

  void test_constructor_low_accuracy() {
    SimpleChebfun cheb(5, Sin, -M_PI, M_PI);
    do_test_values(cheb, Sin, 0.02);
    TS_ASSERT(cheb.isGood());
  }

  void test_constructor_high_accuracy() {
    SimpleChebfun cheb(21, Sin, -M_PI, M_PI);
    do_test_values(cheb, Sin, 1e-15);
    TS_ASSERT(cheb.isGood());
  }

  void test_constructor_best_fit() {
    SimpleChebfun cheb(Sin, -M_PI, M_PI);
    do_test_values(cheb, Sin, 2e-15);
    TS_ASSERT_EQUALS(cheb.size(), 20);
    TS_ASSERT(cheb.isGood());
  }

  void test_constructor_bad_fit() {
    SimpleChebfun cheb(Sin, -1000, 1000);
    do_test_values(cheb, Sin, 2e-15);
    TS_ASSERT_EQUALS(cheb.size(), 10);
    TS_ASSERT(!cheb.isGood());
  }

  void test_constructor_smooth() {
    double xa[] = {-10,
                   -9.7979797979798,
                   -9.5959595959596,
                   -9.39393939393939,
                   -9.19191919191919,
                   -8.98989898989899,
                   -8.78787878787879,
                   -8.58585858585859,
                   -8.38383838383838,
                   -8.18181818181818,
                   -7.97979797979798,
                   -7.77777777777778,
                   -7.57575757575758,
                   -7.37373737373737,
                   -7.17171717171717,
                   -6.96969696969697,
                   -6.76767676767677,
                   -6.56565656565657,
                   -6.36363636363636,
                   -6.16161616161616,
                   -5.95959595959596,
                   -5.75757575757576,
                   -5.55555555555556,
                   -5.35353535353535,
                   -5.15151515151515,
                   -4.94949494949495,
                   -4.74747474747475,
                   -4.54545454545455,
                   -4.34343434343434,
                   -4.14141414141414,
                   -3.93939393939394,
                   -3.73737373737374,
                   -3.53535353535354,
                   -3.33333333333333,
                   -3.13131313131313,
                   -2.92929292929293,
                   -2.72727272727273,
                   -2.52525252525253,
                   -2.32323232323232,
                   -2.12121212121212,
                   -1.91919191919192,
                   -1.71717171717172,
                   -1.51515151515152,
                   -1.31313131313131,
                   -1.11111111111111,
                   -0.90909090909091,
                   -0.707070707070708,
                   -0.505050505050505,
                   -0.303030303030303,
                   -0.1010101010101,
                   0.1010101010101,
                   0.303030303030303,
                   0.505050505050505,
                   0.707070707070708,
                   0.909090909090908,
                   1.11111111111111,
                   1.31313131313131,
                   1.51515151515152,
                   1.71717171717172,
                   1.91919191919192,
                   2.12121212121212,
                   2.32323232323232,
                   2.52525252525252,
                   2.72727272727273,
                   2.92929292929293,
                   3.13131313131313,
                   3.33333333333333,
                   3.53535353535353,
                   3.73737373737374,
                   3.93939393939394,
                   4.14141414141414,
                   4.34343434343434,
                   4.54545454545454,
                   4.74747474747475,
                   4.94949494949495,
                   5.15151515151515,
                   5.35353535353535,
                   5.55555555555556,
                   5.75757575757576,
                   5.95959595959596,
                   6.16161616161616,
                   6.36363636363636,
                   6.56565656565656,
                   6.76767676767677,
                   6.96969696969697,
                   7.17171717171717,
                   7.37373737373737,
                   7.57575757575757,
                   7.77777777777778,
                   7.97979797979798,
                   8.18181818181818,
                   8.38383838383838,
                   8.58585858585858,
                   8.78787878787879,
                   8.98989898989899,
                   9.19191919191919,
                   9.39393939393939,
                   9.5959595959596,
                   9.7979797979798,
                   10};
    double ya[] = {
        0.00639183934751033,   -0.0188335740757259,  -0.00899934560411868,
        0.00283632596574657,   -0.00598441814465674, 0.00696204020099447,
        -0.00287351005669914,  -0.0225096725729169,  0.0111069805441983,
        -0.00577115078672668,  -0.0109182725418975,  0.000532199156305037,
        0.00696599114054958,   0.00365932816743702,  -0.0200435513032935,
        0.00900735793474218,   -0.0111338670746253,  0.023255697971321,
        -0.00832455823569091,  -0.0029048509120321,  -0.0153785693305323,
        -0.00180000537108807,  0.00596251427052631,  0.0172553307333174,
        0.00361568201271514,   0.00928919020849222,  -0.0211978053309214,
        -0.00222464964883951,  0.00360438675108187,  0.00530721642552667,
        0.00784709918377635,   0.00416006087543879,  0.00960094613670988,
        0.00996615094193702,   0.00137548954996262,  0.0207270701280542,
        0.0216393663905915,    0.0377090829328035,   0.0579050519986335,
        0.0801382017977186,    0.157688228936361,    0.22703382274324,
        0.309878514091159,     0.430306496879367,    0.544703938468763,
        0.666604084021673,     0.773206598668196,    0.871405974058432,
        0.975632619000061,     0.995752614565419,    1.00464752735264,
        0.941959943590075,     0.869798994190271,    0.781879765315447,
        0.652587049158077,     0.523235702848816,    0.41967192318005,
        0.325189965870962,     0.228996533585127,    0.161368813413821,
        0.107691324301568,     0.0699110846617227,   0.0638760790932532,
        0.0302559377573298,    0.00460798768888443,  0.014804492257474,
        -1.54042567271596e-05, 0.00465828504155101,  0.0127947548498089,
        -0.00200744725099752,  0.00340488681152168,  0.00438153206037417,
        0.012532700578783,     -0.00479858530403384, 0.00996622957226827,
        -0.00129829848432229,  -0.0236134931852313,  -0.0167198070830326,
        -0.0178570689154704,   0.0190929355772175,   0.0197262324322656,
        0.0053771716671835,    0.020458130293195,    0.00968251677260515,
        -0.00308402567609968,  -0.01009201343035,    -0.0106552592606136,
        -0.00480150616204374,  0.00257980679328633,  0.0106976614827359,
        0.00214320379664739,   -0.00465281998719138, 0.00166258796607687,
        0.00762058850311789,   -0.0175536414593233,  0.027652419889149,
        -0.00829997647938784,  -0.0158371138209429,  -0.0216384527223296,
        -0.007513269418583};
    size_t n = sizeof(xa) / sizeof(double);
    std::vector<double> x(xa, xa + n);
    std::vector<double> y(ya, ya + n);
    SimpleChebfun cheb(x, y);
    TS_ASSERT_EQUALS(cheb.size(), n);
    TS_ASSERT(cheb.isGood());
    do_test_values(cheb, Exp, 0.021, 0.021);
  }

  void test_derivative() {
    SimpleChebfun cheb_sin(Sin, -M_PI, M_PI);
    auto cheb_cos = cheb_sin.derivative();
    do_test_values(cheb_cos, Cos, 1e-13, 1e-13);
  }

  void test_roughRoots() {
    SimpleChebfun cheb(Sin, -2 * M_PI - 0.1, 2 * M_PI + 0.1);
    auto roots = cheb.roughRoots();
    TS_ASSERT_EQUALS(roots.size(), 5);
    if (roots.size() == 5) {
      TS_ASSERT_DELTA(roots[0], -2 * M_PI, 1e-4);
      TS_ASSERT_DELTA(roots[1], -M_PI, 1e-2);
      TS_ASSERT_DELTA(roots[2], 0, 1e-9);
      TS_ASSERT_DELTA(roots[3], M_PI, 1e-2);
      TS_ASSERT_DELTA(roots[4], 2 * M_PI, 1e-4);
    }
  }

private:
  void do_test_values(const SimpleChebfun &cheb,
                      std::function<double(double)> fun, double accur1 = 1e-14,
                      double accur2 = 1e-14) {
    TS_ASSERT(cheb.size() > 0);
    TS_ASSERT(cheb.width() > 0.0);
    if (cheb.isGood()) {
      size_t n = static_cast<size_t>(1.3 * static_cast<double>(cheb.size()));
      auto x = cheb.linspace(n);
      for (size_t i = 0; i < n; ++i) {
        // std::cerr << x[i] << ' ' << cheb(x[i]) - fun(x[i]) << '\n';
        TS_ASSERT_DELTA(cheb(x[i]), fun(x[i]), accur1);
      }
    }
    auto &xp = cheb.xPoints();
    for (double i : xp) {
      // std::cerr << xp[i] << ' ' << cheb(xp[i]) - fun(xp[i]) << '\n';
      TS_ASSERT_DELTA(cheb(i), fun(i), accur2);
    }
  }
};

#endif /* MANTID_CURVEFITTING_SIMPLECHEBFUNTEST_H_ */