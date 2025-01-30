// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidSINQ/PoldiFitPeaks1D2.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/IPeakFunction.h"

#include "MantidCurveFitting/Functions/FlatBackground.h"

#include "MantidAPI/FunctionDomain1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeak.h"

using Mantid::Poldi::PoldiFitPeaks1D2;
using namespace Mantid::Poldi;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

class TestablePoldiFitPeaks1D2 : public Mantid::Poldi::PoldiFitPeaks1D2 {
  friend class PoldiFitPeaks1D2Test;

public:
  TestablePoldiFitPeaks1D2() : PoldiFitPeaks1D2() {}
};

class PoldiFitPeaks1D2Test : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PoldiFitPeaks1D2Test *createSuite() { return new PoldiFitPeaks1D2Test(); }
  static void destroySuite(PoldiFitPeaks1D2Test *suite) { delete suite; }

  PoldiFitPeaks1D2Test() {
    m_testPeak = PoldiPeak::create(MillerIndices(1, 1, 1), UncertainValue(1.108329), UncertainValue(2948.231),
                                   UncertainValue(0.002));
    m_profileTestFunction = std::string("Gaussian");
    m_backgroundTestFunction = IFunction_sptr(new FlatBackground);
    m_backgroundTestFunction->initialize();
  }

  void testSetPeakFunction() {
    TestablePoldiFitPeaks1D2 poldiFitPeaks;
    poldiFitPeaks.setPeakFunction(m_profileTestFunction);

    TS_ASSERT_EQUALS(poldiFitPeaks.m_profileTemplate, m_profileTestFunction);
  }

  void testGetPeakProfile() {
    TestablePoldiFitPeaks1D2 poldiFitPeaks;
    poldiFitPeaks.initialize();
    poldiFitPeaks.setPeakFunction(m_profileTestFunction);

    IFunction_sptr peakFunction = poldiFitPeaks.getPeakProfile(m_testPeak);

    // make sure that the profile is correct
    IPeakFunction_sptr profile = std::dynamic_pointer_cast<IPeakFunction>(peakFunction);
    TS_ASSERT(profile);

    TS_ASSERT_EQUALS(profile->centre(), m_testPeak->q());
    TS_ASSERT_EQUALS(profile->height(), m_testPeak->intensity());
    TS_ASSERT_EQUALS(profile->fwhm(), m_testPeak->fwhm(PoldiPeak::AbsoluteQ));
  }

  void testSetValuesFromProfileFunction() {
    TestablePoldiFitPeaks1D2 poldiFitPeaks;
    poldiFitPeaks.initialize();
    poldiFitPeaks.setPeakFunction(m_profileTestFunction);

    IFunction_sptr totalProfile = poldiFitPeaks.getPeakProfile(m_testPeak);

    // now we have a profile with known parameters. assign them to a new
    // PoldiPeak
    PoldiPeak_sptr newPeak = PoldiPeak::create(1.0);
    poldiFitPeaks.setValuesFromProfileFunction(newPeak, totalProfile);

    TS_ASSERT_EQUALS(newPeak->q(), m_testPeak->q());
    TS_ASSERT_EQUALS(newPeak->intensity(), m_testPeak->intensity());
    TS_ASSERT_EQUALS(newPeak->fwhm(PoldiPeak::AbsoluteQ), m_testPeak->fwhm(PoldiPeak::AbsoluteQ));
  }

  void testProperties() {
    Mantid::Poldi::PoldiFitPeaks1D2 fitPeaks1D;
    fitPeaks1D.initialize();

    TS_ASSERT_EQUALS(fitPeaks1D.propertyCount(), 8);

    std::vector<Property *> properties = fitPeaks1D.getProperties();
    std::unordered_set<std::string> names;

    for (auto &property : properties) {
      names.insert(property->name());
    }

    TS_ASSERT_EQUALS(names.count("InputWorkspace"), 1);
    TS_ASSERT_EQUALS(names.count("FwhmMultiples"), 1);
    TS_ASSERT_EQUALS(names.count("MaximumRelativeFwhm"), 1);
    TS_ASSERT_EQUALS(names.count("PeakFunction"), 1);
    TS_ASSERT_EQUALS(names.count("PoldiPeakTable"), 1);
    TS_ASSERT_EQUALS(names.count("OutputWorkspace"), 1);
    TS_ASSERT_EQUALS(names.count("FitPlotsWorkspace"), 1);
    TS_ASSERT_EQUALS(names.count("AllowedOverlap"), 1);
  }

  void testRefinedRangePeakConstructor() {
    double fwhm = m_testPeak->fwhm();
    double peakQ = m_testPeak->q();
    double rangeXStart = peakQ - 2.0 * fwhm;
    double rangeXEnd = peakQ + 2.0 * fwhm;

    RefinedRange range(m_testPeak, 2.0);
    TS_ASSERT_EQUALS(range.getXStart(), rangeXStart);
    TS_ASSERT_EQUALS(range.getXEnd(), rangeXEnd);
    TS_ASSERT_DELTA(range.getWidth(), 4.0 * fwhm, 1e-15);

    // Null pointer does not work
    PoldiPeak_sptr nullPeak;
    TS_ASSERT_THROWS(RefinedRange invalid(nullPeak, 2.0), const std::invalid_argument &);

    // 0 or fewer multiples does not work
    TS_ASSERT_THROWS(RefinedRange invalid(m_testPeak, 0.0), const std::invalid_argument &);
    TS_ASSERT_THROWS(RefinedRange invalid(m_testPeak, -1.0), const std::invalid_argument &);
  }

  void testRefinedRangeLimitConstructor() {
    std::vector<PoldiPeak_sptr> peaks(1, m_testPeak);

    TS_ASSERT_THROWS_NOTHING(RefinedRange range(0.0, 1.0, peaks));
    TS_ASSERT_THROWS(RefinedRange invalid(1.0, 0.0, peaks), const std::invalid_argument &);
    TS_ASSERT_THROWS(RefinedRange invalid(1.0, 1.0, peaks), const std::invalid_argument &);

    RefinedRange range(3.0, 4.0, peaks);
    TS_ASSERT_EQUALS(range.getXStart(), 3.0);
    TS_ASSERT_EQUALS(range.getXEnd(), 4.0);
    TS_ASSERT_EQUALS(range.getWidth(), 1.0);
  }

  void testContains() {
    std::vector<PoldiPeak_sptr> peaks(1, m_testPeak);

    RefinedRange largeRange(1.0, 3.0, peaks);
    RefinedRange smallRange(1.5, 2.5, peaks);

    TS_ASSERT(largeRange.contains(smallRange));
    TS_ASSERT(!smallRange.contains(largeRange));

    RefinedRange outsideRange(2.5, 4.5, peaks);
    TS_ASSERT(!largeRange.contains(outsideRange));
  }

  void testOperatorLessThan() {
    std::vector<PoldiPeak_sptr> peaks(1, m_testPeak);

    RefinedRange firstRange(1.0, 3.0, peaks);
    RefinedRange secondRange(1.5, 2.5, peaks);

    TS_ASSERT(firstRange < secondRange);
    TS_ASSERT(!(secondRange < firstRange));
  }

  void testMerge() {
    std::vector<PoldiPeak_sptr> peaks(1, m_testPeak);

    RefinedRange firstRange(1.0, 2.0, peaks);
    RefinedRange secondRange(1.5, 3.5, peaks);

    TS_ASSERT_THROWS_NOTHING(firstRange.merge(secondRange));
    TS_ASSERT_EQUALS(firstRange.getXStart(), 1.0);
    TS_ASSERT_EQUALS(firstRange.getXEnd(), 3.5);
    TS_ASSERT_EQUALS(firstRange.getWidth(), 2.5);
  }

  void testGetOverlap() {
    std::vector<PoldiPeak_sptr> peaks(1, m_testPeak);

    RefinedRange firstRange(1.0, 2.0, peaks);
    RefinedRange secondRange(1.5, 3.5, peaks);

    TS_ASSERT_EQUALS(firstRange.getOverlapFraction(secondRange), 0.5);
    TS_ASSERT_EQUALS(secondRange.getOverlapFraction(firstRange), 0.25);

    RefinedRange noOverlapLeft(0.0, 0.5, peaks);
    TS_ASSERT_EQUALS(firstRange.getOverlapFraction(noOverlapLeft), 0.0);
    TS_ASSERT_EQUALS(noOverlapLeft.getOverlapFraction(firstRange), 0.0);

    RefinedRange noOverlapRight(4.0, 4.5, peaks);
    TS_ASSERT_EQUALS(firstRange.getOverlapFraction(noOverlapRight), 0.0);
    TS_ASSERT_EQUALS(noOverlapRight.getOverlapFraction(firstRange), 0.0);

    RefinedRange noOverlapLeftLimit(0.0, 1.0, peaks);
    TS_ASSERT_EQUALS(firstRange.getOverlapFraction(noOverlapLeftLimit), 0.0);
    TS_ASSERT_EQUALS(noOverlapLeftLimit.getOverlapFraction(firstRange), 0.0);

    RefinedRange contained(2.0, 2.5, peaks);
    TS_ASSERT_EQUALS(secondRange.getOverlapFraction(contained), 0.25);
  }

  void testOverlaps() {
    std::vector<PoldiPeak_sptr> peaks(1, m_testPeak);

    RefinedRange firstRange(1.0, 2.0, peaks);
    RefinedRange secondRange(1.5, 3.5, peaks);

    TS_ASSERT(firstRange.overlaps(secondRange));
    TS_ASSERT(secondRange.overlaps(firstRange));

    RefinedRange noOverlapLeft(0.0, 0.5, peaks);
    TS_ASSERT(!firstRange.overlaps(noOverlapLeft));
  }

  void testOverlapsFraction() {
    std::vector<PoldiPeak_sptr> peaks(1, m_testPeak);

    RefinedRange firstRange(1.0, 2.0, peaks);
    RefinedRange secondRange(1.5, 3.5, peaks);

    TS_ASSERT(firstRange.overlaps(secondRange, 0.1));
    TS_ASSERT(firstRange.overlaps(secondRange, 0.15));
    TS_ASSERT(!firstRange.overlaps(secondRange, 0.55));
  }

  void testPeakIsAcceptable() {
    TestablePoldiFitPeaks1D2 poldiPeakFit;
    poldiPeakFit.m_maxRelativeFwhm = 0.02;

    // The testpeak is acceptable
    TS_ASSERT(poldiPeakFit.peakIsAcceptable(m_testPeak));

    // Peaks with certain properties are rejected
    // 1. negative intensity
    PoldiPeak_sptr negativeIntensity = m_testPeak->clonePeak();
    negativeIntensity->setIntensity(UncertainValue(-190.0));
    TS_ASSERT(!poldiPeakFit.peakIsAcceptable(negativeIntensity));

    // 2a. FWHM too large (rel. > 0.02)
    PoldiPeak_sptr tooBroad = m_testPeak->clonePeak();
    tooBroad->setFwhm(UncertainValue(0.021), PoldiPeak::Relative);
    TS_ASSERT(!poldiPeakFit.peakIsAcceptable(tooBroad));

    // 2b. Changing acceptable FWHM
    poldiPeakFit.m_maxRelativeFwhm = 0.03;
    TS_ASSERT(poldiPeakFit.peakIsAcceptable(tooBroad));

    // 3. FWHM too small (rel. < 0.001)
    PoldiPeak_sptr tooNarrow = m_testPeak->clonePeak();
    tooNarrow->setFwhm(UncertainValue(0.0009), PoldiPeak::Relative);
    TS_ASSERT(!poldiPeakFit.peakIsAcceptable(tooNarrow));
  }

  void testGetBestChebyshevPolynomialDegree() {
    TestablePoldiFitPeaks1D2 poldiFitPeaks;
    poldiFitPeaks.initialize();
    poldiFitPeaks.setPeakFunction(m_profileTestFunction);
    // Get workspace with a peak and constant background
    Workspace2D_sptr ws = getWorkspaceWithPeakCurve();

    // Chebyshev polynomial should be best at 0 (= constant)
    RefinedRange_sptr range(new RefinedRange(m_testPeak, 3.0));
    int chebyshevDegree = poldiFitPeaks.getBestChebyshevPolynomialDegree(ws, range);
    TS_ASSERT_EQUALS(chebyshevDegree, 0);

    // If range is too small, Fit will have problems, so the function should
    // return -1 to indicate failure.
    double center = m_testPeak->q();
    RefinedRange_sptr invalidRange(
        new RefinedRange(center - 0.001, center + 0.001, std::vector<PoldiPeak_sptr>(1, m_testPeak)));

    int invalidChebyshevDegree = poldiFitPeaks.getBestChebyshevPolynomialDegree(ws, invalidRange);
    TS_ASSERT_EQUALS(invalidChebyshevDegree, -1);
  }

private:
  Workspace2D_sptr getWorkspaceWithPeakCurve() {
    // Get a peak function for one peak
    TestablePoldiFitPeaks1D2 poldiFitPeaks;
    poldiFitPeaks.initialize();
    poldiFitPeaks.setPeakFunction(m_profileTestFunction);

    IFunction_sptr peakFunction = poldiFitPeaks.getPeakProfile(m_testPeak);

    // calculate the data
    double center = m_testPeak->q();
    double fwhm = m_testPeak->fwhm();

    FunctionDomain1DVector domain(center - 4.0 * fwhm, center + 4.0 * fwhm, 50);
    FunctionValues values(domain);

    peakFunction->function(domain, values);

    // put it into a workspace
    Workspace2D_sptr ws = WorkspaceCreationHelper::create1DWorkspaceConstant(50, 0.0, 1.0, false);
    std::vector<double> &x = ws->dataX(0);
    std::vector<double> &y = ws->dataY(0);

    for (size_t i = 0; i < x.size(); ++i) {
      x[i] = domain[i];
      y[i] = values[i] + 234.5;
    }

    return ws;
  }

  PoldiPeak_sptr m_testPeak;
  std::string m_profileTestFunction;
  IFunction_sptr m_backgroundTestFunction;
};
