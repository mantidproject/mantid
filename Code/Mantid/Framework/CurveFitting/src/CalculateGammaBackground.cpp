/*WIKI*



 *WIKI*/
#include "MantidCurveFitting/CalculateGammaBackground.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MandatoryValidator.h"


namespace Mantid
{
  namespace CurveFitting
  {
    //--------------------------------------------------------------------------------------------------------
    // Public members
    //--------------------------------------------------------------------------------------------------------

    /// Default constructor
    CalculateGammaBackground::CalculateGammaBackground()
      : Algorithm(), m_inputWS(), m_npeaks(0), m_masses(), m_amplitudes(), m_widths(),
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
    void CalculateGammaBackground::applyCorrection(const size_t index)
    {

    }

  } // namespace CurveFitting
} // namespace Mantid
