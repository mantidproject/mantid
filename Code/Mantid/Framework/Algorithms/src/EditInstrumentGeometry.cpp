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

    // 2. Check validity
    bool userL1 = true;
    if (l1 <= 0){
      userL1 = false;
    } else {
      g_log.information() << "L1 = " << l1 << "  # Detector = " << std::endl;
    }

    if (specids.size() != l2s.size() || l2s.size() != tths.size() || phis.size() != l2s.size()){
      throw std::invalid_argument("Input Spectrum, L2 (Secondary Flight Paths), Polar angles (Two Thetas) and Azimuthal have different items number");
    }

    for (size_t ib = 0; ib < l2s.size(); ib ++){
      g_log.information() << "Detector " << specids[ib] << "  L2 = " << l2s[ib] << "  2Theta = " << tths[ib] << std::endl;
      if (specids[ib] < 0){
        throw std::invalid_argument("Detector ID cannot be less than 0");
      }
      if (l2s[ib] <= 0.0){
        throw std::invalid_argument("L2 cannot be less or equal to 0");
      }
    }

    // 3. Generate a new instrument
    std::string name = "";
    if (newinstrument){
      std::string tempname = getProperty("InstrumentName");
      name += tempname;
    } else {
      name += workspace->getInstrument()->getName();
    }

    //  Create a new instrument from scratch
    Geometry::Instrument_sptr instrument(new Geometry::Instrument(name));
    if (instrument.get() == 0)
    {
      g_log.error("Trying to use a Parametrized Instrument as an Instrument.");
      throw std::runtime_error("Trying to use a Parametrized Instrument as an Instrument.");
    }

    // 4. Get information from original: L1
    double mL1;
    if (newinstrument){
      // use input L1
      mL1 = l1;
      if (l1 <= 0){
        g_log.error() << "Input L1 = " << l1 << "  < 0 Is Not Allowed!" << std::endl;
        throw std::invalid_argument("Input L1 is not allowed.");
      }
    } else {
      if (l1 > 0){
        mL1 = l1;
      } else {
        Kernel::V3D sourcepos = workspace->getInstrument()->getSource()->getPos();
        mL1 = sourcepos.Z()*-1;
        g_log.notice() << "source position = " << sourcepos.X() << ", " << sourcepos.Y() << ", " << sourcepos.Z() << std::endl;
      }
    }

    // 5. Source and sample information
    Geometry::ObjComponent *samplepos = new Geometry::ObjComponent("Sample", instrument.get());
    instrument->add(samplepos);
    instrument->markAsSamplePos(samplepos);
    samplepos->setPos(0.0, 0.0, 0.0);

    Geometry::ObjComponent *source = new Geometry::ObjComponent("Source", instrument.get());
    instrument->add(source);
    instrument->markAsSource(source);
    source->setPos(0.0, 0.0, -1.0 * mL1);

    // 6. Keep original instrument and set the new instrument
    Geometry::Instrument_const_sptr oldinstrument = workspace->getInstrument();
    workspace->setInstrument(instrument);

    // 6. Add detector information
    spec2index_map * spec2indexmap = workspace->getSpectrumToWorkspaceIndexMap();

    for (size_t i = 0; i < specids.size(); i ++){
      // Create a new detector. Instrument will take ownership of pointer so no need to delete.
      Geometry::Detector *detector = new Geometry::Detector("det", specids[i]+100, samplepos);
      spec2index_map::iterator it = spec2indexmap->find(specids[i]);
      if (it == spec2indexmap->end()){
        g_log.error() << "Spectrum ID " << specids[i] << " is not found" << std::endl;
        continue;
      }
      size_t workspaceindex = it->second;
      g_log.notice() << "workspace index = " << workspaceindex << " is for Spectrum " << specids[i] << std::endl;

      Kernel::V3D pos;
      double l2 = l2s[i];
      double tth = tths[i];
      double phi = phis[i];
      pos.spherical(l2, tth, phi);
      detector->setPos(pos);
      double tl2, t2t, tph;
      detector->getPos().getSpherical(tl2, t2t, tph);
      g_log.notice() << "Detector Position (Input) = " << l2 << ", " << tth << ", " << phi << std::endl;
      g_log.notice() << "New Detector " << specids[i] << ":  L2, 2theta, phi = " << tl2 << ", " << t2t << ", " << tph << std::endl;

      // add copy to instrument and mark it
      API::ISpectrum *spectrum = workspace->getSpectrum(workspaceindex);
      if (!spectrum){
        g_log.error() << g_log.error() << "Spectrum ID " << specids[i] << " does not exist!  Skip setting detector parameters to this spectrum" << std::endl;
        continue;
      } else {
        g_log.notice() << "Raw: Spectrum " << specids[i] << ": # Detectors =  " << spectrum->getDetectorIDs().size() << std::endl;
      }

      spectrum->clearDetectorIDs();
      spectrum->addDetectorID(specids[i]+100);
      instrument->add(detector);
      instrument->markAsDetector(detector);

      // Check
      g_log.notice() << "New: Spectrum: " << specids[i] << ": # Detectors = " << spectrum->getDetectorIDs().size() << std::endl;


    } // for i

    delete spec2indexmap;

    return;
  }

} // namespace Mantid
} // namespace Algorithms

