#include "MantidAlgorithms/PolarisationCorrection.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ListValidator.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include <algorithm>
#include <boost/shared_ptr.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace
{

  const std::string pNRLabel()
  {
    return "PNR";
  }

  const std::string pALabel()
  {
    return "PA";
  }

  const std::string crhoLabel()
  {
    return "crho";
  }

  const std::string cppLabel()
  {
    return "cPp";
  }

  const std::string cAlphaLabel()
  {
    return "cAlpha";
  }

  const std::string cApLabel()
  {
    return "cAp";
  }

  std::vector<std::string> modes()
  {
    std::vector<std::string> modes;
    modes.push_back(pALabel());
    modes.push_back(pNRLabel());
    return modes;
  }

  Instrument_const_sptr fetchInstrument(WorkspaceGroup const * const groupWS)
  {
    if (groupWS->size() == 0)
    {
      throw std::invalid_argument("Input group workspace has no children.");
    }
    Workspace_sptr firstWS = groupWS->getItem(0);
    MatrixWorkspace_sptr matrixWS = boost::dynamic_pointer_cast<MatrixWorkspace>(firstWS);
    return matrixWS->getInstrument();
  }

  void validateInputWorkspace(WorkspaceGroup_sptr& ws)
  {

    for (size_t i = 0; i < ws->size(); ++i)
    {
      MatrixWorkspace_sptr lastWS;

      Workspace_sptr item = ws->getItem(i);

      if (MatrixWorkspace_sptr ws2d = boost::dynamic_pointer_cast<MatrixWorkspace>(item))
      {

        // X-units check
        auto wsUnit = ws2d->getAxis(0)->unit();
        auto expectedUnit = Units::Wavelength();
        if(wsUnit->unitID() != expectedUnit.unitID())
        {
          throw std::invalid_argument("Input workspaces must have units of Wavelength");
        }

        // More detailed checks based on shape.
        if(lastWS)
        {
          if(lastWS->getNumberHistograms() != ws2d->getNumberHistograms())
          {
            throw std::invalid_argument("Not all workspaces in the InputWorkspace WorkspaceGroup have the same number of spectrum");
          }
          if(lastWS->blocksize() != ws2d->blocksize())
          {
            throw std::invalid_argument("Number of bins do not match between all workspaces in the InputWorkspace WorkspaceGroup");
          }

          auto currentX = ws2d->readX(0);
          auto lastX = lastWS->readX(0);
          if(currentX != lastX)
          {
            throw std::invalid_argument("X-arrays do not match between all workspaces in the InputWorkspace WorkspaceGroup.");
          }
        }

        lastWS = ws2d; //Cache the last workspace so we can use it for comparison purposes.

      }
      else
      {
        std::stringstream messageBuffer;
        messageBuffer << "Item with index: " << i << "in the InputWorkspace is not a MatrixWorkspace";
        throw std::invalid_argument(messageBuffer.str());
      }
    }

  }

  typedef std::vector<double> VecDouble;
}

namespace Mantid
{
  namespace Algorithms
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(PolarisationCorrection)

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    PolarisationCorrection::PolarisationCorrection()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    PolarisationCorrection::~PolarisationCorrection()
    {
    }

    //----------------------------------------------------------------------------------------------
    /// Algorithm's name for identification. @see Algorithm::name
    const std::string PolarisationCorrection::name() const
    {
      return "PolarisationCorrection";
    }
    ;

    /// Algorithm's version for identification. @see Algorithm::version
    int PolarisationCorrection::version() const
    {
      return 1;
    }
    ;

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string PolarisationCorrection::category() const
    {
      return "ISIS//Reflectometry";
    }

    /**
     * @return Return the algorithm summary.
     */
    const std::string PolarisationCorrection::summary() const
    {
      return "Makes corrections for polarization efficiencies of the polarizer and analyzer in a reflectometry neutron spectrometer.";
    }

    MatrixWorkspace_sptr PolarisationCorrection::add(MatrixWorkspace_sptr& lhsWS, const double& rhs)
    {
      auto plus = this->createChildAlgorithm("Plus");
      auto rhsWS = boost::make_shared<DataObjects::WorkspaceSingleValue>(rhs);
      plus->initialize();
      plus->setProperty("LHSWorkspace", lhsWS);
      plus->setProperty("RHSWorkspace", rhsWS);
      plus->execute();
      MatrixWorkspace_sptr outWS = plus->getProperty("OutputWorkspace");
      return outWS;
    }

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
     */
    void PolarisationCorrection::init()
    {
      declareProperty(
          new WorkspaceProperty<Mantid::API::WorkspaceGroup>("InputWorkspace", "", Direction::Input),
          "An input workspace to process.");

      auto propOptions = modes();
      declareProperty("PolarisationAnalysis", "PA", boost::make_shared<StringListValidator>(propOptions),
          "What Polarization mode will be used?\n"
              "PNR: Polarized Neutron Reflectivity mode\n"
              "PA: Full Polarization Analysis PNR-PA");

      VecDouble emptyVec;
      auto mandatoryArray = boost::make_shared<MandatoryValidator<VecDouble> >();

      declareProperty(new ArrayProperty<double>(cppLabel(), mandatoryArray, Direction::Input),
          "Effective polarizing power of the polarizing system. Expressed as a ratio 0 < Pp < 1");

      declareProperty(new ArrayProperty<double>(cApLabel(), mandatoryArray, Direction::Input),
          "Effective polarizing power of the analyzing system. Expressed as a ratio 0 < Ap < 1");

      declareProperty(new ArrayProperty<double>(crhoLabel(), mandatoryArray, Direction::Input),
          "Ratio of efficiencies of polarizer spin-down to polarizer spin-up. This is characteristic of the polarizer flipper. Values are constants for each term in a polynomial expression.");

      declareProperty(new ArrayProperty<double>(cAlphaLabel(), mandatoryArray, Direction::Input),
          "Ratio of efficiencies of analyzer spin-down to analyzer spin-up. This is characteristic of the analyzer flipper. Values are factors for each term in a polynomial expression.");

      declareProperty(
          new WorkspaceProperty<Mantid::API::WorkspaceGroup>("OutputWorkspace", "", Direction::Output),
          "An output workspace.");
    }

    MatrixWorkspace_sptr PolarisationCorrection::execPolynomialCorrection(MatrixWorkspace_sptr& input,
        const VecDouble& coefficients)
    {
      auto polyCorr = this->createChildAlgorithm("PolynomialCorrection");
      polyCorr->initialize();
      polyCorr->setProperty("InputWorkspace", input);
      polyCorr->setProperty("Coefficients", coefficients);
      polyCorr->execute();
      MatrixWorkspace_sptr corrected = polyCorr->getProperty("OutputWorkspace");
      return corrected;
    }

    WorkspaceGroup_sptr PolarisationCorrection::execPA(WorkspaceGroup_sptr inWS)
    {

      size_t itemIndex = 0;
      MatrixWorkspace_sptr Ipp = boost::dynamic_pointer_cast<MatrixWorkspace>(
          inWS->getItem(itemIndex++));
      MatrixWorkspace_sptr Iaa = boost::dynamic_pointer_cast<MatrixWorkspace>(
          inWS->getItem(itemIndex++));
      MatrixWorkspace_sptr Ipa = boost::dynamic_pointer_cast<MatrixWorkspace>(
          inWS->getItem(itemIndex++));
      MatrixWorkspace_sptr Iap = boost::dynamic_pointer_cast<MatrixWorkspace>(
          inWS->getItem(itemIndex++));

      MatrixWorkspace_sptr ones = WorkspaceFactory::Instance().create(Iaa);
      // Copy the x-array across to the new workspace.
      for (size_t i = 0; i < Iaa->getNumberHistograms(); ++i)
      {
        ones->setX(i, Iaa->readX(i));
      }
      ones = this->add(ones, 1.0);
      // The ones workspace is now identical to the input workspaces in x, but has 1 as y values. It can therefore be used to build real polynomial functions.

      const VecDouble c_rho = getProperty(crhoLabel());
      const VecDouble c_alpha = getProperty(cAlphaLabel());
      const VecDouble c_pp = getProperty(cppLabel());
      const VecDouble c_ap = getProperty(cApLabel());

      const auto rho = this->execPolynomialCorrection(ones, c_rho); // Execute polynomial expression
      const auto pp = this->execPolynomialCorrection(ones, c_pp); // Execute polynomial expression
      const auto alpha = this->execPolynomialCorrection(ones, c_alpha); // Execute polynomial expression
      const auto ap = this->execPolynomialCorrection(ones, c_ap); // Execute polynomial expression

      const auto A0 = Iaa * pp + ap * Ipa * rho * pp + ap * Iap * pp * alpha
          + Ipp * ap * alpha * rho * pp;
      const auto A1 = pp * Iaa;
      const auto A2 = pp * Iap;
      const auto A3 = ap * Iaa;
      const auto A4 = ap * Ipa;
      const auto A5 = ap * alpha * Ipp;
      const auto A6 = ap * alpha * Iap;
      const auto A7 = pp * rho * Ipp;
      const auto A8 = pp * rho * Ipa;

      const auto D = pp * ap * (rho + alpha + 1.0 + rho * alpha);

      const auto nIpp = (A0 - A1 + A2 - A3 + A4 + A5 - A6 + A7 - A8 + Ipp + Iaa - Ipa - Iap) / D;
      const auto nIaa = (A0 + A1 - A2 + A3 - A4 - A5 + A6 - A7 + A8 + Ipp + Iaa - Ipa - Iap) / D;
      const auto nIpa = (A0 - A1 + A2 + A3 - A4 - A5 + A6 + A7 - A8 - Ipp - Iaa + Ipa + Iap) / D;
      const auto nIap = (A0 + A1 - A2 - A3 + A4 + A5 - A6 - A7 + A8 - Ipp - Iaa + Ipa + Iap) / D;

      WorkspaceGroup_sptr dataOut = boost::make_shared<WorkspaceGroup>();
      dataOut->addWorkspace(nIpp);
      dataOut->addWorkspace(nIaa);
      dataOut->addWorkspace(nIpa);
      dataOut->addWorkspace(nIap);

      return dataOut;
    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
     */
    void PolarisationCorrection::exec()
    {
      WorkspaceGroup_sptr inWS = getProperty("InputWorkspace");
      const std::string analysisMode = getProperty("PolarisationAnalysis");
      const size_t nWorkspaces = inWS->size();

      validateInputWorkspace(inWS);

      Instrument_const_sptr instrument = fetchInstrument(inWS.get());

      WorkspaceGroup_sptr outWS;
      if (analysisMode == pALabel())
      {
        if (nWorkspaces != 4)
        {
          throw std::invalid_argument("For PA analysis, input group must have 4 periods.");
        }
        outWS = execPA(inWS);
      }
      else if (analysisMode == pNRLabel())
      {
        if (nWorkspaces != 2)
        {
          throw std::invalid_argument("For PNR analysis, input group must have 2 periods.");
        }
        throw std::runtime_error("PNR not implemented.");
      }
      this->setProperty("OutputWorkspace", outWS);
    }

  } // namespace Algorithms
} // namespace Mantid
