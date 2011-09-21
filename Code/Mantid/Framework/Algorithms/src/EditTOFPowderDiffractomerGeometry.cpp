#include "MantidAlgorithms/EditTOFPowderDiffractomerGeometry.h"
#include "MantidKernel/System.h"
#include "MantidKernel/ArrayProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Algorithms
{

  //DECLARE_ALGORITHM(EditTOFPowderDiffractomerGeometry);

  //---------------------------------------------- ------------------------------------------------
  /** Constructor
   */
  EditTOFPowderDiffractomerGeometry::EditTOFPowderDiffractomerGeometry()
  {
    // TODO Auto-generated constructor stub
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  EditTOFPowderDiffractomerGeometry::~EditTOFPowderDiffractomerGeometry()
  {
    // TODO Auto-generated destructor stub
  }
  
  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void EditTOFPowderDiffractomerGeometry::initDocs()
  {
    this->setWikiSummary("Add and/or edit T.O.F. powder diffractomer instrument geometry information");
    this->setOptionalMessage("The edit or added information will be attached to a Workspace.  Currently it is in an overwrite mode only.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void EditTOFPowderDiffractomerGeometry::init()
  {
    declareProperty(new WorkspaceProperty<>("Workspace","",Direction::InOut));
    declareProperty("Name", "");
    declareProperty("PrimaryFlightPath", -1.0);
    declareProperty(new ArrayProperty<int32_t>("DetectorIDs", new MandatoryValidator<std::vector<int32_t> >),
        "Detector IDs.");
    declareProperty(new ArrayProperty<double>("SecondaryFlightPaths", new MandatoryValidator<std::vector<double> >),
        "Seconary flight paths for each detector");
    declareProperty(new ArrayProperty<double>("TwoThetas", new MandatoryValidator<std::vector<double> >),
        "Two thetas for all detectors");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void EditTOFPowderDiffractomerGeometry::exec()
  {

    // 1. Get Input
    MatrixWorkspace_sptr workspace = getProperty("Workspace");
    std::string name = getProperty("Name");
    double l1 = this->getProperty("PrimaryFlightPath");
    const std::vector<int32_t> detectorids = this->getProperty("DetectorIDs");
    const std::vector<double> l2s = this->getProperty("SecondaryFlightPaths");
    const std::vector<double> tths = this->getProperty("TwoThetas");

    // 2. Check validity
    g_log.notice() << "L1 = " << l1 << "  # Detector = " << detectorids.size() << std::endl;
    if (l1 <= 0){
      throw std::invalid_argument("Primary flight path cannot be less or equal to 0");
    }

    if (detectorids.size() != l2s.size() || l2s.size() != tths.size()){
      throw std::invalid_argument("Input Detector IDs, Secondary Flight Paths, and Two Thetas have different items number");
    }

    for (size_t ib = 0; ib < l2s.size(); ib ++){
      g_log.information() << "Detector " << detectorids[ib] << "  L2 = " << l2s[ib] << "  2Theta = " << tths[ib] << std::endl;
      if (detectorids[ib] < 0){
        throw std::invalid_argument("Detector ID cannot be less than 0");
      }
      if (l2s[ib] <= 0.0){
        throw std::invalid_argument("L2 cannot be less or equal to 0");
      }
    }

    // 3. Generate a new instrument or use the old one
    // a) Create a new instrument
    if (name == ""){
      name = "Generic";
    }
    Geometry::Instrument_sptr instrument(new Geometry::Instrument(name));
    workspace->setInstrument(instrument);

    // getBaseInstrument();
    if (instrument.get() == 0)
    {
        g_log.error("Trying to use a Parametrized Instrument as an Instrument.");
        throw std::runtime_error("Trying to use a Parametrized Instrument as an Instrument.");
    }

    // 3. Source and sample information
    Geometry::ObjComponent *samplepos = new Geometry::ObjComponent("Sample", instrument.get());
    instrument->add(samplepos);
    instrument->markAsSamplePos(samplepos);
    samplepos->setPos(0.0, 0.0, 0.0);

    Geometry::ObjComponent *source = new Geometry::ObjComponent("Source", instrument.get());
    instrument->add(source);
    instrument->markAsSource(source);
    source->setPos(0.0, 0.0, -1.0 * l1);

    // 4. Add detector information
    // Create a new detector. Instrument will take ownership of pointer so no need to delete.
    for (size_t ib = 0; ib < detectorids.size(); ib ++){
      Geometry::Detector *detector = new Geometry::Detector("det", detectorids[ib], samplepos);
      Kernel::V3D pos;

      double l2 = l2s[ib];
      double tth = tths[ib];
      pos.spherical(l2, tth, 0.0 );
      detector->setPos(pos);

      // add copy to instrument and mark it
      instrument->add(detector);
      instrument->markAsDetector(detector);
    }

    return;
  }

} // namespace Mantid
} // namespace Algorithms

