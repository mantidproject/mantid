/*WIKI*



 *WIKI*/
#include "MantidCurveFitting/CalculateGammaBackground.h"

#include "MantidAPI/WorkspaceValidators.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MandatoryValidator.h"

#include "MantidGeometry/Instrument/ReferenceFrame.h"

namespace Mantid
{
  namespace CurveFitting
  {
    // Subscription
    DECLARE_ALGORITHM(CalculateGammaBackground);

    //--------------------------------------------------------------------------------------------------------
    // Public members
    //--------------------------------------------------------------------------------------------------------

    /// Default constructor
    CalculateGammaBackground::CalculateGammaBackground()
      : Algorithm(), m_inputWS(), m_npeaks(0), m_masses(), m_amplitudes(), m_widths(),
        m_l1(0.0), m_foilRadius(0.0), m_foilBeamMin(0.0),m_foilBeamMax(0.0),
        m_backgroundWS(), m_correctedWS()
    {
    }

    //--------------------------------------------------------------------------------------------------------
    // Private members
    //--------------------------------------------------------------------------------------------------------

    const std::string CalculateGammaBackground::name() const
    {
      return "CalculateGammaBackground";
    }

    int CalculateGammaBackground::version() const
    {
      return 1;
    }

    const std::string CalculateGammaBackground::category() const
    {
      return "CorrectionFunctions";
    }

    void CalculateGammaBackground::initDocs()
    {
      this->setWikiSummary("Calculates and the background due to gamma rays produced when neutrons are absorbed by shielding");
      this->setOptionalMessage("Calculates the background due to gamma rays produced when neutrons are absorbed by shielding.");
    }

    void CalculateGammaBackground::init()
    {
      using namespace API;
      using namespace Kernel;

      declareProperty(new WorkspaceProperty<>("InputWorkspace", "", Direction::Input,
                                              boost::make_shared<WorkspaceUnitValidator>("TOF")),
                      "An input workspace containing TOF data");

      // Peak information
      const std::string grpName = "Peaks";
      typedef MandatoryValidator<std::vector<double>> NonEmptyDoubleArray;
      declareProperty(new ArrayProperty<double>("Masses",boost::make_shared<NonEmptyDoubleArray>()),
                      "List of mass values for each peak in the input spectrum in atomic mass units");
      setPropertyGroup("Masses", grpName);
      declareProperty(new ArrayProperty<double>("PeakAmplitudes",boost::make_shared<NonEmptyDoubleArray>()),
                      "List of amplitudes of the peaks in the same order as the Masses property");
      setPropertyGroup("PeakAmplitudes", grpName);
      declareProperty(new ArrayProperty<double>("PeakWidths",boost::make_shared<NonEmptyDoubleArray>()),
                      "List of std deviations of peak widths in the same order as the Masses property");
      setPropertyGroup("PeakWidths", grpName);

      declareProperty(new WorkspaceProperty<>("BackgroundWorkspace", "", Direction::Output));
      declareProperty(new WorkspaceProperty<>("CorrectedWorkspace", "", Direction::Output));
    }

    void CalculateGammaBackground::exec()
    {
      retrieveInputs();
      createOutputWorkspaces();

      const size_t nhist = m_inputWS->getNumberHistograms();
      for(size_t i = 0; i < nhist; ++i)
      {
        applyCorrection(i);
      }

      setProperty("BackgroundWorkspace",m_backgroundWS);
      setProperty("CorrectedWorkspace",m_correctedWS);
    }

    /**
     * Caches input details for the peak information
     */
    void CalculateGammaBackground::retrieveInputs()
    {
      m_inputWS = getProperty("InputWorkspace");
      m_masses = getProperty("Masses");
      m_amplitudes = getProperty("PeakAmplitudes");
      m_widths = getProperty("PeakWidths");
      m_npeaks = m_masses.size();

      if(m_masses.size() != m_amplitudes.size() || m_masses.size() != m_widths.size())
      {
        std::ostringstream os;
        os << "Invalid peak inputs: No. of masses=" << m_masses.size()
           << ", No. of amplitudes=" << m_amplitudes.size() << ", No. of peaks=" << m_widths.size();
        throw std::invalid_argument(os.str());
      }

      cacheInstrumentGeometry();
    }

    /**
     */
    void CalculateGammaBackground::cacheInstrumentGeometry()
    {
      auto inst = m_inputWS->getInstrument();
      auto refFrame = inst->getReferenceFrame();
      auto beamAxis = refFrame->pointingAlongBeam();
      auto source = inst->getSource();
      auto sample = inst->getSample();
      m_l1 = sample->getPos().distance(source->getPos());

      // foils
      auto changer = boost::dynamic_pointer_cast<const Geometry::IObjComponent>(inst->getComponentByName("foil-changer"));
      if(!changer)
      {
        throw std::invalid_argument("Input workspace has no component named foil-changer. "
                                    "One is required to define integration area.");
      }
      // 'radius' of shape sets limits in beam direction
      const auto boundBox = changer->shape()->getBoundingBox();
      m_foilBeamMin = boundBox.minPoint()[beamAxis];
      m_foilBeamMax = boundBox.maxPoint()[beamAxis];

      g_log.information() << "Instrument geometry:\n"
                          << "  l1 = " << m_l1 << "m\n"
                          << "  foil radius = " << m_foilRadius << "\n"
                          << "  foil integration min = " << m_foilBeamMin << "\n"
                          << "  foil integration max = " << m_foilBeamMax << "\n";
    }

    /**
     * Create & cache output workspaces
     */
    void CalculateGammaBackground::createOutputWorkspaces()
    {
      using namespace API;
      m_backgroundWS = WorkspaceFactory::Instance().create(m_inputWS);
      m_correctedWS = WorkspaceFactory::Instance().create(m_inputWS);
    }

    /**
     * Calculate & apply gamma correction for the given index of the
     * input workspace
     * @param index A workspace index that defines the spectrum to correct
     */
    void CalculateGammaBackground::applyCorrection(const size_t)
    {

    }

  } // namespace CurveFitting
} // namespace Mantid
