// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"

#include "MantidKernel/Matrix.h"

#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidSINQ/PoldiFitPeaks2D.h"
#include "MantidSINQ/PoldiUtilities/PoldiMockInstrumentHelpers.h"
#include "MantidSINQ/PoldiUtilities/PoldiSpectrumDomainFunction.h"

using namespace Mantid::Poldi;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::LinearGenerator;

class PoldiFitPeaks2DTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PoldiFitPeaks2DTest *createSuite() { return new PoldiFitPeaks2DTest(); }
  static void destroySuite(PoldiFitPeaks2DTest *suite) { delete suite; }

  PoldiFitPeaks2DTest() {
    FrameworkManager::Instance();

    m_instrument = PoldiInstrumentAdapter_sptr(new FakePoldiInstrumentAdapter);
    m_timeTransformer = PoldiTimeTransformer_sptr(new PoldiTimeTransformer(m_instrument));
  }

  void testSetTimeTransformer() {
    TestablePoldiFitPeaks2D spectrumCalculator;
    spectrumCalculator.setTimeTransformer(m_timeTransformer);

    TS_ASSERT_EQUALS(spectrumCalculator.m_timeTransformer, m_timeTransformer);
  }

  void testSetTimeTransformerFromInstrument() {
    TestablePoldiFitPeaks2D spectrumCalculator;
    spectrumCalculator.setTimeTransformerFromInstrument(m_instrument);

    TS_ASSERT(spectrumCalculator.m_timeTransformer);
  }

  void testSetDeltaT() {
    TestablePoldiFitPeaks2D spectrumCalculator;
    TS_ASSERT_THROWS_NOTHING(spectrumCalculator.setDeltaT(2.0));
    TS_ASSERT_EQUALS(spectrumCalculator.m_deltaT, 2.0);

    TS_ASSERT_THROWS(spectrumCalculator.setDeltaT(0.0), const std::invalid_argument &);
    TS_ASSERT_THROWS(spectrumCalculator.setDeltaT(-1.0), const std::invalid_argument &);
  }

  void testSetDeltaTFromWorkspace() {
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(1, 10);
    ws->setBinEdges(0, BinEdges(ws->x(0).size(), LinearGenerator(0, 1)));
    TestablePoldiFitPeaks2D spectrumCalculator;
    spectrumCalculator.setDeltaTFromWorkspace(ws);
    TS_ASSERT_EQUALS(spectrumCalculator.m_deltaT, 1.0);

    MatrixWorkspace_sptr invalidWs = WorkspaceCreationHelper::create2DWorkspace123(1, 1);
    TS_ASSERT_THROWS(spectrumCalculator.setDeltaTFromWorkspace(invalidWs), const std::invalid_argument &);
  }

  void testIsValidDeltaT() {
    TestablePoldiFitPeaks2D spectrumCalculator;
    TS_ASSERT_EQUALS(spectrumCalculator.isValidDeltaT(1.0), true);
    TS_ASSERT_EQUALS(spectrumCalculator.isValidDeltaT(0.0), false);
    TS_ASSERT_EQUALS(spectrumCalculator.isValidDeltaT(-1.0), false);
  }

  void testGetPeakCollection() {
    TestablePoldiFitPeaks2D spectrumCalculator;

    TableWorkspace_sptr peakTable = PoldiPeakCollectionHelpers::createPoldiPeakTableWorkspace();
    TS_ASSERT_THROWS_NOTHING(spectrumCalculator.getPeakCollection(peakTable));

    PoldiPeakCollection_sptr collection = spectrumCalculator.getPeakCollection(peakTable);
    TS_ASSERT_EQUALS(collection->peakCount(), peakTable->rowCount());
  }

  void testGetIntegratedPeakCollection() {
    PoldiPeakCollection_sptr testPeaks = PoldiPeakCollectionHelpers::createPoldiPeakCollectionMaximum();

    TestablePoldiFitPeaks2D spectrumCalculator;
    spectrumCalculator.initialize();
    // deltaT is not set, so this must fail
    TS_ASSERT_THROWS(spectrumCalculator.getIntegratedPeakCollection(testPeaks), const std::invalid_argument &);
    spectrumCalculator.setDeltaT(3.0);

    // still fails, because time transformer is required.
    TS_ASSERT_THROWS(spectrumCalculator.getIntegratedPeakCollection(testPeaks), const std::invalid_argument &);
    spectrumCalculator.setTimeTransformer(m_timeTransformer);

    // peak collection with some peaks, intensities are described by maximum,
    // this is the "happy case".
    TS_ASSERT_THROWS_NOTHING(spectrumCalculator.getIntegratedPeakCollection(testPeaks));

    PoldiPeakCollection_sptr integratedTestPeaks = spectrumCalculator.getIntegratedPeakCollection(testPeaks);
    // this should be a new peak collection
    TS_ASSERT(integratedTestPeaks != testPeaks);
    TS_ASSERT_EQUALS(integratedTestPeaks->peakCount(), testPeaks->peakCount());

    // checking the actual integration result agains reference.
    PoldiPeakCollection_sptr integratedReference(new PoldiPeakCollection(PoldiPeakCollection::Integral));
    for (size_t i = 0; i < testPeaks->peakCount(); ++i) {
      PoldiPeak_sptr peak = testPeaks->peak(i)->clonePeak();

      double oldIntensity = peak->intensity();
      double fwhm = peak->fwhm(PoldiPeak::AbsoluteD);

      // Integral of gaussian is height * sigma * sqrt(2 * pi)
      peak->setIntensity(UncertainValue(oldIntensity * fwhm / (2.0 * sqrt(M_LN2)) * sqrt(M_PI)));
      integratedReference->addPeak(peak);
    }

    // compare result to relative error of 1e-6
    compareIntensities(integratedTestPeaks, integratedReference, 1e-6);

    // In case of integrated peaks nothing should happen
    PoldiPeakCollection_sptr alreadyIntegratedPeaks(new PoldiPeakCollection(PoldiPeakCollection::Integral));
    alreadyIntegratedPeaks->addPeak(PoldiPeak::create(2.0));

    PoldiPeakCollection_sptr alreadyIntegratedResult =
        spectrumCalculator.getIntegratedPeakCollection(alreadyIntegratedPeaks);
    TS_ASSERT(alreadyIntegratedResult != alreadyIntegratedPeaks);
    TS_ASSERT_EQUALS(alreadyIntegratedResult->peakCount(), alreadyIntegratedPeaks->peakCount());
    TS_ASSERT_EQUALS(alreadyIntegratedResult->peak(0)->d(), alreadyIntegratedPeaks->peak(0)->d());

    // Where there's no profile function in the peak collection, falling back to
    // the property PeakProfileFunction.
    // Default is Gaussian, so this is supposed to work.
    PoldiPeakCollection_sptr noProfilePeaks(new PoldiPeakCollection);
    TS_ASSERT_THROWS_NOTHING(spectrumCalculator.getIntegratedPeakCollection(noProfilePeaks));

    // When there is no valid PoldiPeakCollection, the method also throws
    PoldiPeakCollection_sptr invalidPeakCollection;
    TS_ASSERT_THROWS(spectrumCalculator.getIntegratedPeakCollection(invalidPeakCollection),
                     const std::invalid_argument &);
  }

  void testGetNormalizedPeakCollection() {
    TestablePoldiFitPeaks2D spectrumCalculator;

    // first, test the failing cases
    PoldiPeakCollection_sptr invalidPeakCollection;
    TS_ASSERT_THROWS(spectrumCalculator.getNormalizedPeakCollection(invalidPeakCollection),
                     const std::invalid_argument &);

    // m_timeTransformer has not been assigned, so even a "good" PeakCollection
    // will throw
    PoldiPeakCollection_sptr testPeaks = PoldiPeakCollectionHelpers::createPoldiPeakCollectionMaximum();
    TS_ASSERT_THROWS(spectrumCalculator.getNormalizedPeakCollection(testPeaks), const std::invalid_argument &);

    spectrumCalculator.setTimeTransformer(m_timeTransformer);

    // to verify the results, use actual results from after integration step
    PoldiPeakCollection_sptr integratedPeaks = PoldiPeakCollectionHelpers::createPoldiPeakCollectionIntegral();
    TS_ASSERT_THROWS_NOTHING(spectrumCalculator.getNormalizedPeakCollection(integratedPeaks));

    PoldiPeakCollection_sptr normalizedPeaks = spectrumCalculator.getNormalizedPeakCollection(integratedPeaks);
    PoldiPeakCollection_sptr normalizedReferencePeaks =
        PoldiPeakCollectionHelpers::createPoldiPeakCollectionNormalized();

    compareIntensities(normalizedPeaks, normalizedReferencePeaks, 1.5e-6);
  }

  void testGetCountPeakCollection() {
    TestablePoldiFitPeaks2D spectrumCalculator;

    // first, test the failing cases
    PoldiPeakCollection_sptr invalidPeakCollection;
    TS_ASSERT_THROWS(spectrumCalculator.getCountPeakCollection(invalidPeakCollection), const std::invalid_argument &);

    // m_timeTransformer has not been assigned, so even a "good" PeakCollection
    // will throw
    PoldiPeakCollection_sptr testPeaks = PoldiPeakCollectionHelpers::createPoldiPeakCollectionNormalized();
    TS_ASSERT_THROWS(spectrumCalculator.getCountPeakCollection(testPeaks), const std::invalid_argument &);

    spectrumCalculator.setTimeTransformer(m_timeTransformer);

    // to verify the results, use actual results from after integration step
    PoldiPeakCollection_sptr normalizedPeaks = PoldiPeakCollectionHelpers::createPoldiPeakCollectionNormalized();
    TS_ASSERT_THROWS_NOTHING(spectrumCalculator.getCountPeakCollection(normalizedPeaks));

    PoldiPeakCollection_sptr integratedPeaks = spectrumCalculator.getCountPeakCollection(normalizedPeaks);
    PoldiPeakCollection_sptr integratedReferencePeaks = PoldiPeakCollectionHelpers::createPoldiPeakCollectionIntegral();

    compareIntensities(integratedPeaks, integratedReferencePeaks, 1.5e-6);
  }

  void testGetFunctionFromPeakCollection() {
    TestablePoldiFitPeaks2D spectrumCalculator;
    spectrumCalculator.initialize();

    PoldiPeakCollection_sptr peaks = PoldiPeakCollectionHelpers::createPoldiPeakCollectionNormalized();

    std::shared_ptr<Poldi2DFunction> poldi2DFunction = spectrumCalculator.getFunctionFromPeakCollection(peaks);

    TS_ASSERT_EQUALS(poldi2DFunction->nFunctions(), peaks->peakCount());

    for (size_t i = 0; i < poldi2DFunction->nFunctions(); ++i) {
      IFunction_sptr rawFunction = poldi2DFunction->getFunction(i);
      std::shared_ptr<PoldiSpectrumDomainFunction> poldiFunction =
          std::dynamic_pointer_cast<PoldiSpectrumDomainFunction>(rawFunction);

      TS_ASSERT(poldiFunction);

      IPeakFunction_sptr wrappedFunction = poldiFunction->getProfileFunction();

      TS_ASSERT_DELTA(wrappedFunction->intensity(), static_cast<double>(peaks->peak(i)->intensity()), 1e-10);
    }
  }

  void testGetUserDefinedTies() {
    TestablePoldiFitPeaks2D spectrumCalculator;
    spectrumCalculator.initialize();
    spectrumCalculator.setProperty("PeakProfileFunction", "Gaussian");

    // Create a function with some peaks
    PoldiPeakCollection_sptr peaks = PoldiPeakCollectionHelpers::createPoldiPeakCollectionNormalized();
    std::shared_ptr<Poldi2DFunction> poldi2DFunction = spectrumCalculator.getFunctionFromPeakCollection(peaks);

    // Make "Height" global, i.e. the same for all peaks
    spectrumCalculator.setProperty("GlobalParameters", "Height");
    std::string ties = spectrumCalculator.getUserSpecifiedTies(poldi2DFunction);
    TS_ASSERT_EQUALS(ties, "f1.Height=f0.Height,f2.Height=f0.Height,f3.Height=f0.Height");

    // Height and Sigma
    spectrumCalculator.setProperty("GlobalParameters", "Height,Sigma");
    ties = spectrumCalculator.getUserSpecifiedTies(poldi2DFunction);

    TS_ASSERT_EQUALS(ties, "f1.Height=f0.Height,f2.Height=f0.Height,f3.Height=f0.Height,"
                           "f1.Sigma=f0.Sigma,f2.Sigma=f0.Sigma,f3.Sigma=f0.Sigma");

    // Empty
    spectrumCalculator.setProperty("GlobalParameters", "");
    ties = spectrumCalculator.getUserSpecifiedTies(poldi2DFunction);
    TS_ASSERT(ties.empty());

    // Invalid name
    spectrumCalculator.setProperty("GlobalParameters", "Invalid");
    ties = spectrumCalculator.getUserSpecifiedTies(poldi2DFunction);
    TS_ASSERT(ties.empty());

    // Valid and invalid
    spectrumCalculator.setProperty("GlobalParameters", "Height,Invalid");
    ties = spectrumCalculator.getUserSpecifiedTies(poldi2DFunction);
    TS_ASSERT_EQUALS(ties, "f1.Height=f0.Height,f2.Height=f0.Height,f3.Height=f0.Height");

    // Several empty
    spectrumCalculator.setProperty("GlobalParameters", ",,,,");
    ties = spectrumCalculator.getUserSpecifiedTies(poldi2DFunction);
    TS_ASSERT(ties.empty());
  }

  void testGetUserSpecifiedBounds() {
    TestablePoldiFitPeaks2D spectrumCalculator;
    spectrumCalculator.initialize();
    spectrumCalculator.setProperty("PeakProfileFunction", "AsymmetricPearsonVII");

    // Create a function with some peaks
    PoldiPeakCollection_sptr peaks = PoldiPeakCollectionHelpers::createPoldiPeakCollectionNormalized();
    peaks->setProfileFunctionName("AsymmetricPearsonVII");
    std::shared_ptr<Poldi2DFunction> poldi2DFunction = spectrumCalculator.getFunctionFromPeakCollection(peaks);

    // Bounds "LeftShape" and "RightShape" to a [1, 20] range
    spectrumCalculator.setProperty("BoundedParameters", "LeftShape,RightShape");
    std::string bounds = spectrumCalculator.getUserSpecifiedBounds(poldi2DFunction);
    bounds = spectrumCalculator.getUserSpecifiedBounds(poldi2DFunction);

    TS_ASSERT_EQUALS(bounds, "1.0<=f0.LeftShape<=20.0,1.0<=f1.LeftShape<=20.0,"
                             "1.0<=f2.LeftShape<=20.0,1.0<=f3.LeftShape<=20.0,"
                             "1.0<=f0.RightShape<=20.0,1.0<=f1.RightShape<=20.0,"
                             "1.0<=f2.RightShape<=20.0,1.0<=f3.RightShape<=20.0");
  }

  void testEmptyBounds() {
    TestablePoldiFitPeaks2D spectrumCalculator;
    spectrumCalculator.initialize();
    spectrumCalculator.setProperty("PeakProfileFunction", "AsymmetricPearsonVII");

    // Create a function with some peaks
    PoldiPeakCollection_sptr peaks = PoldiPeakCollectionHelpers::createPoldiPeakCollectionNormalized();
    peaks->setProfileFunctionName("AsymmetricPearsonVII");
    std::shared_ptr<Poldi2DFunction> poldi2DFunction = spectrumCalculator.getFunctionFromPeakCollection(peaks);

    // Empty
    spectrumCalculator.setProperty("BoundedParameters", "");
    std::string bounds = spectrumCalculator.getUserSpecifiedBounds(poldi2DFunction);
    bounds = spectrumCalculator.getUserSpecifiedBounds(poldi2DFunction);
    TS_ASSERT(bounds.empty());

    // Several empty
    spectrumCalculator.setProperty("BoundedParameters", ",,,,");
    bounds = spectrumCalculator.getUserSpecifiedBounds(poldi2DFunction);
    TS_ASSERT(bounds.empty());
  }

  void testInvalidBounds() {
    TestablePoldiFitPeaks2D spectrumCalculator;
    spectrumCalculator.initialize();
    spectrumCalculator.setProperty("PeakProfileFunction", "AsymmetricPearsonVII");

    // Create a function with some peaks
    PoldiPeakCollection_sptr peaks = PoldiPeakCollectionHelpers::createPoldiPeakCollectionNormalized();
    peaks->setProfileFunctionName("AsymmetricPearsonVII");
    std::shared_ptr<Poldi2DFunction> poldi2DFunction = spectrumCalculator.getFunctionFromPeakCollection(peaks);

    // Invalid name
    spectrumCalculator.setProperty("BoundedParameters", "Invalid");
    std::string bounds = spectrumCalculator.getUserSpecifiedBounds(poldi2DFunction);
    bounds = spectrumCalculator.getUserSpecifiedBounds(poldi2DFunction);
    TS_ASSERT(bounds.empty());

    // Valid and invalid
    spectrumCalculator.setProperty("BoundedParameters", "LeftShape,Invalid");
    bounds = spectrumCalculator.getUserSpecifiedBounds(poldi2DFunction);
    TS_ASSERT_EQUALS(bounds, "1.0<=f0.LeftShape<=20.0,1.0<=f1.LeftShape<=20.0,"
                             "1.0<=f2.LeftShape<=20.0,1.0<=f3.LeftShape<=20.0");
  }

  void testGetPeakCollectionFromFunction() {
    TestablePoldiFitPeaks2D spectrumCalculator;
    spectrumCalculator.initialize();

    PoldiPeakCollection_sptr peaks = PoldiPeakCollectionHelpers::createPoldiPeakCollectionNormalized();
    IFunction_sptr poldi2DFunction = spectrumCalculator.getFunctionFromPeakCollection(peaks);

    size_t nParams = poldi2DFunction->nParams();

    // Make a matrix with diagonal elements = 0.05 and set as covariance matrix
    std::shared_ptr<DblMatrix> matrix = std::make_shared<DblMatrix>(nParams, nParams, true);
    matrix->operator*=(0.05);
    poldi2DFunction->setCovarianceMatrix(matrix);

    // Also set errors for old behavior
    for (size_t i = 0; i < nParams; ++i) {
      poldi2DFunction->setError(i, sqrt(0.05));
    }

    PoldiPeakCollection_sptr peaksFromFunction = spectrumCalculator.getPeakCollectionFromFunction(poldi2DFunction);

    TS_ASSERT_EQUALS(peaksFromFunction->peakCount(), peaks->peakCount());
    for (size_t i = 0; i < peaksFromFunction->peakCount(); ++i) {
      PoldiPeak_sptr functionPeak = peaksFromFunction->peak(i);
      PoldiPeak_sptr referencePeak = peaks->peak(i);

      TS_ASSERT_EQUALS(functionPeak->d().value(), referencePeak->d().value());
      TS_ASSERT_EQUALS(functionPeak->fwhm().value(), referencePeak->fwhm().value());
      TS_ASSERT_DELTA(functionPeak->d().error(), sqrt(0.05), 1e-6);
      TS_ASSERT_DELTA(functionPeak->fwhm(PoldiPeak::AbsoluteD).error(), sqrt(0.05) * (2.0 * sqrt(2.0 * M_LN2)), 1e-6);
    }
  }

  void testAssignMillerIndices() {
    PoldiPeak_sptr peak1 = PoldiPeak::create(MillerIndices(1, 1, 1), 2.0);
    PoldiPeakCollection_sptr from = std::make_shared<PoldiPeakCollection>();
    from->addPeak(peak1);

    PoldiPeak_sptr peak2 = PoldiPeak::create(Conversions::dToQ(2.0));
    PoldiPeakCollection_sptr to = std::make_shared<PoldiPeakCollection>();
    to->addPeak(peak2);

    PoldiPeakCollection_sptr invalid;

    TestablePoldiFitPeaks2D spectrumCalculator;

    TS_ASSERT_THROWS(spectrumCalculator.assignMillerIndices(from, invalid), const std::invalid_argument &);
    TS_ASSERT_THROWS(spectrumCalculator.assignMillerIndices(invalid, from), const std::invalid_argument &);
    TS_ASSERT_THROWS(spectrumCalculator.assignMillerIndices(invalid, invalid), const std::invalid_argument &);

    TS_ASSERT_DIFFERS(peak1->hkl(), peak2->hkl());

    TS_ASSERT_THROWS_NOTHING(spectrumCalculator.assignMillerIndices(from, to));
    TS_ASSERT_EQUALS(peak1->hkl(), peak2->hkl());

    to->addPeak(peak1);

    TS_ASSERT_THROWS(spectrumCalculator.assignMillerIndices(from, to), const std::runtime_error &);
  }

  void testAddBackgroundFunctions() {
    TestablePoldiFitPeaks2D spectrumCalculator;
    spectrumCalculator.initialize();

    std::shared_ptr<Poldi2DFunction> funDefault = std::make_shared<Poldi2DFunction>();
    TS_ASSERT_EQUALS(funDefault->nParams(), 0);
    TS_ASSERT_EQUALS(funDefault->nFunctions(), 0);

    spectrumCalculator.addBackgroundTerms(funDefault);
    TS_ASSERT_EQUALS(funDefault->nParams(), 2);
    TS_ASSERT_EQUALS(funDefault->nFunctions(), 2);

    std::shared_ptr<Poldi2DFunction> funLinear = std::make_shared<Poldi2DFunction>();
    spectrumCalculator.setProperty("FitConstantBackground", false);
    spectrumCalculator.addBackgroundTerms(funLinear);

    // Now there's only the linear term
    TS_ASSERT_EQUALS(funLinear->nParams(), 1);
    TS_ASSERT_EQUALS(funLinear->parameterName(0), "f0.A1");
    TS_ASSERT_EQUALS(funLinear->nFunctions(), 1);

    std::shared_ptr<Poldi2DFunction> funConstant = std::make_shared<Poldi2DFunction>();
    spectrumCalculator.setProperty("FitConstantBackground", true);
    spectrumCalculator.setProperty("FitLinearBackground", false);
    spectrumCalculator.addBackgroundTerms(funConstant);

    // Now there's only the constant term
    TS_ASSERT_EQUALS(funConstant->nParams(), 1);
    TS_ASSERT_EQUALS(funConstant->parameterName(0), "f0.A0");
    TS_ASSERT_EQUALS(funConstant->nFunctions(), 1);
  }

  void testGetRefinedStartingCell() {
    // Get Silicon peaks for testing
    PoldiPeakCollection_sptr peaks = PoldiPeakCollectionHelpers::createPoldiPeakCollectionNormalized();

    TestablePoldiFitPeaks2D alg;

    std::string refinedCell;
    TS_ASSERT_THROWS_NOTHING(refinedCell = alg.getRefinedStartingCell("5.4 5.4 5.4 90 90 90", "Cubic", peaks));

    UnitCell cell = strToUnitCell(refinedCell);
    TS_ASSERT_DELTA(cell.a(), 5.43111972, 1e-8);

    // With less accurate starting parameters the result should not change
    TS_ASSERT_THROWS_NOTHING(refinedCell = alg.getRefinedStartingCell("5 5 5 90 90 90", "Cubic", peaks));

    cell = strToUnitCell(refinedCell);
    TS_ASSERT_DELTA(cell.a(), 5.43111972, 1e-8);

    // Adding an unindexed peak should make the function return the initial
    peaks->addPeak(PoldiPeak::create(UncertainValue(1.0)));
    TS_ASSERT_THROWS_NOTHING(refinedCell = alg.getRefinedStartingCell("5 5 5 90 90 90", "Cubic", peaks));
    TS_ASSERT_EQUALS(refinedCell, "5 5 5 90 90 90");
  }

  void testGetCrystalSystemFromPointGroup() {
    TestablePoldiFitPeaks2D alg;

    auto pgCubic = PointGroupFactory::Instance().createPointGroup("m-3m");
    TS_ASSERT_EQUALS(alg.getLatticeSystemFromPointGroup(pgCubic), "Cubic");

    auto pgTetra = PointGroupFactory::Instance().createPointGroup("4/mmm");
    TS_ASSERT_EQUALS(alg.getLatticeSystemFromPointGroup(pgTetra), "Tetragonal");

    auto pgOrtho = PointGroupFactory::Instance().createPointGroup("mmm");
    TS_ASSERT_EQUALS(alg.getLatticeSystemFromPointGroup(pgOrtho), "Orthorhombic");

    auto pgMono = PointGroupFactory::Instance().createPointGroup("2/m");
    TS_ASSERT_EQUALS(alg.getLatticeSystemFromPointGroup(pgMono), "Monoclinic");

    auto pgTric = PointGroupFactory::Instance().createPointGroup("-1");
    TS_ASSERT_EQUALS(alg.getLatticeSystemFromPointGroup(pgTric), "Triclinic");

    auto pgHex = PointGroupFactory::Instance().createPointGroup("6/mmm");
    TS_ASSERT_EQUALS(alg.getLatticeSystemFromPointGroup(pgHex), "Hexagonal");

    auto pgTrigRh = PointGroupFactory::Instance().createPointGroup("-3m r");
    TS_ASSERT_EQUALS(alg.getLatticeSystemFromPointGroup(pgTrigRh), "Rhombohedral");

    auto pgTrigHex = PointGroupFactory::Instance().createPointGroup("-3m");
    TS_ASSERT_EQUALS(alg.getLatticeSystemFromPointGroup(pgTrigHex), "Hexagonal");

    PointGroup_sptr invalid;
    TS_ASSERT_THROWS(alg.getLatticeSystemFromPointGroup(invalid), const std::invalid_argument &);
  }

private:
  PoldiInstrumentAdapter_sptr m_instrument;
  PoldiTimeTransformer_sptr m_timeTransformer;

  void compareIntensities(const PoldiPeakCollection_sptr &first, const PoldiPeakCollection_sptr &second,
                          double relativePrecision) {
    for (size_t i = 0; i < first->peakCount(); ++i) {
      PoldiPeak_sptr peak = first->peak(i);
      PoldiPeak_sptr referencePeak = second->peak(i);

      std::stringstream strm;
      strm << std::setprecision(15);
      strm << "Error in Peak " << i << ": " << peak->intensity().value()
           << " != " << referencePeak->intensity().value();

      TSM_ASSERT_DELTA(strm.str().c_str(), fabs(1.0 - peak->intensity().value() / referencePeak->intensity().value()),
                       0.0, relativePrecision);
    }
  }

  class TestablePoldiFitPeaks2D : public PoldiFitPeaks2D {
    friend class PoldiFitPeaks2DTest;

  public:
    TestablePoldiFitPeaks2D() : PoldiFitPeaks2D() {}
    ~TestablePoldiFitPeaks2D() override = default;
  };
};
