#ifndef MANTID_SINQ_POLDIPEAKSEARCH_H_
#define MANTID_SINQ_POLDIPEAKSEARCH_H_

#include "MantidSINQ/DllConfig.h"

#include "MantidKernel/System.h"
#include "MantidKernel/V2D.h"
#include "MantidAPI/Algorithm.h"

#include "MantidDataObjects/Workspace2D.h"

#include "MantidSINQ/PoldiUtilities/PoldiPeak.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeakCollection.h"
namespace Mantid
{
namespace Poldi
{
  /** PoldiPeakSearch :
    
    This Algorithm performs a peak search in POLDI auto-correlation data. It's a direct
    translation from the original POLDI analysis software.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 28/02/2014

    Copyright © 2014 PSI-MSS

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */

  using namespace Kernel;
  using namespace DataObjects;

  class MANTID_SINQ_DLL PoldiPeakSearch : public API::Algorithm
  {
  public:
    PoldiPeakSearch();
    virtual ~PoldiPeakSearch() {}

    virtual int version() const { return 1; }
    virtual const std::string name() const { return "PoldiPeakSearch"; }
    virtual const std::string category() const { return "SINQ\\Poldi"; }
    
  protected:
    void initDocs();

    MantidVec getNeighborSums(MantidVec correlationCounts);

    std::list<MantidVec::iterator> findPeaks(MantidVec::iterator begin, MantidVec::iterator end);
    std::list<MantidVec::iterator> findPeaksRecursive(MantidVec::iterator begin, MantidVec::iterator end);
    MantidVec::iterator getLeftRangeBegin(MantidVec::iterator begin);
    MantidVec::iterator getRightRangeEnd(MantidVec::iterator end);

    std::list<MantidVec::iterator> mapPeakPositionsToCorrelationData(std::list<MantidVec::iterator> peakPositions, MantidVec::iterator baseDataStart, MantidVec::iterator originalDataStart);

    std::pair<double, double> getBackgroundWithSigma(std::list<MantidVec::iterator> peakPositions, MantidVec &correlationCounts);
    bool distanceToPeaksGreaterThanMinimum(std::list<MantidVec::iterator> peakPositions, MantidVec::iterator point);
    size_t getNumberOfBackgroundPoints(std::list<MantidVec::iterator> peakPositions, MantidVec &correlationCounts);

    double minimumPeakHeightFromBackground(std::pair<double, double> backgroundWithSigma);
    std::vector<PoldiPeak_sptr> getPeaks(MantidVec::iterator baseListStart, std::list<MantidVec::iterator> peakPositions, MantidVec xData);

    void setErrorsOnWorkspace(Workspace2D_sptr correlationWorkspace, double error);

    void setMinimumDistance(int newMinimumDistance);
    void setMinimumPeakHeight(double newMinimumPeakHeight);
    void setMaximumPeakNumber(int newMaximumPeakNumber);

    void setRecursionAbsoluteBorders(MantidVec::iterator begin, MantidVec::iterator end);

    static bool vectorElementGreaterThan(MantidVec::iterator first, MantidVec::iterator second);
    static bool intensityGreaterThan(PoldiPeak_sptr first, PoldiPeak_sptr second);
    bool isLessThanMinimum(PoldiPeak_sptr peak);

    int m_minimumDistance;
    int m_doubleMinimumDistance;
    double m_minimumPeakHeight;
    int m_maximumPeakNumber;

    MantidVec::iterator m_recursionAbsoluteBegin;
    MantidVec::iterator m_recursionAbsoluteEnd;

    PoldiPeakCollection_sptr m_peaks;

  private:
    void init();
    void exec();
  };


} // namespace Poldi
} // namespace Mantid

#endif  /* MANTID_SINQ_POLDIPEAKSEARCH_H_ */
