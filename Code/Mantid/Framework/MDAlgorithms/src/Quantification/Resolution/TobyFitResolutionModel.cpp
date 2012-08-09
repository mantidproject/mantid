// Includes
#include "MantidMDAlgorithms/Quantification/Resolution/TobyFitResolutionModel.h"
#include "MantidMDAlgorithms/Quantification/Resolution/ModeratorChopperResolution.h"
#include "MantidMDAlgorithms/Quantification/CachedExperimentInfo.h"

#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/CPUTimer.h"
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
      const char * CRYSTAL_MOSAIC = "CrystalMosaic";
    }

    /*
     * Default constructor
     */
    TobyFitResolutionModel::TobyFitResolutionModel()
      : MDResolutionConvolution(), m_randGen(new Kernel::SobolSequence(TobyFitYVector::variableCount() + 2)), // For eta
        m_activeAttrValue(1),
        m_mcLoopMin(100), m_mcLoopMax(1000), m_mcRelErrorTol(1e-5), m_mosaicActive(1),
        m_bmatrix(), m_yvector(), m_etaInPlane(), m_etaOutPlane(), m_deltaQE(),
        m_exptCache()
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
        m_mcLoopMin(100), m_mcLoopMax(1000), m_mcRelErrorTol(1e-5), m_mosaicActive(1),
        m_bmatrix(), m_yvector(), m_etaInPlane(), m_etaOutPlane(), m_deltaQE(),
        m_exptCache()
    {
    }

    /**
     * Destroy the object deleting the observation cache
     */
    TobyFitResolutionModel::~TobyFitResolutionModel()
    {
      auto iter = m_exptCache.begin();
      while(iter != m_exptCache.end())
      {
        delete iter->second; // Delete the observation itself
        m_exptCache.erase(iter++);// Post-increment to return old iterator to remove item from map
      }
    }

    /**
     * Returns the value of the cross-section convoluted with the resolution an event. This assumes that
     * the box forms a 4D point with axes: Qx, Qy, Qz, \f$\DeltaE\f$
     * @param box :: An iterator pointing at the current box under examination
     * @param eventIndex :: An index of the current event in the box
     * @param innerRunIndex :: An index of the current run within the workspace. This is NOT the run number. The experiment
     * can be retrieved using box->getExperimentInfo(innerRunIndex)
     * @returns the cross-section convoluted with the resolution
     */
    double TobyFitResolutionModel::signal(const API::IMDIterator & box, const uint16_t innerRunIndex,
                                          const size_t eventIndex) const
    {
      auto iter = m_exptCache.find(std::make_pair(innerRunIndex, box.getInnerDetectorID(eventIndex))); // Guaranteed to exist
      const CachedExperimentInfo & detCachedExperimentInfo = *(iter->second);
      QOmegaPoint qOmega(box, eventIndex);

      // Calculate the matrix of coefficients that contribute to the resolution function (the B matrix in TobyFit).
      calculateResolutionCoefficients(detCachedExperimentInfo, qOmega);

      // Start MC loop and check the relative error every
      // min steps
      double sumSigma(0.0), sumSigmaSqr(0.0), avgSigma(0.0);
      for(int step = 1; step <= m_mcLoopMax; ++step)
      {
        generateIntegrationVariables(detCachedExperimentInfo, qOmega);
        calculatePerturbedQE(detCachedExperimentInfo, qOmega);
        // Compute weight from the foreground at this point
        const double weight = foregroundModel().scatteringIntensity(detCachedExperimentInfo.experimentInfo(),
                                                                    m_deltaQE[PARALLEL_THREAD_NUMBER]);

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
     * Sets up the function to cope with the given number of threads processing it at once
     * @param nthreads :: The maximum number of threads that will be used to evaluate the
     * function
     */
    void TobyFitResolutionModel::useNumberOfThreads(const int nthreads)
    {
      m_bmatrix = std::vector<TobyFitBMatrix>(nthreads, TobyFitBMatrix());
      m_yvector = std::vector<TobyFitYVector>(nthreads, TobyFitYVector());
      m_etaInPlane = std::vector<double>(nthreads, 0.0);
      m_etaOutPlane = std::vector<double>(nthreads, 0.0);
      m_deltaQE = std::vector<std::vector<double>>(nthreads, std::vector<double>(4, 0.0));
    }

    /**
     * Called before any fit/simulation is started to allow caching of
     * frequently used parameters
     * @param workspace :: The MD that will be used for the fit
     */
    void TobyFitResolutionModel::preprocess(const API::IMDEventWorkspace_const_sptr & workspace)
    {
      Kernel::CPUTimer timer;
      // Fill the observation cache
      auto iterator = workspace->createIterator();
      g_log.debug() << "Starting preprocessing loop\n";
      do
      {
        const size_t nevents = iterator->getNumEvents();
        for(size_t i = 0; i < nevents; ++i)
        {
          uint16_t innerRunIndex = iterator->getInnerRunIndex(i);
          detid_t detID = iterator->getInnerDetectorID(i);
          const auto key = std::make_pair(innerRunIndex, detID);
          if(m_exptCache.find(key) == m_exptCache.end())
          {
            API::ExperimentInfo_const_sptr expt = workspace->getExperimentInfo(innerRunIndex);
            m_exptCache.insert(std::make_pair(key, new CachedExperimentInfo(*expt, detID)));
          }
        }
      }
      while(iterator->next());
      g_log.debug() << "Done preprocessing loop:" << timer.elapsedWallClock() << " seconds\n";
      delete iterator;
    }

    /**
     * Resets the random number generator ready for the next call.
     * This ensures that each evaluation of the function during fitting gets the
     * same set of random numbers
     */
    void TobyFitResolutionModel::functionEvalFinished() const
    {
      m_randGen->restart();
    }

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
      declareAttribute(CRYSTAL_MOSAIC, IFunction::Attribute(m_activeAttrValue));

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
      else if(name == CRYSTAL_MOSAIC) m_mosaicActive = value.asInt();
      else
      {
        for(auto iter = m_yvector.begin(); iter != m_yvector.end(); ++iter)
        {
          iter->setAttribute(name, value.asInt());
        }
      }
    }

    /**
     * Ensure the run parameters are up to date. Gets the values of the current parameters
     * from the fit
     * @param observation :: A reference to the current observation
     */
    void TobyFitResolutionModel::updateRunParameters(const CachedExperimentInfo & observation) const
    {
      UNUSED_ARG(observation);
    }

    /**
     * Calculate the values of the resolution coefficients from pg 112 of T.G.Perring's thesis. It maps the
     * vector of randomly generated integration points to a vector of resolution coefficients
     * @param observation :: The current observation defining the point experimental setup
     * @param eventPoint :: The point in QE space that this refers to
     */
    void TobyFitResolutionModel::calculateResolutionCoefficients(const CachedExperimentInfo & observation,
                                                                 const QOmegaPoint & eventPoint) const
    {
      m_bmatrix[PARALLEL_THREAD_NUMBER].recalculate(observation, eventPoint);
    }

    /**
     * Generates the Y vector of random deviates
     * @param observation :: The current observation defining the point experimental setup
     * @param eventPoint :: The point in QE space that this refers to
     */
    void TobyFitResolutionModel::generateIntegrationVariables(const CachedExperimentInfo & observation,
                                      const QOmegaPoint & eventPoint) const
    {
      const std::vector<double> & randomNums = m_randGen->nextPoint();
      const size_t nvars = m_yvector[PARALLEL_THREAD_NUMBER].recalculate(randomNums, observation, eventPoint);

      // Calculate crystal mosaic contribution
      if(m_mosaicActive)
      {
        static const double small(1e-20);
        const double prefactor = std::sqrt(-2.0*std::log(std::max(small,randomNums[nvars])));
        const double r2 = randomNums[nvars+1];
        const double etaSig = observation.experimentInfo().run().getLogAsSingleValue("eta_sigma");

        m_etaInPlane[PARALLEL_THREAD_NUMBER] = etaSig*prefactor*std::cos(2.0*M_PI*r2);
        m_etaOutPlane[PARALLEL_THREAD_NUMBER] = etaSig*prefactor*std::sin(2.0*M_PI*r2);
      }
      else
      {
        m_etaInPlane[PARALLEL_THREAD_NUMBER] = m_etaOutPlane[PARALLEL_THREAD_NUMBER] = 0.0;
      }
    }

    /**
     * Calculates the point in Q-E space where the foreground model will be evaluated.
     * @param observation :: The current observation defining the point experimental setup
     * @param eventPoint :: The point in QE space that this refers to
     */
    void TobyFitResolutionModel::calculatePerturbedQE(const CachedExperimentInfo & observation,const QOmegaPoint & qOmega) const
    {
      // -- Calculate components of dKi & dKf, essentially X vector in TobyFit --

      /**
       * This function is called for each iteration of the Monte Carlo loop
       * and using the standard matrix algebra will produce a lot of repeated memory
       * allocations for creating new vectors/matrices.

       * The manual computation of the matrix multiplication is to avoid this overhead.
       */

      double xVec0(0.0), xVec1(0.0), xVec2(0.0),
          xVec3(0.0), xVec4(0.0), xVec5(0.0);
      const std::vector<double> & yvalues = m_yvector[PARALLEL_THREAD_NUMBER].values();
      const TobyFitBMatrix & bmatrix = m_bmatrix[PARALLEL_THREAD_NUMBER];
      for(unsigned int i =0; i < TobyFitYVector::variableCount(); ++i)
      {
        xVec0 += bmatrix[0][i] * yvalues[i];
        xVec1 += bmatrix[1][i] * yvalues[i];
        xVec2 += bmatrix[2][i] * yvalues[i];
        xVec3 += bmatrix[3][i] * yvalues[i];
        xVec4 += bmatrix[4][i] * yvalues[i];
        xVec5 += bmatrix[5][i] * yvalues[i];
      }

      // Convert to dQ in lab frame
      // dQ = L*Xf, L = ((labToDet)^1)^T, Xf = outgoing components of X vector
      const Kernel::DblMatrix & D =  observation.labToDetectorTransform();
      const double D00(D[0][0]), D01(D[0][1]), D02(D[0][2]),
                   D10(D[1][0]), D11(D[1][1]), D12(D[1][2]),
                   D20(D[2][0]), D21(D[2][1]), D22(D[2][2]);
      const double determinant = D00*(D11*D22 - D12*D21) -
                                 D01*(D10*D22 - D12*D20) +
                                 D02*(D10*D21 - D11*D20);
      const double L00(D11*D22 - D12*D21), L01(D12*D20 - D10*D22), L02(D10*D21 - D11*D20),
                   L10(D02*D21 - D01*D22), L11(D00*D22 - D02*D20), L12(D20*D01 - D00*D21),
                   L20(D01*D12 - D02*D11), L21(D02*D10 - D00*D12), L22(D00*D11 - D01*D10);
      
      const double dqlab0 = (L00*xVec3 + L01*xVec4 + L02*xVec5)/determinant;
      const double dqlab1 = (L10*xVec3 + L11*xVec4 + L12*xVec5)/determinant;
      const double dqlab2 = (L20*xVec3 + L21*xVec4 + L22*xVec5)/determinant;
      std::vector<double> & deltaQE = m_deltaQE[PARALLEL_THREAD_NUMBER];
      deltaQE[0] = (xVec0 - dqlab0);
      deltaQE[1] = (xVec1 - dqlab1);
      deltaQE[2] = (xVec2 - dqlab2);

      const double efixed = observation.getEFixed();
      const double wi = std::sqrt(efixed/PhysicalConstants::E_mev_toNeutronWavenumberSq);
      const double wf = std::sqrt((efixed - qOmega.deltaE)/PhysicalConstants::E_mev_toNeutronWavenumberSq);
      deltaQE[3] = 4.1442836 * (wi*xVec2 - wf*xVec5);

      // Add on the nominal Q
      deltaQE[0] += qOmega.qx;
      deltaQE[1] += qOmega.qy;
      deltaQE[2] += qOmega.qz;
      deltaQE[3] += qOmega.deltaE;

      if(m_mosaicActive)
      {
        const double etaInPlane = m_etaInPlane[PARALLEL_THREAD_NUMBER];
        const double etaOutPlane = m_etaOutPlane[PARALLEL_THREAD_NUMBER];
        const double qx(deltaQE[0]),qy(deltaQE[1]),qz(deltaQE[1]);
        const double qipmodSq = qy*qy + qz*qz;
        const double qmod = std::sqrt(qx*qx + qipmodSq);
        static const double small(1e-10);
        if(qmod > small)
        {
          const double qipmod = std::sqrt(qipmodSq);
          if(qipmod > small)
          {
            deltaQE[0] -= qipmod*etaInPlane;
            deltaQE[1] += ((qx*qy)*etaInPlane - (qz*qmod)*etaOutPlane)/qipmod;
            deltaQE[2] += ((qx*qz)*etaInPlane + (qy*qmod)*etaOutPlane)/qipmod;
          }
          else
          {
            deltaQE[1] += qmod*etaInPlane;
            deltaQE[2] += qmod*etaOutPlane;
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


