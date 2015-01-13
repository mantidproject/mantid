//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidSINQ/PoldiRemoveDeadWires.h"

#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"

#include <boost/shared_ptr.hpp>

using namespace std;

namespace Mantid {
namespace Poldi {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(PoldiRemoveDeadWires)

using namespace Kernel;
using namespace API;
using Geometry::Instrument;

/// Empty default constructor
PoldiRemoveDeadWires::PoldiRemoveDeadWires()
    : m_filename(), m_runDeadWires(true), m_runAutoDetectDW(true),
      m_defautDWThreshold(0.5), m_numberOfSpectra(0), m_channelsPerSpectrum(0) {
}

/// Initialisation method.
void PoldiRemoveDeadWires::init() {

  // Input workspace containing the data raw to treat.
  declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>(
                      "InputWorkspace", "", Direction::InOut),
                  "Input workspace containing the raw data to treat.");

  // The output Tableworkspace with columns containing key summary information
  // about the PoldiDeadWires.
  declareProperty(new WorkspaceProperty<ITableWorkspace>("PoldiDeadWires", "",
                                                         Direction::Output),
                  "The input Tableworkspace with columns containing key "
                  "summary information about the PoldiDeadWires.");

  // Should we remove the declare dead wires
  declareProperty("RemoveExcludedWires", true,
                  "Set to 0 the data value of all the excluded wires.");
  // Auto detect and remove the potential bas wires
  declareProperty("AutoRemoveBadWires", true,
                  "Auto detect and remove the potential bas wires");

  // defaut threshold used to detect dead wires
  double defautDWThreshold = 0.5;
  declareProperty(
      "BadWiresThreshold", defautDWThreshold,
      "Threshold for the auto-detection of the bad wires\n"
      "If the average value of a wire differs of more than 'threshold'\n"
      "of one of the next valid neighbor, it is removed.");

  std::string grp1 = "Data loading option";
  setPropertyGroup("RemoveExcludedWires", grp1);
  setPropertyGroup("AutoRemoveBadWires", grp1);
  setPropertyGroup("BadWiresThreshold", grp1);

  // output information about nb ox removed dead wires
  declareProperty("nbExcludedWires", 0, "nb of excluded wires",
                  Direction::Output);
  // output information about auto-detected dead wires
  declareProperty("nbAuteDeadWires", 0, "nb of auto-detect dead wires",
                  Direction::Output);
}

/** ***************************************************************** */

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw Exception::NotFoundError Error when saving the PoldiDeadWires Results
 *data to Workspace
 *  @throw std::runtime_error Error when saving the PoldiDeadWires Results data
 *to Workspace
 */
void PoldiRemoveDeadWires::exec() {

  ////////////////////////////////////////////////////////////////////////
  // About the workspace
  ////////////////////////////////////////////////////////////////////////

  DataObjects::Workspace2D_sptr localWorkspace =
      this->getProperty("InputWorkspace");

  this->m_channelsPerSpectrum = localWorkspace.get()->blocksize();
  this->m_numberOfSpectra =
      localWorkspace.get()->size() / m_channelsPerSpectrum;

  g_log.debug() << "_poldi : m_numberOfSpectra     = " << m_numberOfSpectra
                << std::endl;
  g_log.debug() << "_poldi : m_channelsPerSpectrum = " << m_channelsPerSpectrum
                << std::endl;

  ////////////////////////////////////////////////////////////////////////
  // Load the data into the workspace
  ////////////////////////////////////////////////////////////////////////

  // create table workspace
  try {
    ITableWorkspace_sptr outputws = WorkspaceFactory::Instance().createTable();

    // remove the dead-declared wires
    bool doRemoveExcludedWires = getProperty("RemoveExcludedWires");
    if (doRemoveExcludedWires) {
      runExcludWires3(localWorkspace, outputws);
    }

    // remove the auto-detected dead wires
    bool doAutoRemoveBadWires = getProperty("AutoRemoveBadWires");
    if (doAutoRemoveBadWires) {
      autoRemoveDeadWires(localWorkspace, outputws);
    }

    setProperty("PoldiDeadWires", outputws);

  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    throw std::runtime_error(
        "Error when saving the PoldiDeadWires Results data to Workspace");
  } catch (std::runtime_error &) {
    throw std::runtime_error(
        "Error when saving the PoldiDeadWires Results data to Workspace");
  }
}

/**
   Read from the instrument file the dead wires and store the information in a
   TableWorkspace.
   If asked, the dead wires are removed from the data set.

   @param localWorkspace :: input raw data workspace, containing the information
   about the instrument
   @param outputws :: input dead wire liste workspace
  */
void PoldiRemoveDeadWires::runExcludWires3(
    DataObjects::Workspace2D_sptr &localWorkspace,
    API::ITableWorkspace_sptr &outputws) {
  outputws->addColumn("int", "DeadWires");

  boost::shared_ptr<const Mantid::Geometry::IComponent> comp =
      localWorkspace->getInstrument()->getComponentByName("holder");
  boost::shared_ptr<const Mantid::Geometry::ICompAssembly> bank =
      boost::dynamic_pointer_cast<const Mantid::Geometry::ICompAssembly>(comp);
  if (bank) {
    // Get a vector of children (recursively)
    std::vector<boost::shared_ptr<const Mantid::Geometry::IComponent>> children;
    bank->getChildren(children, true);

    std::vector<double> defaultDeadWires;
    int ewLine = 0;

    for (unsigned int it = 0; it < children.size(); ++it) {
      string wireName = children.at(it)->getName();
      std::vector<boost::shared_ptr<const Mantid::Geometry::IComponent>> tyty =
          localWorkspace.get()->getInstrument().get()->getAllComponentsWithName(
              wireName);

      std::vector<double> tempWire = tyty[0]->getNumberParameter("excluded");
      if (tempWire.size() > 0) {
        int val = (int)tempWire[0];
        g_log.debug() << "_poldi : dead wires :" << val << std::endl;
        defaultDeadWires.push_back(val);
        for (unsigned int j = 0; j < m_channelsPerSpectrum; j++) {
          localWorkspace->maskBin(val - 1, j, 1);
        }
        ewLine++;

        TableRow t = outputws->appendRow();
        t << val;
      }
    }
    g_log.information() << "_poldi : dead wires set to 0 (nb:" << ewLine << ")"
                        << std::endl;
    setProperty("nbExcludedWires", ewLine);
  } else {
    g_log.information() << "_poldi : no dead wire removed" << std::endl;
  }
}

/**
   Auto detecte the dead wires and store the information in the TableWorkspace.
   If asked, the dead wires are removed from the data set.

   @param localWorkspace :: input raw data workspace, containing the information
   about the instrument
   @param outputws :: input dead wire liste workspace
  */
void PoldiRemoveDeadWires::autoRemoveDeadWires(
    DataObjects::Workspace2D_sptr &localWorkspace,
    API::ITableWorkspace_sptr &outputws) {

  double autoDeadWiresThreshold = 0;
  autoDeadWiresThreshold = getProperty("BadWiresThreshold");
  if (!autoDeadWiresThreshold)
    autoDeadWiresThreshold = m_defautDWThreshold;
  autoDeadWiresThreshold = 1. - autoDeadWiresThreshold;
  //	double autoDeadWiresThreshold = 1-0.4;
  g_log.information() << "_poldi : auto removed wires : BadWiresThreshold:"
                      << autoDeadWiresThreshold << std::endl;
  int count = 0;

  double minValue = INFINITY;
  unsigned int minPos = 0;

  bool checkContinue = true;

  std::vector<double> average(this->m_numberOfSpectra);
  double globalAverage = 0;

  // compute the average intensity per spectrum
  for (unsigned int i = 0; i < this->m_numberOfSpectra; i++) {
    if (!localWorkspace.get()->hasMaskedBins(i)) {
      average.at(i) = 0;
      MantidVec &tempY = localWorkspace.get()->dataY(i);
      for (unsigned int j = 0; j < this->m_channelsPerSpectrum; j++) {
        average.at(i) += tempY[j];
      }
      average.at(i) /= static_cast<double>(this->m_channelsPerSpectrum);
      if (average[i] < minValue) {
        minValue = average[i];
        minPos = i;
      }
    }
  }

  g_log.debug() << "_poldi : auto removed wires : average done" << std::endl;

  while (checkContinue) {
    checkContinue = false;
    minValue = INFINITY;
    minPos = 0;
    int n = 0;
    // find the minimum average position, the most probably wrong spectra
    for (unsigned int i = 0; i < this->m_numberOfSpectra; i++) {
      if (!localWorkspace.get()->hasMaskedBins(i)) {
        globalAverage += average[i];
        n++;
        if (average[i] < minValue) {
          minValue = average[i];
          minPos = i;
        }
      }
    }
    globalAverage /= n;

    // applied the threshold to determine if a wires should be excluded
    // check if the wire is not already excluded
    if (!localWorkspace.get()->hasMaskedBins(minPos)) {

      if (average[minPos] < globalAverage * autoDeadWiresThreshold) {
        // mask the wires
        for (unsigned int j = 0; j < this->m_channelsPerSpectrum; j++) {
          localWorkspace->maskBin(minPos, j, 1);
        }
        count++;
        checkContinue = true;
        TableRow t = outputws->appendRow();
        t << int(minPos);
      }
    }

    // applied the threshold to determine if a wires should be excluded
    // check if the wire is not already excluded
    if (!localWorkspace.get()->hasMaskedBins(minPos)) {
      // check the threshold on the left
      unsigned int left = minPos - 1;
      // find the first used wires on the left
      while (localWorkspace.get()->hasMaskedBins(left) && left > 0) {
        left--;
      }
      if (left > 0 &&
          average[minPos] < average[left] * autoDeadWiresThreshold) {
        // mask the wires
        for (unsigned int j = 0; j < this->m_channelsPerSpectrum; j++) {
          localWorkspace->maskBin(minPos, j, 1);
        }
        count++;
        checkContinue = true;
        TableRow t = outputws->appendRow();
        t << int(minPos);
      }
    }

    if (!localWorkspace.get()->hasMaskedBins(minPos)) {
      // check the threshold on the right
      unsigned int right = minPos + 1;
      // find the first used wires on the left
      while (localWorkspace.get()->hasMaskedBins(right) &&
             right < this->m_numberOfSpectra) {
        right++;
      }
      if (right < m_numberOfSpectra - 1 &&
          average[minPos] < average[right] * autoDeadWiresThreshold) {
        // mask the wires
        for (unsigned int j = 0; j < this->m_channelsPerSpectrum; j++) {
          localWorkspace->maskBin(minPos, j, 1);
        }
        count++;
        checkContinue = true;
        TableRow t = outputws->appendRow();
        t << int(minPos);
      }
    }
  }

  g_log.information() << "_poldi : auto removed wires (nb:" << count << ")"
                      << std::endl;
  setProperty("nbAuteDeadWires", count);
}

} // namespace Poldi
} // namespace Mantid
