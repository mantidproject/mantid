// Includes
#include "MantidMDAlgorithms/Quantification/Resolution/TobyFitResolutionModel.h"
#include "MantidMDAlgorithms/Quantification/Resolution/ModeratorChopperResolution.h"
#include "MantidMDAlgorithms/Quantification/Observation.h"

#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

#include "MantidKernel/SobolSequence.h"


namespace Mantid
{
  namespace MDAlgorithms
  {
    using Geometry::Instrument;
    using Geometry::Instrument_const_sptr;
    using Geometry::IObjComponent_const_sptr;
    using Geometry::IDetector_const_sptr;
    using API::Run;
    using API::IFunction;

    DECLARE_MDRESOLUTIONCONVOLUTION(TobyFitResolutionModel, "TobyFitResolutionModel");

    namespace // anonymous
    {
      /// Map strings to attributes names
      const char * MC_MIN_NAME = "MCLoopMin";
      const char * MC_MAX_NAME = "MCLoopMax";
      const char * MC_LOOP_TOL = "MCTolerance";
    }

    /*
     * Default constructor
     */
    TobyFitResolutionModel::TobyFitResolutionModel()
      : MDResolutionConvolution(), m_randGen(new Kernel::SobolSequence(TobyFitYVector::variableCount() + 2)), // For eta
        m_activeAttrValue(1),
        m_mcLoopMin(100), m_mcLoopMax(1000), m_mcRelErrorTol(1e-5),
        m_bmatrix(), m_yvector(*this), m_etaInPlane(0.0), m_etaOutPlane(0.0), m_deltaQE(4, 0.0)
    {
    }

    /**
     *  Construct with a model pointer
     *  @param fittedFunction :: A pointer to the function being fitted
     *  @param fgModel :: A pointer to a foreground model
     */
    TobyFitResolutionModel::TobyFitResolutionModel(const API::IFunctionMD & fittedFunction,
                                                                     const std::string & fgModel)
      : MDResolutionConvolution(fittedFunction, fgModel), m_randGen(new Kernel::SobolSequence(TobyFitYVector::variableCount())),
        m_activeAttrValue(1),
        m_mcLoopMin(100), m_mcLoopMax(1000), m_mcRelErrorTol(1e-5),
        m_bmatrix(),
        m_yvector(*this)
    {
    }

    /**
     * Returns true if the given attribute is active.
     * @param variable A TobyFitYVectoor::Variable enum
     * @return Boolean indicating if the attribute should be used
     */
    bool TobyFitResolutionModel::useAttribute(const unsigned int variable) const
    {
      return (getAttribute(TobyFitYVector::identifier(variable)).asInt() == m_activeAttrValue);
    }

    /**
     * Returns true if the given attribute is active.
     * @param attr :: A string name defining the attribute
     * @return Boolean indicating if the attribute should be used
     */
    bool TobyFitResolutionModel::useAttribute(const std::string & attr) const
    {
      return (getAttribute(attr).asInt() == m_activeAttrValue);
    }

    /**
     * Returns the value of the cross-section convoluted with the resolution an event. This assumes that
     * the box forms a 4D point with axes: Qx, Qy, Qz, \f$\DeltaE\f$
     * @param box :: An iterator pointing at the current box under examination
     * @param eventIndex :: An index of the current pixel in the box
     * @param experimentInfo :: A pointer to the experimental run for this point
     * @returns the cross-section convoluted with the resolution
     */
    double TobyFitResolutionModel::signal(const API::IMDIterator & box, const size_t eventIndex,
                                                   API::ExperimentInfo_const_sptr experimentInfo) const
    {
      const detid_t detID = box.getInnerDetectorID(eventIndex);
      Observation detObservation(experimentInfo, detID);
      QOmegaPoint qOmega(box, eventIndex);

//      // Moderator-chopper broadening for sharp models ??
//      ModeratorChopperResolution moderatorChopper(detObservation);
//
//      const double energySigma = moderatorChopper.energyWidth(qOmega.deltaE);
//      const double gaussianHeight = 1.0/(0.1*energySigma*std::sqrt(2.0*M_PI));
//      const double gaussianArg = 0.5/std::pow((0.1*energySigma),2);

      // Calculate the matrix of coefficients that contribute to the resolution function (the B matrix in TobyFit).
      calculateResolutionCoefficients(detObservation, qOmega);

      // Start MC loop and check the relative error every
      // min steps
      double sumSigma(0.0), sumSigmaSqr(0.0), avgSigma(0.0);
      API::ExperimentInfo_const_sptr exptSetup = detObservation.experimentInfo();
      for(int step = 1; step <= m_mcLoopMax; ++step)
      {
        generateIntegrationVariables(detObservation, qOmega);
        calculatePerturbedQE(detObservation, qOmega);
        // Compute weight from the foreground at this point
        const double weight = foregroundModel().scatteringIntensity(*exptSetup, m_deltaQE);

        // Add on this contribution to the average
        sumSigma += weight;
        sumSigmaSqr += weight*weight;
        avgSigma = sumSigma/step;
        if(checkForConvergence(step) && hasConverged(step, sumSigma, sumSigmaSqr, avgSigma))
        {
          break;
        }
      }

      return avgSigma;
    }

    //---------------------------------------------------------------------------------
    // Private members
    //---------------------------------------------------------------------------------

    /**
     * Declare function attributes
     */
    void TobyFitResolutionModel::declareAttributes()
    {
      // Resolution attributes, all on by default
      for(unsigned int i = 0; i < TobyFitYVector::variableCount(); ++i)
      {
        declareAttribute(TobyFitYVector::identifier(i), IFunction::Attribute(m_activeAttrValue));
      }
      // Crystal mosaic
      declareAttribute("CrystalMosaic", IFunction::Attribute(m_activeAttrValue));

      declareAttribute(MC_MIN_NAME, API::IFunction::Attribute(m_mcLoopMin));
      declareAttribute(MC_MAX_NAME, API::IFunction::Attribute(m_mcLoopMax));
      declareAttribute(MC_LOOP_TOL, API::IFunction::Attribute(m_mcRelErrorTol));
    }

    /**
     *  Declare possible fitting parameters that will vary as fit progresses
     */
    void TobyFitResolutionModel::declareParameters()
    {
    }

    /**
     * Cache some frequently used attributes
     * @param name :: The name of the attribute
     * @param value :: It's value
     */
    void TobyFitResolutionModel::setAttribute(const std::string& name,
                                              const API::IFunction::Attribute & value)
    {
      MDResolutionConvolution::setAttribute(name, value);
      if(name == MC_MIN_NAME) m_mcLoopMin = value.asInt();
      else if(name == MC_MAX_NAME) m_mcLoopMax = value.asInt();
      else if(name == MC_LOOP_TOL) m_mcRelErrorTol = value.asDouble();
      else {}
    }

    /**
     * Ensure the run parameters are up to date. Gets the values of the current parameters
     * from the fit
     * @param observation :: A reference to the current observation
     */
    void TobyFitResolutionModel::updateRunParameters(const Observation & observation) const
    {
      UNUSED_ARG(observation);
    }

    /**
     * Calculate the values of the resolution coefficients from pg 112 of T.G.Perring's thesis. It maps the
     * vector of randomly generated integration points to a vector of resolution coefficients
     * @param observation :: The current observation defining the point experimental setup
     * @param eventPoint :: The point in QE space that this refers to
     */
    void TobyFitResolutionModel::calculateResolutionCoefficients(const Observation & observation,
                                                                 const QOmegaPoint & eventPoint) const
    {
      m_bmatrix.recalculate(observation, eventPoint);
    }

    /**
     * Generates the Y vector of random deviates
     * @param observation :: The current observation defining the point experimental setup
     * @param eventPoint :: The point in QE space that this refers to
     */
    void TobyFitResolutionModel::generateIntegrationVariables(const Observation & observation,
                                      const QOmegaPoint & eventPoint) const
    {
      const std::vector<double> & randomNums = m_randGen->nextPoint();
      const size_t nvars = m_yvector.recalculate(randomNums, observation, eventPoint);

      // Calculate crystal mosaic contribution
      if(useAttribute("CrystalMosaic"))
      {
        static const double small(1e-20);
        const double prefactor = std::sqrt(-2.0*std::log(std::max(small,randomNums[nvars])));
        const double r2 = randomNums[nvars+1];
        const double etaSig = observation.experimentInfo()->run().getLogAsSingleValue("eta_sigma");

        m_etaInPlane = etaSig*prefactor*std::cos(2.0*M_PI*r2);
        m_etaOutPlane = etaSig*prefactor*std::sin(2.0*M_PI*r2);
      }
      else
      {
        m_etaInPlane = m_etaOutPlane = 0.0;
      }
    }

    /**
     * Calculates the point in Q-E space where the foreground model will be evaluated.
     * @param observation :: The current observation defining the point experimental setup
     * @param eventPoint :: The point in QE space that this refers to
     */
    void TobyFitResolutionModel::calculatePerturbedQE(const Observation & observation,const QOmegaPoint & qOmega) const
    {
      // Calculate components of dKi & dKf, essentially X vector in TobyFit
      const std::vector<double> xVector = m_bmatrix * m_yvector.values();

      // Convert to dQ in lab frame
      Kernel::DblMatrix labToDetInv =  observation.labToDetectorTransform();
      labToDetInv.Invert();
      std::vector<double> xVectorKf(3,0.0);
      xVectorKf[0] = xVector[3];
      xVectorKf[1] = xVector[4];
      xVectorKf[2] = xVector[5];
      const std::vector<double> dqInLab = labToDetInv.Tprime()*xVectorKf;
      m_deltaQE[0] = (xVector[0] - dqInLab[0]);
      m_deltaQE[1] = (xVector[1] - dqInLab[1]);
      m_deltaQE[2] = (xVector[2] - dqInLab[2]);

      const double efixed = observation.getEFixed();
      const double wi = std::sqrt(efixed/PhysicalConstants::E_mev_toNeutronWavenumberSq);
      const double wf = std::sqrt((efixed - qOmega.deltaE)/PhysicalConstants::E_mev_toNeutronWavenumberSq);
      m_deltaQE[3] = 4.1442836 * (wi*xVector[2] - wf*xVector[5]);

      // Add on the nominal Q
      m_deltaQE[0] += qOmega.qx;
      m_deltaQE[1] += qOmega.qy;
      m_deltaQE[2] += qOmega.qz;
      m_deltaQE[3] += qOmega.deltaE;

      if(useAttribute("CrystalMosaic"))
      {
        const double qx(m_deltaQE[0]),qy(m_deltaQE[1]),qz(m_deltaQE[1]);
        const double qipmodSq = qy*qy + qz*qz;
        const double qmod = std::sqrt(qx*qx + qipmodSq);
        static const double small(1e-10);
        if(qmod > small)
        {
          const double qipmod = std::sqrt(qipmodSq);
          if(qipmod > small)
          {
            m_deltaQE[0] -= qipmod*m_etaInPlane;
            m_deltaQE[1] += ((qx*qy)*m_etaInPlane - (qz*qmod)*m_etaOutPlane)/qipmod;
            m_deltaQE[2] += ((qx*qz)*m_etaInPlane + (qy*qmod)*m_etaOutPlane)/qipmod;
          }
          else
          {
            m_deltaQE[1] += qmod*m_etaInPlane;
            m_deltaQE[2] += qmod*m_etaOutPlane;
          }
        }
      }
    }

    /**
     * Return true if it is time to check for convergence of the
     * current sigma value
     * @param step :: The current step value
     * @return True if it is time to check for convergence
     */
    bool TobyFitResolutionModel::checkForConvergence(const int step) const
    {
      return (step % m_mcLoopMin) == 0 || step == m_mcLoopMax;
    }

    /**
     * Returns true if the Monte Carlo loop should be broken. This occurs when either the relative
     * error satisfies the tolerance or simulated value is zero after the minimum number of steps
     *
     * @param step
     * @param sumSigma :: The current value of the sum of \f$\sigma\f$
     * @param sumSigmaSqr :: The current value of the sum of \f$\sigma^2\f$
     * @param avgSigma :: The current value of the monte carlo estimate of the true \f$\sigma\f$ value,
     *                    i.e. \f$\sigma/step\f$
     * @return True if the sum has converged
     */
    bool TobyFitResolutionModel::hasConverged(const int step, const double sumSigma,
                                                       const double sumSigmaSqr, const double avgSigma) const
    {
      static const double smallValue(1e-10);
      const double error = std::sqrt(std::fabs((sumSigmaSqr/step) - std::pow(sumSigma/step, 2))/step);
      if(std::fabs(avgSigma) > smallValue)
      {
        const double relativeError = error/avgSigma;
        if(relativeError < m_mcRelErrorTol)
        {
          return true;
        }
      }
      else
      {
        // Value is considered zero after min number of steps
        // Probably as converged as we will get
        return true;
      }
      return false;
    }

  }
}


