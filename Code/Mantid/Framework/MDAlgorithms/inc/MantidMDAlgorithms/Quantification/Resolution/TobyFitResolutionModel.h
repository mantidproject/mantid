#ifndef MANTID_MDALGORITHMS_TOBYFITRESOLUTIONMODEL_H_
#define MANTID_MDALGORITHMS_TOBYFITRESOLUTIONMODEL_H_
/**
  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

  File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#include "MantidMDAlgorithms/Quantification/MDResolutionConvolution.h"
#include "MantidMDAlgorithms/Quantification/Observation.h"

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
      const double qx, qy, qz, deltaE;
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
      /// Returns the function's name
      std::string name()const { return "MonteCarloResolutionConvolution"; }

      /// Returns true if the given attribute is active.
      bool useAttribute(const unsigned int variable) const;
      /// Returns true if the given attribute is active
      bool useAttribute(const std::string & attr) const;

      /// Returns the value of the model convoluted with the resolution
      double signal(const API::IMDIterator & box, const size_t eventIndex,
                    API::ExperimentInfo_const_sptr experimentInfo) const;

    private:
      DISABLE_COPY_AND_ASSIGN(TobyFitResolutionModel);
      /// Declare function attributes
      void declareAttributes();
      /// Declare fitting parameters
      void declareParameters();
      /// Cache some frequently used attributes
      void setAttribute(const std::string& name, const API::IFunction::Attribute & value);

      /// Ensure the run parameters are up to date
      void updateRunParameters(const Observation & exptInfo) const;

      /// Calculate resolution coefficients
      void calculateResolutionCoefficients(const Observation & observation,
                                           const QOmegaPoint & eventPoint) const;
      /// Generates the vector of random points
      void generateIntegrationVariables(const Observation & observation,
                                        const QOmegaPoint & eventPoint) const;
      /// Map integration variables to perturbed values in Q-E space
      void calculatePerturbedQE(const Observation & observation,const QOmegaPoint & eventPoint) const;

      /// Return true if it is time to check for convergence of the
      /// current sigma value
      bool checkForConvergence(const int step) const;
      /// Returns true if the Monte Carlo loop should be broken
      bool hasConverged(const int step, const double sumSigma,
                        const double sumSigmaSqr, const double avgSigma) const;

      /// Required by the interface
      void function(const Mantid::API::FunctionDomain&, Mantid::API::FunctionValues&) const {}

      /// A random number generator
      Kernel::NDRandomNumberGenerator *m_randGen;
      /// The value to mark an attribute as active
      int m_activeAttrValue;
      /// Check for convergence after loop min number of steps
      int m_mcLoopMin;
      /// Maximum number of Monte Carlo evaluations
      int m_mcLoopMax;
      /// Tolerance for relative error. Loop breaks out when this is reached
      double m_mcRelErrorTol;

      /// A pre-sized matrix for the resolution coefficients
      mutable TobyFitBMatrix m_bmatrix;
      /// A pre-sized vector for the randomly generated points
      mutable TobyFitYVector m_yvector;
      /// The generated value of the in-place mosaic (eta_2)
      mutable double m_etaInPlane;
      /// The generated value of the in-place mosaic (eta_3)
      mutable double m_etaOutPlane;
      /// A pre-sized vector for the QE position to be evaluated
      mutable std::vector<double> m_deltaQE;
    };
  }
}


#endif /* MANTID_MDALGORITHMS_TOBYFITRESOLUTIONMODEL_H_*/
