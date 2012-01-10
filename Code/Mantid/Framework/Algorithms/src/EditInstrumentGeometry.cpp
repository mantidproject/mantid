/*WIKI*



This algorithm edits the diffractomer detectors' parameters




*WIKI*/

#include "MantidAlgorithms/EditInstrumentGeometry.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidGeometry/IDetector.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidKernel/System.h"
#include "MantidKernel/ArrayProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Algorithms
{

  DECLARE_ALGORITHM(EditInstrumentGeometry);

  //---------------------------------------------- ------------------------------------------------
  /** Constructor
   */
  EditInstrumentGeometry::EditInstrumentGeometry()
  {
    // TODO Auto-generated constructor stub
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  EditInstrumentGeometry::~EditInstrumentGeometry()
  {
    // TODO Auto-generated destructor stub
  }
  
  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void EditInstrumentGeometry::initDocs()
  {
    this->setWikiSummary("Add and/or edit T.O.F. powder diffractomer instrument geometry information");
    this->setOptionalMessage("The edit or added information will be attached to a Workspace.  Currently it is in an overwrite mode only.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void EditInstrumentGeometry::init()
  {
    declareProperty(new WorkspaceProperty<>("Workspace", "", Direction::InOut),
        "Workspace to edit the detector information");
    declareProperty("InstrumentName", "GenericPowder", "Name of the instrument. ");
    declareProperty("PrimaryFlightPath", -1.0);
    declareProperty(new ArrayProperty<int32_t>("SpectrumIDs", new MandatoryValidator<std::vector<int32_t> >),
        "Spectrum IDs (note that it is not detector ID or workspace indices).");
    declareProperty(new ArrayProperty<double>("L2", new MandatoryValidator<std::vector<double> >),
        "Seconary flight (L2) paths for each detector");
    declareProperty(new ArrayProperty<double>("Polar", new MandatoryValidator<std::vector<double> >),
        "Polar angles (two thetas) for detectors");
    declareProperty(new ArrayProperty<double>("Azimuthal", new MandatoryValidator<std::vector<double> >),
        "Azimuthal angles (out-of-plain) for detectors");
    declareProperty("NewInstrument", false, "Add detectors information from scratch");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void EditInstrumentGeometry::exec()
  {

    // 1. Get Input
    MatrixWorkspace_sptr workspace = getProperty("Workspace");
    double l1 = this->getProperty("PrimaryFlightPath");
    const std::vector<int32_t> specids = this->getProperty("SpectrumIDs");
    const std::vector<double> l2s = this->getProperty("L2");
    const std::vector<double> tths = this->getProperty("Polar");
    const std::vector<double> phis = this->getProperty("Azimuthal");
    const bool newinstrument = getProperty("NewInstrument");

    spec2index_map * spec2indexmap = workspace->getSpectrumToWorkspaceIndexMap();

    // 2. Check validity
    if (l1 > 0){
        g_log.information() << "L1 = " << l1 << "  # Detector = " << std::endl;
    }

    if (specids.size() != l2s.size() || l2s.size() != tths.size() || phis.size() != l2s.size()){
      delete spec2indexmap;
      throw std::invalid_argument("Input Spectrum, L2 (Secondary Flight Paths), Polar angles (Two Thetas) and Azimuthal have different items number");
    }

    for (size_t ib = 0; ib < l2s.size(); ib ++){
      g_log.information() << "Detector " << specids[ib] << "  L2 = " << l2s[ib] << "  2Theta = " << tths[ib] << std::endl;
      if (specids[ib] < 0){
        delete spec2indexmap;
        throw std::invalid_argument("Detector ID cannot be less than 0");
      }
      if (l2s[ib] <= 0.0){
        delete spec2indexmap;
        throw std::invalid_argument("L2 cannot be less or equal to 0");
      }
    }

    // 3. Get information from original: L1
    Geometry::Instrument_const_sptr oldinstrument = workspace->getInstrument();

    double mL1;
    if (newinstrument){
      // use input L1
      mL1 = l1;
      if (l1 <= 0){
        delete spec2indexmap;
        g_log.error() << "Input L1 = " << l1 << "  < 0 Is Not Allowed!" << std::endl;
        throw std::invalid_argument("Input L1 is not allowed.");
      }
    } else {
      // use L1 belonged to the current (old) instrument
      if (l1 > 0){
        mL1 = l1;
      } else {
        Kernel::V3D sourcepos = oldinstrument->getSource()->getPos();
        mL1 = sourcepos.Z()*-1;
        g_log.debug() << "source position = " << sourcepos.X() << ", " << sourcepos.Y() << ", " << sourcepos.Z() << std::endl;
      }
    }

    // 4. Keep original instrument and set the new instrument, if necessary
    //    Condition: spectrum has 1 and only 1 detector
    std::vector<bool> wsindexsetflag;
    std::vector<bool> storable;
    std::vector<double> storL2s;
    std::vector<double> stor2Thetas;
    std::vector<double> storPhis;
    std::vector<detid_t> storDetids;
    for (size_t i = 0; i < workspace->getNumberHistograms(); i ++){
      wsindexsetflag.push_back(false);
      storable.push_back(false);
      storL2s.push_back(0.0);
      stor2Thetas.push_back(0.0);
      storPhis.push_back(0.0);
      storDetids.push_back(0);
    }

    // 4.1 Sort out workspace (index) will be stored
    for (size_t i = 0; i < specids.size(); i ++){
      // a) Find spectrum's workspace index
      spec2index_map::iterator it = spec2indexmap->find(specids[i]);
      if (it == spec2indexmap->end()){
        g_log.error() << "Spectrum ID " << specids[i] << " is not found" << std::endl;
        continue;
      }
      size_t workspaceindex = it->second;

      // b) Store and set value
      wsindexsetflag[workspaceindex] = true;
      storL2s[workspaceindex] = l2s[i];
      stor2Thetas[workspaceindex] = tths[i];
      storPhis[workspaceindex] = phis[i];

      g_log.debug() << "workspace index = " << workspaceindex << " is for Spectrum " << specids[i] << std::endl;
    }

    // 4.2 Storable?  (1) NOT TO BE SET ... and (2) DETECTOR > 1
    for (size_t i = 0; i < workspace->getNumberHistograms(); i ++){
      if (!wsindexsetflag[i]){
        // Won't be set
        API::ISpectrum *spectrum = workspace->getSpectrum(i);
        std::set<detid_t> detids = spectrum->getDetectorIDs();
        if (detids.size() == 1){
          // 1 and only 1 detector
          storable[i] = true;
          // a) get DetectorID
          detid_t detid = 0;
          std::set<detid_t>::iterator it;
          for (it=detids.begin(); it!=detids.end(); ++it){
            detid = *it;
          }
          // b) get Detector
          Geometry::IDetector_const_sptr stodet = oldinstrument->getDetector(detid);
          double rt, thetat, phit;
          stodet->getPos().getSpherical(rt, thetat, phit);
          // c) Store
          storDetids[i] = detid;
          storL2s[i] = rt;
          stor2Thetas[i] = thetat;
          storPhis[i] = phit;

        } else {
          // more than 1 detectors
          storable[i] = false;
        }
      }
    }

    // 5. Generate a new instrument
    // 5.1 Name
    std::string name = "";
    if (newinstrument){
      std::string tempname = getProperty("InstrumentName");
      name += tempname;
    } else {
      name += oldinstrument->getName();
    }

    //  5.2 Create a new instrument from scratch
    Geometry::Instrument_sptr instrument(new Geometry::Instrument(name));
    if (instrument.get() == 0)
    {
      delete spec2indexmap;
      g_log.error("Trying to use a Parametrized Instrument as an Instrument.");
      throw std::runtime_error("Trying to use a Parametrized Instrument as an Instrument.");
    }

    // 5.3 Source and sample information
    Geometry::ObjComponent *samplepos = new Geometry::ObjComponent("Sample", instrument.get());
    instrument->add(samplepos);
    instrument->markAsSamplePos(samplepos);
    samplepos->setPos(0.0, 0.0, 0.0);

    Geometry::ObjComponent *source = new Geometry::ObjComponent("Source", instrument.get());
    instrument->add(source);
    instrument->markAsSource(source);
    source->setPos(0.0, 0.0, -1.0 * mL1);

    // 5.4 Set the new instrument
    workspace->setInstrument(instrument);

    // 6. Add or copy detector information
    for (size_t i = 0; i < workspace->getNumberHistograms(); i ++){

      if (wsindexsetflag[i] || storable[i]){
        // 6.1 Create a new detector.
        //     (Instrument will take ownership of pointer so no need to delete.)
        detid_t newdetid = detid_t(i)+100;
        Geometry::Detector *detector = new Geometry::Detector("det", newdetid, samplepos);

        // 6.2 Set the new instrument
        double l2 = storL2s[i];
        double tth = stor2Thetas[i];
        double phi = storPhis[i];

        Kernel::V3D pos;
        pos.spherical(l2, tth, phi);
        detector->setPos(pos);

        // 6.3 add copy to instrument and mark it
        API::ISpectrum *spectrum = workspace->getSpectrum(i);
        if (!spectrum){
          g_log.error() << g_log.error() << "Spectrum ID " << specids[i] << " does not exist!  Skip setting detector parameters to this spectrum" << std::endl;
          continue;
        } else {
          g_log.debug() << "Raw: Spectrum " << spectrum->getSpectrumNo() << ": # Detectors =  " << spectrum->getDetectorIDs().size() << std::endl;
        }

        spectrum->clearDetectorIDs();
        spectrum->addDetectorID(newdetid);
        instrument->add(detector);
        instrument->markAsDetector(detector);

      }

    } // for i

    delete spec2indexmap;

    return;
  }

} // namespace Mantid
} // namespace Algorithms

