// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidSINQ/PoldiPeakSummary.h"

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidSINQ/PoldiUtilities/MillerIndicesIO.h"
#include "MantidSINQ/PoldiUtilities/PoldiPeakCollection.h"
#include "MantidSINQ/PoldiUtilities/UncertainValueIO.h"

namespace Mantid::Poldi {

using namespace API;
using namespace DataObjects;
using namespace Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PoldiPeakSummary)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string PoldiPeakSummary::name() const { return "PoldiPeakSummary"; }

/// Algorithm's version for identification. @see Algorithm::version
int PoldiPeakSummary::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string PoldiPeakSummary::category() const { return "SINQ\\Poldi"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string PoldiPeakSummary::summary() const {
  return "The algorithm takes a table with peaks from POLDI analysis "
         "algorithms and creates a summary table.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void PoldiPeakSummary::init() {
  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>("InputWorkspace", "", Direction::Input),
                  "Input workspace containing a table with peaks from a POLDI "
                  "fit routine.");
  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "Output table workspace that contains ");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void PoldiPeakSummary::exec() {
  TableWorkspace_sptr poldiPeakTableWorkspace = getProperty("InputWorkspace");
  PoldiPeakCollection_sptr peaks = std::make_shared<PoldiPeakCollection>(poldiPeakTableWorkspace);

  TableWorkspace_sptr resultTable = getSummaryTable(peaks);

  setProperty("OutputWorkspace", resultTable);
}

TableWorkspace_sptr PoldiPeakSummary::getSummaryTable(const PoldiPeakCollection_sptr &peakCollection) const {
  if (!peakCollection) {
    throw std::invalid_argument("Cannot create summary of a null PoldiPeakCollection.");
  }

  TableWorkspace_sptr peakResultWorkspace = getInitializedResultWorkspace();

  for (size_t i = 0; i < peakCollection->peakCount(); ++i) {
    storePeakSummary(peakResultWorkspace->appendRow(), peakCollection->peak(i));
  }

  return peakResultWorkspace;
}

TableWorkspace_sptr PoldiPeakSummary::getInitializedResultWorkspace() const {
  TableWorkspace_sptr peakResultWorkspace =
      std::dynamic_pointer_cast<TableWorkspace>(WorkspaceFactory::Instance().createTable());

  peakResultWorkspace->addColumn("str", "hkl");
  peakResultWorkspace->addColumn("str", "Q");
  peakResultWorkspace->addColumn("str", "d");
  peakResultWorkspace->addColumn("double", "deltaD/d *10^3");
  peakResultWorkspace->addColumn("str", "FWHM rel. *10^3");
  peakResultWorkspace->addColumn("str", "Intensity");

  return peakResultWorkspace;
}

void PoldiPeakSummary::storePeakSummary(TableRow tableRow, const PoldiPeak_sptr &peak) const {
  UncertainValue q = peak->q();
  UncertainValue d = peak->d();

  tableRow << MillerIndicesIO::toString(peak->hkl()) << UncertainValueIO::toString(q) << UncertainValueIO::toString(d)
           << d.error() / d.value() * 1e3 << UncertainValueIO::toString(peak->fwhm(PoldiPeak::Relative) * 1e3)
           << UncertainValueIO::toString(peak->intensity());
}

} // namespace Mantid::Poldi
