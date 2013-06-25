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

    // Instrument Name
    declareProperty("InstrumentName", "GenericPowder", "Name of the instrument. ");

    // L1
    declareProperty("PrimaryFlightPath", EMPTY_DBL(), "Primary flight path L1 of the powder diffractomer. ");

    auto required = boost::make_shared<MandatoryValidator<std::vector<double> > >();

    // Spectrum ID for the spectrum to have instrument geometry edited
    declareProperty(new ArrayProperty<int32_t>("SpectrumIDs", boost::make_shared<MandatoryValidator<std::vector<int32_t>>>()),
                    "Spectrum IDs (note that it is not detector ID or workspace indices). "
                    "Number of spectrum IDs must be same as workspace's histogram number.");

    // Vector for L2
    declareProperty(new ArrayProperty<double>("L2", required),
                    "Seconary flight (L2) paths for each detector.  Number of L2 given must be same as number of histogram.");

    // Vector for 2Theta
    declareProperty(new ArrayProperty<double>("Polar", required),
                    "Polar angles (two thetas) for detectors. Number of 2theta given must be same as number of histogram.");

    // Vector for Azimuthal angle
    declareProperty(new ArrayProperty<double>("Azimuthal", required),
                    "Azimuthal angles (out-of-plain) for detectors. "
                    "Number of azimuthal angles given must be same as number of histogram.");

    // Detector IDs
    declareProperty(new ArrayProperty<int>("DetectorIDs"), "User specified detector IDs of the spectra. "
                    "Number of specified detector IDs must be either zero or number of histogram");

    // Indicator if it is a new instrument
    declareProperty("NewInstrument", false, "Add detectors information from scratch");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void EditInstrumentGeometry::exec()
  {
    // Process properties
    MatrixWorkspace_sptr workspace = getProperty("Workspace");
    m_L1 = this->getProperty("PrimaryFlightPath");
    const std::vector<int32_t> specids = this->getProperty("SpectrumIDs");
    const std::vector<double> l2s = this->getProperty("L2");
    const std::vector<double> tths = this->getProperty("Polar");
    const std::vector<double> phis = this->getProperty("Azimuthal");
    const vector<int> vec_detids = getProperty("DetectorIDs");
    const bool newinstrument = getProperty("NewInstrument");

    if (vec_detids.size() == 0)
      m_renameDetID = false;
    else
      m_renameDetID = true;

    // Check validity of the input properties
    if (m_L1 != EMPTY_DBL())
    {
      g_log.information() << "Use user-specified L1 = " << m_L1 << "  # Detector = \n";
    }
    else
    {
      g_log.information("Use current instrument's L1 value");
    }

    if (specids.size() != workspace->getNumberHistograms())
    {
      stringstream errss;
      errss << "Number of input spectrum ID/L2/Azimuthal angle/2theta angle (" << specids.size()
            << ") is different from number of spectra (" << workspace->getNumberHistograms()
            << ").  Algorithm is unable to handle this situation. ";
      g_log.error(errss.str());
      throw runtime_error(errss.str());
    }

    if (specids.size() != l2s.size() || l2s.size() != tths.size() || phis.size() != l2s.size())
    {
      // Error: Sizes of input vectors are different.
      stringstream errmsg;
      errmsg << "Input vector for SpectrumID, L2 (Secondary Flight Paths), Polar angles (Two Thetas) "
             << "and Azimuthal have different items number";
      g_log.error(errmsg.str());
      throw std::invalid_argument(errmsg.str());
    }

    if (m_renameDetID && specids.size() != vec_detids.size())
    {
      // DectorIDs are not given right (number)
      stringstream errmsg;
      errmsg << "Input vector for Detector IDs is neither zero nor same number of spectra (" << specids.size()
             << " vs. " << vec_detids.size() << ". ";
      g_log.error(errmsg.str());
      throw std::invalid_argument(errmsg.str());

    }

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

    // Get information from original: L1
    Geometry::Instrument_const_sptr originstrument = workspace->getInstrument();
    if (m_L1 != EMPTY_DBL() && m_L1 > 0.)
    {
      // Use user specified L1 anyway.  Even it can overwrite the original instrument's L1
    }
    else if (newinstrument)
    {
      // User input L1's exception: new instrument but no/invalid L1
      g_log.error() << "For 'NewInstrument', input L1 (" << m_L1 << ") must be given and larger than 0. " << ".\n";
      throw std::invalid_argument("Input L1 is not allowed.");
    }
    else
    {
      // Retrive from current instrumetn
      Kernel::V3D sourcepos = originstrument->getSource()->getPos();
      m_L1 = fabs(sourcepos.Z());
      g_log.information() << "Retrieve L1 from input data workspace. Source position = " << sourcepos.X()
                            << ", " << sourcepos.Y() << ", " << sourcepos.Z() << std::endl;
    }

    // Keep original instrument and set the new instrument, if necessary
    spec2index_map *spec2indexmap = workspace->getSpectrumToWorkspaceIndexMap();

    //  ???  Condition: spectrum has 1 and only 1 detector
    size_t nspec = workspace->getNumberHistograms();

    std::vector<bool> wsindexsetflag(nspec, false);
    std::vector<bool> storable(nspec, false);
    std::vector<double> storL2s(nspec, 0.);
    std::vector<double> stor2Thetas(nspec, 0.);
    std::vector<double> storPhis(nspec, 0.);
    vector<int> storDetIDs(nspec, 0);

    /*
    for (size_t i = 0; i < ; i ++)
    {
      wsindexsetflag.push_back(false);
      storable.push_back(false);
      storL2s.push_back(0.0);
      stor2Thetas.push_back(0.0);
      storPhis.push_back(0.0);
      //storDetids.push_back(0);
    }
    */

    // Map the properties from spectrum ID to workspace index
    for (size_t i = 0; i < specids.size(); i ++)
    {
      // Find spectrum's workspace index
      spec2index_map::iterator it = spec2indexmap->find(specids[i]);
      if (it == spec2indexmap->end())
      {
        stringstream errss;
        errss << "Spectrum ID " << specids[i] << " is not found. "
              << "Instrument won't be edited for this spectrum. " << std::endl;
        g_log.error(errss.str());
        throw std::runtime_error(errss.str());
      }

      // Store and set value
      size_t workspaceindex = it->second;

      wsindexsetflag[workspaceindex] = true;
      storL2s[workspaceindex] = l2s[i];
      stor2Thetas[workspaceindex] = tths[i];
      storPhis[workspaceindex] = phis[i];
      if (m_renameDetID)
        storDetIDs[workspaceindex] = vec_detids[i];

      g_log.debug() << "workspace index = " << workspaceindex << " is for Spectrum " << specids[i] << std::endl;
    }

    // Reset detector information of each spectrum
    for (size_t i = 0; i < workspace->getNumberHistograms(); i ++)
    {
      if (!wsindexsetflag[i])
      {
        // Won't be set
        API::ISpectrum *spectrum = workspace->getSpectrum(i);
        std::set<detid_t> detids = spectrum->getDetectorIDs();
        if (detids.size() == 1)
        {
          // Case: 1 and only 1 detector
          storable[i] = true;

          // a) get DetectorID
          detid_t detid = 0;
          std::set<detid_t>::iterator it;
          for (it=detids.begin(); it!=detids.end(); ++it)
          {
            detid = *it;
          }
          // b) get Detector
          Geometry::IDetector_const_sptr stodet = originstrument->getDetector(detid);
          double rt, thetat, phit;
          stodet->getPos().getSpherical(rt, thetat, phit);

          // c) Store
          //storDetids[i] = detid;
          storL2s[i] = rt;
          stor2Thetas[i] = thetat;
          storPhis[i] = phit;

        }
        else
        {
          // more than 1 detectors
          storable[i] = false;
          if (m_renameDetID)
          {
            // newinstrument = true;
            g_log.notice() << "Spectrum at workspace index " << i << " has more than 1 (" << detids.size()
                           << "detectors.  A new instrument is required to create from sctrach regardless of "
                           << "user's choice." << ".\n";
          }
        }
      }
    }

    // Generate a new instrument
    // Name of the new instrument
    std::string name = "";
    if (newinstrument)
    {
      std::string tempname = getProperty("InstrumentName");
      name += tempname;
    }
    else
    {
      name += originstrument->getName();
    }

    // b) Create a new instrument from scratch any way.
    Geometry::Instrument_sptr instrument(new Geometry::Instrument(name));
    if (instrument.get() == 0)
    {
      delete spec2indexmap;
      stringstream errss;
      errss << "Trying to use a Parametrized Instrument as an Instrument.";
      g_log.error(errss.str());
      throw std::runtime_error(errss.str());
    }

    // c) Source and sample information
    Geometry::ObjComponent *samplepos = new Geometry::ObjComponent("Sample", instrument.get());
    instrument->add(samplepos);
    instrument->markAsSamplePos(samplepos);
    samplepos->setPos(0.0, 0.0, 0.0);

    Geometry::ObjComponent *source = new Geometry::ObjComponent("Source", instrument.get());
    instrument->add(source);
    instrument->markAsSource(source);
    source->setPos(0.0, 0.0, -1.0 * m_L1);

    // d) Set the new instrument
    workspace->setInstrument(instrument);

    // 6. Add or copy detector information
    for (size_t i = 0; i < workspace->getNumberHistograms(); i ++)
    {
      if (wsindexsetflag[i] || storable[i]){
        // a) Create a new detector.
        //    (Instrument will take ownership of pointer so no need to delete.)
        detid_t newdetid;
        if (m_renameDetID)
          newdetid = storDetIDs[i];
        else
          newdetid = detid_t(i)+100;
        Geometry::Detector *detector = new Geometry::Detector("det", newdetid, samplepos);

        // b) Set the new instrument
        double l2 = storL2s[i];
        double tth = stor2Thetas[i];
        double phi = storPhis[i];

        Kernel::V3D pos;
        pos.spherical(l2, tth, phi);
        detector->setPos(pos);

        // c) add copy to instrument and mark it
        API::ISpectrum *spectrum = workspace->getSpectrum(i);
        if (!spectrum)
        {
          stringstream errss;
          errss << "Spectrum ID " << specids[i]
                << " does not exist!  Skip setting detector parameters to this spectrum" << std::endl;
          g_log.error(errss.str());
          throw runtime_error(errss.str());
        }
        else
        {
          g_log.debug() << "Raw: Spectrum " << spectrum->getSpectrumNo()
                        << ": # Detectors =  " << spectrum->getDetectorIDs().size() << std::endl;
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

