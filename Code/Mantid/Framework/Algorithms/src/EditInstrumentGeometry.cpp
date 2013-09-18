/*WIKI*
This algorithm can 
 1. add an Instrument to a Workspace without any real instrument associated with, or
 2. replace a Workspace's Instrument with a new Instrument, or
 3. edit whole and partial detectors' parameters of the Instrument associated with a Workspace.

==Limitations==
There are some limitations of this algorithm.  

1. The key to locate the detector is via spectrum ID;

2. For each spectrum, there is only one and only one new detector.  Thus, if one spectrum is associated with a group of detectors previously, the replacement (new) detector is the one which is (diffraction) focused on after this algorithm is called.

3. If only part of the spectra to have detector edited, for rest of the spectra, if any of them is associated with a group of detectors, then after the algorithm is called, this spectra won't have any detector associated.

==Instruction==
1. For powder diffractomer, user can input
   SpectrumIDs = "1, 2, 3"
   L2 = "3.1, 3.2, 3.3"
   Polar = "90.01, 90.02, 90.03"
   Azimuthal = "0.1,0.2,0.3"
   NewInstrument = False
   to set up the focused detectors' parameters for spectrum 1, 2 and 3.  
*WIKI*/

#include "MantidAlgorithms/EditInstrumentGeometry.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidKernel/System.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MandatoryValidator.h"

#include <sstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;

using namespace std;

namespace Mantid
{
namespace Algorithms
{

  DECLARE_ALGORITHM(EditInstrumentGeometry)

  //---------------------------------------------- ------------------------------------------------
  /** Constructor
   */
  EditInstrumentGeometry::EditInstrumentGeometry()
  { }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  EditInstrumentGeometry::~EditInstrumentGeometry()
  { }

  const std::string EditInstrumentGeometry::name() const
  {
    return "EditInstrumentGeometry";
  }

  const std::string EditInstrumentGeometry::category() const
  {
    return "Diffraction";
  }

  int EditInstrumentGeometry::version() const
  {
    return 1;
  }

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void EditInstrumentGeometry::initDocs()
  {
    this->setWikiSummary("Add, substitute and edit an Instrument associated with a Workspace");
    this->setOptionalMessage("The edit or added information will be attached to a Workspace.  Currently it is in an overwrite mode only.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void EditInstrumentGeometry::init()
  {
    // Input workspace
    declareProperty(new WorkspaceProperty<>("Workspace", "", Direction::InOut),
                    "Workspace to edit the detector information");

    // L1
    declareProperty("PrimaryFlightPath", EMPTY_DBL(), "Primary flight path L1 of the powder diffractomer. ");


    // Spectrum ID for the spectrum to have instrument geometry edited
    declareProperty(new ArrayProperty<int32_t>("SpectrumIDs"),
                    "Spectrum IDs (note that it is not detector ID or workspace indices). "
                    "Number of spectrum IDs must be same as workspace's histogram number.");

    auto required = boost::make_shared<MandatoryValidator<std::vector<double> > >();

    // Vector for L2
    declareProperty(new ArrayProperty<double>("L2", required),
                    "Seconary flight (L2) paths for each detector.  Number of L2 given must be same as number of histogram.");

    // Vector for 2Theta
    declareProperty(new ArrayProperty<double>("Polar", required),
                    "Polar angles (two thetas) for detectors. Number of 2theta given must be same as number of histogram.");

    // Vector for Azimuthal angle
    declareProperty(new ArrayProperty<double>("Azimuthal"),
                    "Azimuthal angles (out-of-plain) for detectors. "
                    "Number of azimuthal angles given must be same as number of histogram.");

    // Detector IDs
    declareProperty(new ArrayProperty<int>("DetectorIDs"), "User specified detector IDs of the spectra. "
                    "Number of specified detector IDs must be either zero or number of histogram");

    // Instrument Name
    declareProperty("InstrumentName", "", "Name of the newly built instrument.  If left empty, "
                    "the original instrument will be used. ");

    return;
  }

  template <typename NumT>
  std::string checkValues(const std::vector<NumT> &thingy,
                          const size_t numHist)
  {
    if ((!thingy.empty()) && thingy.size() != numHist)
    {
      stringstream msg;
      msg << "Must equal number of spectra or be empty (" << numHist
            << " != " << thingy.size() << ")";
      return msg.str();
    }
    else
    {
      return "";
    }
  }

  std::map<std::string, std::string> EditInstrumentGeometry::validateInputs()
  {
    std::map<std::string, std::string> result;

    // everything depends on being parallel to the workspace # histo
    size_t numHist(0);
    {
      MatrixWorkspace_const_sptr workspace = getProperty("Workspace");
      numHist = workspace->getNumberHistograms();
    }

    std::string error;

    const std::vector<int32_t> specids = this->getProperty("SpectrumIDs");
    error = checkValues(specids, numHist);
    if (!error.empty())
      result["SpectrumIDs"] = error;

    const std::vector<double> l2 = this->getProperty("L2");
    error = checkValues(l2, numHist);
    if (!error.empty())
      result["L2"] = error;

    const std::vector<double> tth = this->getProperty("Polar");
    error = checkValues(tth, numHist);
    if (!error.empty())
      result["Polar"] = error;

    const std::vector<double> phi = this->getProperty("Azimuthal");
    error = checkValues(phi, numHist);
    if (!error.empty())
      result["Azimuthal"] = error;

    const vector<int> detids = getProperty("DetectorIDs");
    error = checkValues(detids, numHist);
    if (!error.empty())
      result["DetectorIDs"] = error;

    // TODO verify that SpectrumIDs, L2, Polar, Azimuthal, and DetectorIDs are parallel or not specified
    return result;
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void EditInstrumentGeometry::exec()
  {
    // Lots of things have to do with the input workspace
    MatrixWorkspace_sptr workspace = getProperty("Workspace");
    Geometry::Instrument_const_sptr originstrument = workspace->getInstrument();

    // Get and check the primary flight path
    double l1 = this->getProperty("PrimaryFlightPath");
    if (isEmpty(l1))
    {
      // Use the original L1
      if (!originstrument)
      {
        std::string errmsg("It is not supported that L1 is not given, ",
                           "while there is no instrument associated to input workspace.");
        g_log.error(errmsg);
        throw std::runtime_error(errmsg);
      }
      Geometry::IObjComponent_const_sptr source = originstrument->getSource();
      Geometry::IObjComponent_const_sptr sample = originstrument->getSample();
      l1 = source->getDistance(*sample);
      g_log.information() << "Retrieve L1 from input data workspace. \n";
    }
    g_log.information() << "Using L1 = " << l1 << "\n";

    // Get spectra number in case they are in a funny order
    std::vector<int32_t> specids = this->getProperty("SpectrumIDs");
    if (specids.empty()) // they are using the order of the input workspace
    {
      size_t numHist = workspace->getNumberHistograms();
      for (size_t i = 0; i < numHist; ++i)
      {
        specids.push_back(workspace->getSpectrum(i)->getSpectrumNo());
        g_log.information() << "Add spectrum " << workspace->getSpectrum(i)->getSpectrumNo() << ".\n";
      }
    }

    // Get the detector ids - empsy means ignore it
    const vector<int> vec_detids = getProperty("DetectorIDs");
    const bool renameDetID(!vec_detids.empty());

    // Get individual detector geometries ordered by input spectrum IDs
    const std::vector<double> l2s = this->getProperty("L2");
    const std::vector<double> tths = this->getProperty("Polar");
    std::vector<double> phis = this->getProperty("Azimuthal");

    // empty list of L2 and 2-theta is not allowed
    if (l2s.empty())
    {
      throw std::runtime_error("User must specify L2 for all spectra. ");
    }
    if (tths.empty())
    {
      throw std::runtime_error("User must specify 2theta for all spectra.");
    }

    // empty list of phi means that they are all zero
    if (phis.empty())
    {
      phis.assign(l2s.size(), 0.);
    }

    // Validate
    for (size_t ib = 0; ib < l2s.size(); ib ++)
    {
      g_log.information() << "Detector " << specids[ib] << "  L2 = " << l2s[ib] << "  2Theta = " << tths[ib] << std::endl;
      if (specids[ib] < 0)
      {
        // Invalid spectrum ID : less than 0.
        stringstream errmsgss;
        errmsgss << "Detector ID = " << specids[ib] << " cannot be less than 0.";
        throw std::invalid_argument(errmsgss.str());
      }
      if (l2s[ib] <= 0.0)
      {
        throw std::invalid_argument("L2 cannot be less or equal to 0");
      }
    }

    // Keep original instrument and set the new instrument, if necessary
    const auto spec2indexmap(workspace->getSpectrumToWorkspaceIndexMap());

    // ??? Condition: spectrum has 1 and only 1 detector
    size_t nspec = workspace->getNumberHistograms();

    // Initialize another set of L2/2-theta/Phi/DetectorIDs vector ordered by workspace index
    std::vector<double> storL2s(nspec, 0.);
    std::vector<double> stor2Thetas(nspec, 0.);
    std::vector<double> storPhis(nspec, 0.);
    vector<int> storDetIDs(nspec, 0);

    // Map the properties from spectrum ID to workspace index
    for (size_t i = 0; i < specids.size(); i ++)
    {
      // Find spectrum's workspace index
      spec2index_map::const_iterator it = spec2indexmap.find(specids[i]);
      if (it == spec2indexmap.end())
      {
        stringstream errss;
        errss << "Spectrum ID " << specids[i] << " is not found. "
              << "Instrument won't be edited for this spectrum. " << std::endl;
        g_log.error(errss.str());
        throw std::runtime_error(errss.str());
      }

      // Store and set value
      size_t workspaceindex = it->second;

      storL2s[workspaceindex] = l2s[i];
      stor2Thetas[workspaceindex] = tths[i];
      storPhis[workspaceindex] = phis[i];
      if (renameDetID)
        storDetIDs[workspaceindex] = vec_detids[i];

      g_log.debug() << "workspace index = " << workspaceindex << " is for Spectrum " << specids[i] << std::endl;
    }

    // Generate a new instrument
    // Name of the new instrument
    std::string name = std::string(getProperty("InstrumentName"));
    if (name.empty())
    {
      // Use the original L1
      if (!originstrument)
      {
        std::string errmsg("It is not supported that InstrumentName is not given, ",
                           "while there is no instrument associated to input workspace.");
        g_log.error(errmsg);
        throw std::runtime_error(errmsg);
      }
      name = originstrument->getName();
    }

    // Create a new instrument from scratch any way.
    Geometry::Instrument_sptr instrument(new Geometry::Instrument(name));
    if (!bool(instrument))
    {
      stringstream errss;
      errss << "Trying to use a Parametrized Instrument as an Instrument.";
      g_log.error(errss.str());
      throw std::runtime_error(errss.str());
    }

    // Set up source and sample information
    Geometry::ObjComponent *samplepos = new Geometry::ObjComponent("Sample", instrument.get());
    instrument->add(samplepos);
    instrument->markAsSamplePos(samplepos);
    samplepos->setPos(0.0, 0.0, 0.0);

    Geometry::ObjComponent *source = new Geometry::ObjComponent("Source", instrument.get());
    instrument->add(source);
    instrument->markAsSource(source);
    source->setPos(0.0, 0.0, -1.0 * l1);

    // Add the new instrument
    workspace->setInstrument(instrument);

    // Add/copy detector information
    for (size_t i = 0; i < workspace->getNumberHistograms(); i ++)
    {
      // Create a new detector.
      //    (Instrument will take ownership of pointer so no need to delete.)
      detid_t newdetid;
      if (renameDetID)
        newdetid = storDetIDs[i];
      else
        newdetid = detid_t(i)+100;
      Geometry::Detector *detector = new Geometry::Detector("det", newdetid, samplepos);

      // Set up new detector parameters related to new instrument
      double l2 = storL2s[i];
      double tth = stor2Thetas[i];
      double phi = storPhis[i];

      Kernel::V3D pos;
      pos.spherical(l2, tth, phi);
      detector->setPos(pos);

      // Add new detector to spectrum and instrument
      API::ISpectrum *spectrum = workspace->getSpectrum(i);
      if (!spectrum)
      {
        // Error!
        delete detector;

        stringstream errss;
        errss << "Spectrum ID " << specids[i]
              << " does not exist!  Skip setting detector parameters to this spectrum. ";
        g_log.error(errss.str());
        throw runtime_error(errss.str());
      }
      else
      {
        // Good and do some debug output
        g_log.debug() << "Orignal spectrum " << spectrum->getSpectrumNo()
                      << "has " << spectrum->getDetectorIDs().size() << " detectors. \n";
      }

      spectrum->clearDetectorIDs();
      spectrum->addDetectorID(newdetid);
      instrument->add(detector);
      instrument->markAsDetector(detector);

    } // ENDFOR workspace index

    return;
  }

} // namespace Mantid
} // namespace Algorithms

