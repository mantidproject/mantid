#include "MantidQtCustomInterfaces/Poldi/PoldiPresenter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtCustomInterfaces/Poldi/IPoldiView.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCommand.h"
#include <Poco/ActiveResult.h>
#include <qapplication.h>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
* @param view : [input] The view this presenter handles
* @param presenter : [input] The DataProcessor presenter that will tell us when
* the ADS changes
*/
PoldiPresenter::PoldiPresenter(
    IPoldiView *view, boost::shared_ptr<DataProcessorPresenter> presenter)
    : m_view(view), m_presenter(presenter) {

  // This is very important, it means that our table presenter will use this
  // presenter to communicate changes in the ADS
  // Without this the interface will crash
  m_presenter->accept(this);

  // Set up the instrument selectors
  std::vector<std::string> instruments = {"POLDI"};
  m_presenter->setInstrumentList(instruments, "POLDI");
}

/** Default destructor */
PoldiPresenter::~PoldiPresenter() {}

/** This method is used by the view to tell this presenter that something has
 * happened
 * @param flag : [input] A flag indicating what happened
 */
void PoldiPresenter::notify(IPoldiPresenter::Flag flag) {

  switch (flag) {
  case IPoldiPresenter::LoadDemoFlag:
    loadDemoTable();
    break;
  }
}

/** This method is used by the DataProcessor presenter to tell this presenter
* that something has changed in the ADS. We don't need to do anything, as in
* this interface, but we need to have this flag defined.
* @param flag : [input] A flag indicating what happened (currently only one
* possible flag)
*/
void PoldiPresenter::notify(WorkspaceReceiver::Flag flag) {

  switch (flag) {
  case WorkspaceReceiver::ADSChangedFlag:
    // Do nothing
    break;
  }
}

/** Slot to load a demo table following user example in PoldiDataAnalysis
 * documentation */
void PoldiPresenter::loadDemoTable() {

  // First load some runs into Mantid (see usage example for PoldiDataAnalysis)
  auto loader =
      Mantid::API::AlgorithmManager::Instance().create("PoldiLoadRuns");
  loader->setPropertyValue("Year", "2013");
  loader->setPropertyValue("FirstRun", "6903");
  loader->setPropertyValue("LastRun", "6904");
  loader->setPropertyValue("MergeWidth", "2");
  loader->setPropertyValue("OutputWorkspace", "poldi");
  auto resultLoader = loader->executeAsync();

  // Create Silicon peaks
  auto peakCreator = Mantid::API::AlgorithmManager::Instance().create(
      "PoldiCreatePeaksFromCell");
  peakCreator->setPropertyValue("SpaceGroup", "F d -3 m");
  peakCreator->setPropertyValue("Atoms", "Si 0 0 0 1.0 0.01");
  peakCreator->setPropertyValue("a", "5.431");
  peakCreator->setPropertyValue("LatticeSpacingMin", "0.7");
  peakCreator->setPropertyValue("OutputWorkspace", "Si");
  auto resultPeakCreator = peakCreator->executeAsync();

  while (!resultPeakCreator.available() || !resultLoader.available()) {
    // Wait for the results
    QApplication::processEvents();
  }

  // Then populate the table
  std::map<std::string, std::string> firstRow = {
      {"Run(s)", "poldi_data_6904"},
      {"Expected peak(s)", "Si"},
      {"Profile function", "Gaussian"},
      {"Maximum number of peaks", "8"}};
  std::vector<std::map<std::string, std::string>> toTransfer;
  toTransfer.push_back(firstRow);
  m_presenter->transfer(toTransfer);

  m_view->giveUserInfo("Hit 'Process' to reduce the data", "Info");
}

} // CustomInterfaces
} // MantidQt
