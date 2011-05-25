#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Utils.h"
#include "MantidMDEvents/BoxController.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidTestHelpers/DLLExport.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"


using Mantid::DataObjects::EventWorkspace_sptr;
using Mantid::Kernel::DateAndTime;
using Mantid::DataHandling::LoadInstrument;
using Mantid::DataObjects::EventWorkspace;


/** Set of helper methods for testing MDEventWorkspace things
 *
 * @author Janik Zikovsky
 * @date March 29, 2011
 * */
namespace MDEventsTestHelper
{



  /** Create an EventWorkspace containing fake data
   * of single-crystal diffraction.
   * Instrument is MINITOPAZ
   *
   * @return EventWorkspace_sptr
   */
  EventWorkspace_sptr createDiffractionEventWorkspace(int numEvents)
  {
    Mantid::Kernel::ConfigService::Instance().setString("default.facility", "TEST");
    int numPixels = 10000;
    int numBins = 1600;
    double binDelta = 10.0;

    EventWorkspace_sptr retVal(new EventWorkspace);
    retVal->initialize(numPixels,1,1);

    // --------- Load the instrument -----------
    LoadInstrument * loadInst = new LoadInstrument();
    loadInst->initialize();
    loadInst->setPropertyValue("Filename", "IDFs_for_UNIT_TESTING/MINITOPAZ_Definition.xml");
    loadInst->setProperty<Mantid::API::MatrixWorkspace_sptr> ("Workspace", retVal);
    loadInst->execute();
    delete loadInst;
    // Populate the instrument parameters in this workspace - this works around a bug
    retVal->populateInstrumentParameters();

    DateAndTime run_start("2010-01-01");

    for (int pix = 0; pix < numPixels; pix++)
    {
      for (int i=0; i<numEvents; i++)
      {
        retVal->getEventListAtPixelID(pix) += Mantid::DataObjects::TofEvent((i+0.5)*binDelta, run_start+double(i));
      }

    }
    retVal->doneLoadingData();

    //Create the x-axis for histogramming.
    Mantid::MantidVecPtr x1;
    Mantid::MantidVec& xRef = x1.access();
    xRef.resize(numBins);
    for (int i = 0; i < numBins; ++i)
    {
      xRef[i] = i*binDelta;
    }

    //Set all the histograms at once.
    retVal->setAllX(x1);

    // Give it a crystal and goniometer
    WorkspaceCreationHelper::SetGoniometer(retVal, 0., 0., 0.);
    WorkspaceCreationHelper::SetOrientedLattice(retVal, 1., 1., 1.);

    // Some sanity checks
    if (retVal->getInstrument()->getName() != "MINITOPAZ")
      throw std::runtime_error("MDEventsTestHelper::createDiffractionEventWorkspace(): Wrong instrument loaded.");
    Mantid::detid2det_map dets;
    retVal->getInstrument()->getDetectors(dets);
    if ( dets.size() != 100*100)
      throw std::runtime_error("MDEventsTestHelper::createDiffractionEventWorkspace(): Wrong instrument size.");


    return retVal;
  }


} // namespace

