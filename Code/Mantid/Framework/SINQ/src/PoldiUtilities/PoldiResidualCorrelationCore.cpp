#include "MantidSINQ/PoldiUtilities/PoldiResidualCorrelationCore.h"

namespace Mantid
{
namespace Poldi
{

PoldiResidualCorrelationCore::PoldiResidualCorrelationCore(Kernel::Logger &g_log) :
    PoldiAutoCorrelationCore(g_log)
{
}

/// Returns norm counts (with an added weight).
double PoldiResidualCorrelationCore::getNormCounts(int x, int y) const
{
    return fabs(m_normCountData->readY(x)[y]) + 0.1;
}

/// Calculates a scaled and weighted average signal/noise value from the supplied list.
double PoldiResidualCorrelationCore::reduceChopperSlitList(const std::vector<Mantid::Poldi::UncertainValue> &valuesWithSigma, double weight) const
{
    std::vector<double> signalToNoise(valuesWithSigma.size());
    std::transform(valuesWithSigma.begin(), valuesWithSigma.end(), signalToNoise.begin(), &UncertainValue::valueToErrorRatio);

    double numberOfElements = static_cast<double>(valuesWithSigma.size());
    double average = std::accumulate(signalToNoise.begin(), signalToNoise.end(), 0.0) / numberOfElements;

    std::vector<double> deviationFromAverage(signalToNoise.size());
    for(size_t i = 0; i < signalToNoise.size(); ++i) {
        deviationFromAverage[i] = fabs(signalToNoise[i] - average);
    }

    double averageDeviation = std::accumulate(deviationFromAverage.begin(), deviationFromAverage.end(), 0.0) / numberOfElements;

    double absoluteAverage = fabs(average);
    return average * absoluteAverage/(averageDeviation + absoluteAverage) * numberOfElements * weight;
}

/// Background is the sum of correlation counts, sum of counts is discarded.
double PoldiResidualCorrelationCore::calculateCorrelationBackground(double sumOfCorrelationCounts, double sumOfCounts) const
{
    UNUSED_ARG(sumOfCounts);

    return sumOfCorrelationCounts;
}

/**
 * Distributes correlation counts into count data and corrects correlation spectrum
 *
 * This method does three things: First it distributes the intensity of the correlation spectrum
 * for a given d-value over all places in the detector where it may belong. After that it sums
 * the new residuals and distributes them equally over all points of the 2D data.
 *
 * After a new summation of those corrected residuals, the correlation spectrum is corrected
 * accordingly.
 *
 * Please note that this method modifies the stored count data.
 *
 * @param correctedCorrelatedIntensities :: Corrected correlation spectrum of the residuals.
 * @param dValues :: d-values in reverse order.
 * @return Final corrected correlation spectrum of residuals.
 */
DataObjects::Workspace2D_sptr PoldiResidualCorrelationCore::finalizeCalculation(const std::vector<double> &correctedCorrelatedIntensities, const std::vector<double> &dValues) const
{
    const std::vector<double> chopperSlits(m_chopper->slitTimes());

    PARALLEL_FOR_NO_WSP_CHECK()
    for(int k = 0; k < static_cast<int>(m_indices.size()); ++k) {
        for(size_t i = 0; i < dValues.size(); ++i) {
            double d = dValues[dValues.size() - i - 1];
            double dInt = correctedCorrelatedIntensities[i];
            double deltaForD = -dInt/m_weightsForD[i]/static_cast<double>(chopperSlits.size());

            for(auto offset = chopperSlits.begin(); offset != chopperSlits.end(); ++offset) {
                CountLocator locator = getCountLocator(d, *offset, m_indices[k]);

                int indexDifference = locator.icmax - locator.icmin;

                switch(indexDifference) {
                case 0:
                    addToCountData(locator.detectorElement, locator.iicmin, deltaForD * locator.arrivalWindowWidth);
                    break;
                case 2: {
                    int middleIndex = cleanIndex((locator.icmin + 1), m_timeBinCount);

                    addToCountData(locator.detectorElement, middleIndex, deltaForD);
                }
                case 1: {
                    addToCountData(locator.detectorElement, locator.iicmin, deltaForD * (static_cast<double>(locator.icmin) - locator.cmin + 1.0));
                    addToCountData(locator.detectorElement, locator.iicmax, deltaForD * (locator.cmax - static_cast<double>(locator.icmax)));
                    break;
                }
                default:
                    break;
                }
            }
        }
    }

    double sumOfResiduals = getSumOfCounts(m_timeBinCount, m_detectorElements);
    double numberOfCells = static_cast<double>(m_timeBinCount * (m_detectorElements.size()));
    double ratio = sumOfResiduals / numberOfCells;

    PARALLEL_FOR_NO_WSP_CHECK()
    for(int i = 0; i < static_cast<int>(m_indices.size()); ++i) {
        int element = getElementFromIndex(m_indices[i]);
        for(int j = 0; j < m_timeBinCount; ++j) {
            addToCountData(element, j, -ratio);
        }
    }

    sumOfResiduals = getSumOfCounts(m_timeBinCount, m_detectorElements);

    std::vector<double> newCorrected(correctedCorrelatedIntensities.size());
    for(size_t i = 0; i < correctedCorrelatedIntensities.size(); ++i) {
        newCorrected[i] = correctedCorrelatedIntensities[i] - (sumOfResiduals * m_weightsForD[i]/m_sumOfWeights);
    }

    return PoldiAutoCorrelationCore::finalizeCalculation(newCorrected, dValues);
}

/// Adds the supplied value to each data point.
void PoldiResidualCorrelationCore::addToCountData(int x, int y, double newCounts) const
{
    m_countData->dataY(x)[y] += newCounts;
}

} // namespace Poldi
} // namespace Mantid
