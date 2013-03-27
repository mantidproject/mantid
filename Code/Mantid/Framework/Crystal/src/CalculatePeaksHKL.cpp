/*WIKI*
Uses the UB matrix on the sample to calculate the Miller indices for all peaks in the peaks workspace.
Unlike [[IndexPeaks]] this algorithm does not perform any mandatory optimization. This algorithm does not
round the Miller indices to the nearest integer.

== Alternatives ==
[[IndexPeaks]]
*WIKI*/

#include "MantidCrystal/CalculatePeaksHKL.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include <set>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid
{
  namespace Crystal
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(CalculatePeaksHKL)

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    CalculatePeaksHKL::CalculatePeaksHKL()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    CalculatePeaksHKL::~CalculatePeaksHKL()
    {
    }

    //----------------------------------------------------------------------------------------------
    /// Algorithm's name for identification. @see Algorithm::name
    const std::string CalculatePeaksHKL::name() const
    {
      return "CalculatePeaksHKL";
    }
    ;

    /// Algorithm's version for identification. @see Algorithm::version
    int CalculatePeaksHKL::version() const
    {
      return 1;
    }
    ;

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string CalculatePeaksHKL::category() const
    {
      return "Crystal";
    }

    //----------------------------------------------------------------------------------------------
    /// Sets documentation strings for this algorithm
    void CalculatePeaksHKL::initDocs()
    {
      this->setWikiSummary("Calculates Miller indices for each peak. No rounding or UB optimization.");
      this->setOptionalMessage(
          "Calculates Miller indices for each peak. No rounding or UB optimization.");
    }

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
     */
    void CalculatePeaksHKL::init()
    {
      this->declareProperty(
          new WorkspaceProperty<PeaksWorkspace>("PeaksWorkspace", "", Direction::InOut),
          "Input Peaks Workspace");

      this->declareProperty("OverWrite", false,
          "Overwrite existing miller indices as well as empty ones.");

      this->declareProperty(new PropertyWithValue<int>("NumIndexed", 0, Direction::Output),
          "Gets set with the number of indexed peaks.");
    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
     */
    void CalculatePeaksHKL::exec()
    {
      Mantid::DataObjects::PeaksWorkspace_sptr ws = getProperty("PeaksWorkspace");
      const bool overwrite = getProperty("OverWrite");
      const int n_peaks = ws->getNumberPeaks();

      OrientedLattice o_lattice = ws->mutableSample().getOrientedLattice();
      Matrix<double> UB = o_lattice.getUB();

      DblMatrix UB_inverse(UB);

      if (IndexingUtils::CheckUB(UB))
      {
        UB_inverse.Invert();
      }
      else
      {
        throw std::runtime_error("The UB is not valid");
      }

      int peaksIndexed = 0;
      for (int i = 0; i < n_peaks; i++)
      {
        Peak& peak = ws->getPeak(i);
        if (overwrite || (peak.getHKL().nullVector()))
        {
          V3D qlab = peak.getQSampleFrame();
          V3D hkl = UB_inverse * qlab / (2.0 * M_PI);
          peak.setHKL(hkl);
          ++peaksIndexed;
        }
      }
      setProperty("NumIndexed", peaksIndexed);
    }

  } // namespace Crystal
} // namespace Mantid
