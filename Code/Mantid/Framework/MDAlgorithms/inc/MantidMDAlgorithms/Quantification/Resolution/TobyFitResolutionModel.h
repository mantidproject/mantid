#ifndef MANTID_MDALGORITHMS_TOBYFITRESOLUTIONMODEL_H_
#define MANTID_MDALGORITHMS_TOBYFITRESOLUTIONMODEL_H_
/**
  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#include "MantidMDAlgorithms/Quantification/MDResolutionConvolution.h"
#include "MantidMDAlgorithms/Quantification/CachedExperimentInfo.h"

#include "MantidMDAlgorithms/Quantification/Resolution/TobyFitYVector.h"
#include "MantidMDAlgorithms/Quantification/Resolution/TobyFitBMatrix.h"

#include "MantidAPI/ExperimentInfo.h"

#include "MantidKernel/ClassMacros.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/NDRandomNumberGenerator.h"

namespace Mantid
{
  namespace MDAlgorithms
  {
    /// Small structure to map a 4D box to named variables
    struct DLLExport QOmegaPoint
    {
      /// Constructor with a box & event
      QOmegaPoint(const API::IMDIterator & box, const size_t eventIndex)
        : qx(box.getInnerPosition(eventIndex, 0)),
          qy(box.getInnerPosition(eventIndex, 1)),
          qz(box.getInnerPosition(eventIndex, 2)),
          deltaE(box.getInnerPosition(eventIndex, 3)) {}
      /// Constructor with 3 Q values & an energy
      QOmegaPoint(const double qX, const double qY, const double qZ, const double dE)
      : qx(qX),
        qy(qY),
        qz(qZ),
        deltaE(dE) {}
      ///
      double qx, qy, qz, deltaE;
    };

    /**
     * Implements the Monte Carlo integration over the instrument
     * resolution & foreground model from TobyFit classic
     *
     * It uses the Fit IFunction interface so that it can use the same
     * attribute mechanism
     */
    class DLLExport TobyFitResolutionModel : public MDResolutionConvolution
    {
    public:

      /// Default constructor (required by factory)
      TobyFitResolutionModel();
      /// Construct with a model pointer & a pointer to the fitting function
      TobyFitResolutionModel(const API::IFunctionMD & fittedFunction,
                                      const std::string & fgModelName);
      /// Destructor
      ~TobyFitResolutionModel();

      /// Returns the function's name
      std::string name() const { return "TobyFitResolutionModel"; }
      /// Returns the value of the model convoluted with the resolution
      virtual double signal(const API::IMDIterator & box, const uint16_t innerRunIndex,
                            const size_t eventIndex) const;

    private:
      DISABLE_COPY_AND_ASSIGN(TobyFitResolutionModel);

      friend class TobyFitYVector;      

      /// Declare function attributes
      void declareAttributes();
      /// Declare fitting parameters
      void declareParameters();
      /// Cache some frequently used attributes
      void setAttribute(const std::string& name, const API::IFunction::Attribute & value);

      /// Calculate resolution coefficients
      void calculateResolutionCoefficients(const CachedExperimentInfo & observation,
                                           const QOmegaPoint & eventPoint) const;
      /// Generates the vector of random points
      void generateIntegrationVariables(const CachedExperimentInfo & observation,
                                        const QOmegaPoint & eventPoint) const;
      /// Returns the next set of random numbers
      const std::vector<double> & generateRandomNumbers() const;

      /// Map integration variables to perturbed values in Q-E space
      void calculatePerturbedQE(const CachedExperimentInfo & observation,const QOmegaPoint & eventPoint) const;
      /// Return true if it is time to check for convergence of the
      /// current sigma value
      bool checkForConvergence(const int step) const;
      /// Returns true if the Monte Carlo loop should be broken
      bool hasConverged(const int step, const double sumSigma,
                        const double sumSigmaSqr, const double avgSigma) const;

      /// Called before a function evaluation begins
      void functionEvalStarting();
      /// Called after a function evaluation is finished
      void functionEvalFinished();
      /// Called just before the monte carlo loop starts
      void monteCarloLoopStarting() const;

      /// Cache detector observations once when the workspace is set
      void preprocess(const API::IMDEventWorkspace_const_sptr & workspace);
      /// Called just before the fitting job starts
      void setUpForFit();
      /// Set up the calculator for the given number of threads
      void setNThreads(int nthreads);
      /// Setup the random number generator based on the given type
      void setupRandomNumberGenerator();
      /// Delete random number generator object(s)
      void deleteRandomNumberGenerator();

      /// Required by the interface. Does nothing
      void function(const Mantid::API::FunctionDomain&, Mantid::API::FunctionValues&) const {}

      /// Storage for currently in use random number generators
      mutable std::vector<Kernel::NDRandomNumberGenerator*> m_randomNumbers;
      /// Check for convergence after loop min number of steps
      int m_mcLoopMin;
      /// Maximum number of Monte Carlo evaluations
      int m_mcLoopMax;
      /// Store the MC type attribute
      int m_mcType;
      /// Tolerance for relative error. Loop breaks out when this is reached
      double m_mcRelErrorTol;
      /// Flags whether we should only include the foreground model
      bool m_foregroundOnly;
      /// Flag for including crystal mosaic
      bool m_mosaicActive;

      /// A pre-sized matrix for the resolution coefficients
      mutable std::vector<TobyFitBMatrix> m_bmatrix;
      /// A pre-sized vector for the randomly generated points
      mutable std::vector<TobyFitYVector> m_yvector;
      /// The generated value of the in-place mosaic (eta_2)
      mutable std::vector<double> m_etaInPlane;
      /// The generated value of the in-place mosaic (eta_3)
      mutable std::vector<double> m_etaOutPlane;
      /// A pre-sized vector for the QE position to be evaluated
      mutable std::vector<std::vector<double> > m_deltaQE;

      /// Cache of experiment info caches
      std::map<std::pair<int, detid_t>, CachedExperimentInfo*> m_exptCache;
    };
  }
}


#endif /* MANTID_MDALGORITHMS_TOBYFITRESOLUTIONMODEL_H_*/
