#ifndef MANTID_ALGORITHMS_STITCH1DTEST_H_
#define MANTID_ALGORITHMS_STITCH1DTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/Stitch1D.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include <algorithm>
#include <boost/assign/list_of.hpp>
#include <boost/tuple/tuple.hpp>

using namespace Mantid::API;
using namespace boost::assign;
using Mantid::Algorithms::Stitch1D;
using Mantid::MantidVec;

class Stitch1DTest: public CxxTest::TestSuite
{
private:

  template<typename T>
  class LinearSequence
  {
  private:
    const T m_start;
    const T m_step;
    T m_stepNumber;
  public:
    LinearSequence(const T& start, const T& step) :
        m_start(start), m_step(step), m_stepNumber(T(0))
    {
    }

    T operator()()
    {
      T result = m_start + (m_step * m_stepNumber);
      m_stepNumber += 1;
      return result;
    }

  };

  MatrixWorkspace_sptr create1DWorkspace(MantidVec& xData, MantidVec& yData, MantidVec& eData)
  {
    auto createWorkspace = AlgorithmManager::Instance().create("CreateWorkspace");
    createWorkspace->setChild(true);
    createWorkspace->initialize();
    createWorkspace->setProperty("UnitX", "1/q");
    createWorkspace->setProperty("DataX", xData);
    createWorkspace->setProperty("DataY", yData);
    createWorkspace->setProperty("NSpec", 1);
    createWorkspace->setProperty("DataE", eData);
    createWorkspace->setPropertyValue("OutputWorkspace", "dummy");
    createWorkspace->execute();
    MatrixWorkspace_sptr outWS = createWorkspace->getProperty("OutputWorkspace");
    return outWS;
  }

  MatrixWorkspace_sptr a;
  MatrixWorkspace_sptr b;
  MantidVec x;
  MantidVec e;
  typedef boost::tuple<MatrixWorkspace_sptr, double> ResultType;

  MatrixWorkspace_sptr make_arbitrary_point_ws()
  {
    auto x = MantidVec(3);
    const double xstart = -1;
    const double xstep = 0.2;
    LinearSequence<MantidVec::value_type> sequenceX(xstart, xstep);
    std::generate(x.begin(), x.end(), sequenceX);

    auto y = MantidVec(3);
    const double ystart = 1;
    const double ystep = 1;
    LinearSequence<MantidVec::value_type> sequenceY(ystart, ystep);
    std::generate(y.begin(), y.end(), sequenceY);

    auto e = MantidVec(3, 1);

    return create1DWorkspace(x, y, e);
  }

  MatrixWorkspace_sptr make_arbitrary_histogram_ws()
  {
    auto x = MantidVec(3);
    const double xstart = -1;
    const double xstep = 0.2;
    LinearSequence<MantidVec::value_type> sequenceX(xstart, xstep);
    std::generate(x.begin(), x.end(), sequenceX);

    auto y = MantidVec(2);
    const double ystart = 1;
    const double ystep = 1;
    LinearSequence<MantidVec::value_type> sequenceY(ystart, ystep);
    std::generate(y.begin(), y.end(), sequenceY);

    auto e = MantidVec(2, 1);

    return create1DWorkspace(x, y, e);
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static Stitch1DTest *createSuite()
  {
    return new Stitch1DTest();
  }
  static void destroySuite(Stitch1DTest *suite)
  {
    delete suite;
  }

  Stitch1DTest()
  {
    FrameworkManager::Instance();

    e = MantidVec(10, 0);
    x = MantidVec(11);
    const double xstart = -1;
    const double xstep = 0.2;
    LinearSequence<MantidVec::value_type> sequence(xstart, xstep);
    std::generate(x.begin(), x.end(), sequence);

    MantidVec y = boost::assign::list_of(0)(0)(0)(3)(3)(3)(3)(3)(3)(3).convert_to_container<MantidVec>();

    // Pre-canned workspace to stitch
    a = create1DWorkspace(x, y, e);

    y = boost::assign::list_of(2)(2)(2)(2)(2)(2)(2)(0)(0)(0).convert_to_container<MantidVec>();
    // Another pre-canned workspace to stitch
    b = create1DWorkspace(x, y, e);

  }

  ResultType do_stitch1D(MatrixWorkspace_sptr& lhs, MatrixWorkspace_sptr& rhs,
      const double& startOverlap, const double& endOverlap, const MantidVec& params)
  {
    Stitch1D alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("LHSWorkspace", lhs);
    alg.setProperty("RHSWorkspace", rhs);
    alg.setProperty("StartOverlap", startOverlap);
    alg.setProperty("EndOverlap", endOverlap);
    alg.setProperty("Params", params);
    alg.setPropertyValue("OutputWorkspace", "dummy_value");
    alg.execute();
    MatrixWorkspace_sptr stitched = alg.getProperty("OutputWorkspace");
    double scaleFactor = alg.getProperty("OutScaleFactor");
    return ResultType(stitched, scaleFactor);
  }

  void test_Init()
  {
    Stitch1D alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize())
    TS_ASSERT( alg.isInitialized())
  }

  void test_startoverlap_greater_than_end_overlap_throws()
  {
    TSM_ASSERT_THROWS("Should have thrown with StartOverlap < x max",
        do_stitch1D(this->a, this->b, this->x.back(), this->x.front(), MantidVec(1, 0.2)),
        std::runtime_error&);
  }

  void test_lhsworkspace_must_be_histogram()
  {
    auto lhs_ws = make_arbitrary_point_ws();
    auto rhs_ws = make_arbitrary_histogram_ws();
    TSM_ASSERT_THROWS("LHS WS must be histogram", do_stitch1D(lhs_ws, rhs_ws, -1, 1, MantidVec(0.2)),
        std::invalid_argument&);
  }

  void test_rhsworkspace_must_be_histogram()
  {
    auto lhs_ws = make_arbitrary_histogram_ws();
    auto rhs_ws = make_arbitrary_point_ws();
    TSM_ASSERT_THROWS("RHS WS must be histogram", do_stitch1D(lhs_ws, rhs_ws, -1, 1, MantidVec(0.2)),
        std::invalid_argument&);
  }

  void test_stitching_uses_suppiled_params()
  {
    auto ret = do_stitch1D(this->b, this->a, -0.4, 0.4,
        boost::assign::list_of<double>(-0.8)(0.2)(1.0).convert_to_container<MantidVec>());

    MantidVec xValues = ret.get<0>()->readX(0); // Get the output workspace and look at the limits.

    double xMin = xValues.front();
    double xMax = xValues.back();

    TS_ASSERT_EQUALS(xMin, -0.8);
    TS_ASSERT_EQUALS(xMax, 1.0);
  }

};

#endif /* MANTID_ALGORITHMS_STITCH1DTEST_H_ */
