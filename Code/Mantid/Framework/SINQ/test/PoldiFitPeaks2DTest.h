#ifndef MANTID_SINQ_POLDICALCULATESPECTRUM2DTEST_H_
#define MANTID_SINQ_POLDICALCULATESPECTRUM2DTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"

#include "MantidSINQ/PoldiFitPeaks2D.h"
#include "MantidSINQ/PoldiUtilities/PoldiSpectrumDomainFunction.h"
#include "MantidSINQ/PoldiUtilities/PoldiMockInstrumentHelpers.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::Poldi;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class PoldiFitPeaks2DTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static PoldiFitPeaks2DTest *createSuite() { return new PoldiFitPeaks2DTest(); }
    static void destroySuite( PoldiFitPeaks2DTest *suite ) { delete suite; }

    PoldiFitPeaks2DTest()
    {
        FrameworkManager::Instance();

        m_instrument = PoldiInstrumentAdapter_sptr(new FakePoldiInstrumentAdapter);
        m_timeTransformer = PoldiTimeTransformer_sptr(new PoldiTimeTransformer(m_instrument));
    }

    void testSetTimeTransformer()
    {
        TestablePoldiFitPeaks2D spectrumCalculator;
        spectrumCalculator.setTimeTransformer(m_timeTransformer);

        TS_ASSERT_EQUALS(spectrumCalculator.m_timeTransformer, m_timeTransformer);
    }

    void testSetTimeTransformerFromInstrument()
    {
        TestablePoldiFitPeaks2D spectrumCalculator;
        spectrumCalculator.setTimeTransformerFromInstrument(m_instrument);

        TS_ASSERT(spectrumCalculator.m_timeTransformer);
    }

    void testSetDeltaT()
    {
        TestablePoldiFitPeaks2D spectrumCalculator;
        TS_ASSERT_THROWS_NOTHING(spectrumCalculator.setDeltaT(2.0));
        TS_ASSERT_EQUALS(spectrumCalculator.m_deltaT, 2.0);

        TS_ASSERT_THROWS(spectrumCalculator.setDeltaT(0.0), std::invalid_argument);
        TS_ASSERT_THROWS(spectrumCalculator.setDeltaT(-1.0), std::invalid_argument);
    }

    void testSetDeltaTFromWorkspace()
    {
        MatrixWorkspace_sptr ws = WorkspaceCreationHelper::Create2DWorkspace(1, 10);
        for(size_t i = 0; i <= 10; ++i) {
            ws->dataX(0)[i] = static_cast<double>(i);
        }

        TestablePoldiFitPeaks2D spectrumCalculator;
        spectrumCalculator.setDeltaTFromWorkspace(ws);
        TS_ASSERT_EQUALS(spectrumCalculator.m_deltaT, 1.0);

        MatrixWorkspace_sptr invalidWs = WorkspaceCreationHelper::Create2DWorkspace123(1, 1);
        TS_ASSERT_THROWS(spectrumCalculator.setDeltaTFromWorkspace(invalidWs), std::invalid_argument);
    }

    void testIsValidDeltaT()
    {
        TestablePoldiFitPeaks2D spectrumCalculator;
        TS_ASSERT_EQUALS(spectrumCalculator.isValidDeltaT(1.0), true);
        TS_ASSERT_EQUALS(spectrumCalculator.isValidDeltaT(0.0), false);
        TS_ASSERT_EQUALS(spectrumCalculator.isValidDeltaT(-1.0), false);
    }

    void testGetPeakCollection()
    {
        TestablePoldiFitPeaks2D spectrumCalculator;

        TableWorkspace_sptr peakTable = PoldiPeakCollectionHelpers::createPoldiPeakTableWorkspace();
        TS_ASSERT_THROWS_NOTHING(spectrumCalculator.getPeakCollection(peakTable));

        PoldiPeakCollection_sptr collection = spectrumCalculator.getPeakCollection(peakTable);
        TS_ASSERT_EQUALS(collection->peakCount(), peakTable->rowCount());
    }

    void testGetIntegratedPeakCollection()
    {
        PoldiPeakCollection_sptr testPeaks = PoldiPeakCollectionHelpers::createPoldiPeakCollectionMaximum();

        TestablePoldiFitPeaks2D spectrumCalculator;
        spectrumCalculator.initialize();
        // deltaT is not set, so this must fail
        TS_ASSERT_THROWS(spectrumCalculator.getIntegratedPeakCollection(testPeaks), std::invalid_argument);
        spectrumCalculator.setDeltaT(3.0);

        // still fails, because time transformer is required.
        TS_ASSERT_THROWS(spectrumCalculator.getIntegratedPeakCollection(testPeaks), std::invalid_argument);
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
        for(size_t i = 0; i < testPeaks->peakCount(); ++i) {
            PoldiPeak_sptr peak = testPeaks->peak(i)->clone();

            double oldIntensity = peak->intensity();
            double fwhm = peak->fwhm(PoldiPeak::AbsoluteD);

            // Integral of gaussian is height * sigma * sqrt(2 * pi)
            peak->setIntensity(UncertainValue(oldIntensity * fwhm / (2.0 * sqrt(2.0 * log(2.0))) * sqrt(2.0 * M_PI)));
            integratedReference->addPeak(peak);
        }

        // compare result to relative error of 1e-6
        compareIntensities(integratedTestPeaks, integratedReference, 1e-6);

        // In case of integrated peaks nothing should happen
        PoldiPeakCollection_sptr alreadyIntegratedPeaks(new PoldiPeakCollection(PoldiPeakCollection::Integral));
        alreadyIntegratedPeaks->addPeak(PoldiPeak::create(2.0));

        PoldiPeakCollection_sptr alreadyIntegratedResult = spectrumCalculator.getIntegratedPeakCollection(alreadyIntegratedPeaks);
        TS_ASSERT(alreadyIntegratedResult != alreadyIntegratedPeaks);
        TS_ASSERT_EQUALS(alreadyIntegratedResult->peakCount(), alreadyIntegratedPeaks->peakCount());
        TS_ASSERT_EQUALS(alreadyIntegratedResult->peak(0)->d(), alreadyIntegratedPeaks->peak(0)->d());

        // Where there's no profile function in the peak collection, falling back to the property PeakProfileFunction.
        // Default is Gaussian, so this is supposed to work.
        PoldiPeakCollection_sptr noProfilePeaks(new PoldiPeakCollection);
        TS_ASSERT_THROWS_NOTHING(spectrumCalculator.getIntegratedPeakCollection(noProfilePeaks));

        // While setting an invalid function name throws.
        spectrumCalculator.setProperty("PeakProfileFunction", "InvalidFunctionName");
        TS_ASSERT_THROWS(spectrumCalculator.getIntegratedPeakCollection(noProfilePeaks), std::runtime_error);

        // When there is no valid PoldiPeakCollection, the method also throws
        PoldiPeakCollection_sptr invalidPeakCollection;
        TS_ASSERT_THROWS(spectrumCalculator.getIntegratedPeakCollection(invalidPeakCollection), std::invalid_argument);
    }

    void testGetNormalizedPeakCollection()
    {
        TestablePoldiFitPeaks2D spectrumCalculator;

        // first, test the failing cases
        PoldiPeakCollection_sptr invalidPeakCollection;
        TS_ASSERT_THROWS(spectrumCalculator.getNormalizedPeakCollection(invalidPeakCollection), std::invalid_argument);

        // m_timeTransformer has not been assigned, so even a "good" PeakCollection will throw
        PoldiPeakCollection_sptr testPeaks = PoldiPeakCollectionHelpers::createPoldiPeakCollectionMaximum();
        TS_ASSERT_THROWS(spectrumCalculator.getNormalizedPeakCollection(testPeaks), std::invalid_argument);

        spectrumCalculator.setTimeTransformer(m_timeTransformer);

        // to verify the results, use actual results from after integration step
        PoldiPeakCollection_sptr integratedPeaks = PoldiPeakCollectionHelpers::createPoldiPeakCollectionIntegral();
        TS_ASSERT_THROWS_NOTHING(spectrumCalculator.getNormalizedPeakCollection(integratedPeaks));

        PoldiPeakCollection_sptr normalizedPeaks = spectrumCalculator.getNormalizedPeakCollection(integratedPeaks);
        PoldiPeakCollection_sptr normalizedReferencePeaks = PoldiPeakCollectionHelpers::createPoldiPeakCollectionNormalized();

        compareIntensities(normalizedPeaks, normalizedReferencePeaks, 1.5e-6);
    }

    void testGetCountPeakCollection()
    {
        TestablePoldiFitPeaks2D spectrumCalculator;

        // first, test the failing cases
        PoldiPeakCollection_sptr invalidPeakCollection;
        TS_ASSERT_THROWS(spectrumCalculator.getCountPeakCollection(invalidPeakCollection), std::invalid_argument);

        // m_timeTransformer has not been assigned, so even a "good" PeakCollection will throw
        PoldiPeakCollection_sptr testPeaks = PoldiPeakCollectionHelpers::createPoldiPeakCollectionNormalized();
        TS_ASSERT_THROWS(spectrumCalculator.getCountPeakCollection(testPeaks), std::invalid_argument);

        spectrumCalculator.setTimeTransformer(m_timeTransformer);

        // to verify the results, use actual results from after integration step
        PoldiPeakCollection_sptr normalizedPeaks = PoldiPeakCollectionHelpers::createPoldiPeakCollectionNormalized();
        TS_ASSERT_THROWS_NOTHING(spectrumCalculator.getCountPeakCollection(normalizedPeaks));

        PoldiPeakCollection_sptr integratedPeaks = spectrumCalculator.getCountPeakCollection(normalizedPeaks);
        PoldiPeakCollection_sptr integratedReferencePeaks = PoldiPeakCollectionHelpers::createPoldiPeakCollectionIntegral();

        compareIntensities(integratedPeaks, integratedReferencePeaks, 1.5e-6);

    }

    void testGetFunctionFromPeakCollection()
    {
        TestablePoldiFitPeaks2D spectrumCalculator;
        spectrumCalculator.initialize();

        PoldiPeakCollection_sptr peaks = PoldiPeakCollectionHelpers::createPoldiPeakCollectionNormalized();

        boost::shared_ptr<Poldi2DFunction> poldi2DFunction = spectrumCalculator.getFunctionFromPeakCollection(peaks);

        TS_ASSERT_EQUALS(poldi2DFunction->nFunctions(), peaks->peakCount());

        for(size_t i = 0; i < poldi2DFunction->nFunctions(); ++i) {
            IFunction_sptr rawFunction = poldi2DFunction->getFunction(i);
            boost::shared_ptr<PoldiSpectrumDomainFunction> poldiFunction =
                boost::dynamic_pointer_cast<PoldiSpectrumDomainFunction>(rawFunction);

            TS_ASSERT(poldiFunction);

            IPeakFunction_sptr wrappedFunction = poldiFunction->getProfileFunction();

            TS_ASSERT_DELTA(wrappedFunction->intensity(), static_cast<double>(peaks->peak(i)->intensity()), 1e-10);
        }
    }

    void testGetPeakCollectionFromFunction()
    {
        TestablePoldiFitPeaks2D spectrumCalculator;
        spectrumCalculator.initialize();

        PoldiPeakCollection_sptr peaks = PoldiPeakCollectionHelpers::createPoldiPeakCollectionNormalized();
        IFunction_sptr poldi2DFunction = spectrumCalculator.getFunctionFromPeakCollection(peaks);

        PoldiPeakCollection_sptr peaksFromFunction = spectrumCalculator.getPeakCollectionFromFunction(poldi2DFunction);

        TS_ASSERT_EQUALS(peaksFromFunction->peakCount(), peaks->peakCount());
        for(size_t i = 0; i < peaksFromFunction->peakCount(); ++i) {
            PoldiPeak_sptr functionPeak = peaksFromFunction->peak(i);
            PoldiPeak_sptr referencePeak = peaks->peak(i);

            TS_ASSERT_EQUALS(functionPeak->d(), referencePeak->d());
            TS_ASSERT_EQUALS(functionPeak->fwhm(), referencePeak->fwhm());
        }
    }

    void testAssignMillerIndices()
    {
        PoldiPeak_sptr peak1 = PoldiPeak::create(MillerIndices(1, 1, 1), 2.0);
        PoldiPeakCollection_sptr from = boost::make_shared<PoldiPeakCollection>();
        from->addPeak(peak1);

        PoldiPeak_sptr peak2 = PoldiPeak::create(Conversions::dToQ(2.0));
        PoldiPeakCollection_sptr to = boost::make_shared<PoldiPeakCollection>();
        to->addPeak(peak2);

        PoldiPeakCollection_sptr invalid;

        TestablePoldiFitPeaks2D spectrumCalculator;

        TS_ASSERT_THROWS(spectrumCalculator.assignMillerIndices(from, invalid), std::invalid_argument);
        TS_ASSERT_THROWS(spectrumCalculator.assignMillerIndices(invalid, from), std::invalid_argument);
        TS_ASSERT_THROWS(spectrumCalculator.assignMillerIndices(invalid, invalid), std::invalid_argument);

        TS_ASSERT_DIFFERS(peak1->hkl(), peak2->hkl());

        TS_ASSERT_THROWS_NOTHING(spectrumCalculator.assignMillerIndices(from, to));
        TS_ASSERT_EQUALS(peak1->hkl(), peak2->hkl());

        to->addPeak(peak1);

        TS_ASSERT_THROWS(spectrumCalculator.assignMillerIndices(from, to), std::runtime_error);
    }

    void testAddBackgroundFunctions()
    {
        TestablePoldiFitPeaks2D spectrumCalculator;
        spectrumCalculator.initialize();

        boost::shared_ptr<Poldi2DFunction> funDefault(new Poldi2DFunction);
        TS_ASSERT_EQUALS(funDefault->nParams(), 0);
        TS_ASSERT_EQUALS(funDefault->nFunctions(), 0);

        spectrumCalculator.addBackgroundTerms(funDefault);
        TS_ASSERT_EQUALS(funDefault->nParams(), 2);
        TS_ASSERT_EQUALS(funDefault->nFunctions(), 2);


        boost::shared_ptr<Poldi2DFunction> funLinear(new Poldi2DFunction);
        spectrumCalculator.setProperty("FitConstantBackground", false);
        spectrumCalculator.addBackgroundTerms(funLinear);

        // Now there's only the linear term
        TS_ASSERT_EQUALS(funLinear->nParams(), 1);
        TS_ASSERT_EQUALS(funLinear->parameterName(0), "f0.A1");
        TS_ASSERT_EQUALS(funLinear->nFunctions(), 1);

        boost::shared_ptr<Poldi2DFunction> funConstant(new Poldi2DFunction);
        spectrumCalculator.setProperty("FitConstantBackground", true);
        spectrumCalculator.setProperty("FitLinearBackground", false);
        spectrumCalculator.addBackgroundTerms(funConstant);

        // Now there's only the constant term
        TS_ASSERT_EQUALS(funConstant->nParams(), 1);
        TS_ASSERT_EQUALS(funConstant->parameterName(0), "f0.A0");
        TS_ASSERT_EQUALS(funConstant->nFunctions(), 1);
    }

private:
    PoldiInstrumentAdapter_sptr m_instrument;
    PoldiTimeTransformer_sptr m_timeTransformer;

    void compareIntensities(PoldiPeakCollection_sptr first, PoldiPeakCollection_sptr second, double relativePrecision)
    {
        for(size_t i = 0; i < first->peakCount(); ++i) {
            PoldiPeak_sptr peak = first->peak(i);
            PoldiPeak_sptr referencePeak = second->peak(i);

            std::stringstream strm;
            strm << std::setprecision(15);
            strm << "Error in Peak " << i << ": " << peak->intensity().value() << " != " << referencePeak->intensity().value();

            TSM_ASSERT_DELTA(strm.str().c_str(), fabs(1.0 - peak->intensity().value()/referencePeak->intensity().value()), 0.0, relativePrecision);
        }
    }

    class TestablePoldiFitPeaks2D : public PoldiFitPeaks2D
    {
        friend class PoldiFitPeaks2DTest;
    public:
        TestablePoldiFitPeaks2D() : PoldiFitPeaks2D() { }
        ~TestablePoldiFitPeaks2D() { }
    };

};

#endif // MANTID_SINQ_POLDICALCULATESPECTRUM2DTEST_H_
