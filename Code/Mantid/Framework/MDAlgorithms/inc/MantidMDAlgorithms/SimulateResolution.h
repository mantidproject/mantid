#ifndef MANTID_MDALGORITHMS_SIMULATERESOULTION_H_
#define MANTID_MDALGORITHMS_SIMULATERESOULTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ParamFunction.h"
#include "MantidKernel/RandomNumberGenerator.h"
#include "MantidAPI/IFunctionMD.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidMDAlgorithms/RunParam.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidMDAlgorithms/MagneticFormFactor.h"

#include <gsl/gsl_errno.h>
#include <gsl/gsl_qrng.h>

namespace Mantid
{
    namespace MDAlgorithms
    {

        /**
        Semi-abstract class for fitting with instrument resolution function.
        This class implements the MC/Sobol simulation of the resolution function.
        A function defining the scattering S(Q,W) is required in a subclass
        to provide the real fit function.
        This function will be invoked from the fitting process to return the expected signal for
        a given set of model parameters at each of the physical detectors within the instrument.
        In the MDWorkspaces there may be data from multiple runs and the runIndex of the data point
        is used to identify which case is being used.
1

        Copyright &copy; 2007-12 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

        File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
        Code Documentation is available at: <http://doxygen.mantidproject.org>
        */
        // This class will  define the "function" method used by Fit but will not define the
        // actual sqwBroad or sqwSharp functions that define the real model
        class DLLExport SimulateResolution : public API::ParamFunction, public API::IFunctionMD
        {
        public:
            /// Constructor
            SimulateResolution();
            /// Destructor
            virtual ~SimulateResolution();

            /// overwrite IFunction base class methods
            std::string name()const{return "SimulateResolution";}

            /// set pointer to runData vector
            void setWorkspaceMD(API::WorkspaceGroup_sptr wsGroup);
            /// Create a RunParam pointer - this is a temporary method for testing and will be replaced
            void setRunDataInfo(boost::shared_ptr<Mantid::MDAlgorithms::RunParam> runData);
            /// Set magnetic form factor, function can be access in use SQW
            void setMagneticForm(const int atomicNo, const int ionisation);

            // Attributes
            /// Returns the number of attributes associated with the function for now minStep, maxStep, tol for MC
            size_t nAttributes()const{return 3;}
            /// Returns a list of attribute names
            std::vector<std::string> getAttributeNames()const;
            /// Return a value of attribute attName
            Attribute getAttribute(const std::string& attName)const;
            /// Set a value to attribute attName
            void setAttribute(const std::string& attName,const Attribute& );
            /// Check if attribute attName exists
            bool hasAttribute(const std::string& attName)const;

        protected:
            /// function to return the calculated signal at cell r, given the energy dependent model applied to points
            virtual double functionMD(const Mantid::API::IMDIterator& r) const;
            /// This is the new more general interface to the user scattering function which can takes different arguments
            /// depending on the sharp/broad setting.
            virtual void userSqw(const boost::shared_ptr<Mantid::MDAlgorithms::RunParam> run, const std::vector<double> & params,
                                 const std::vector<double> & qE, std::vector<double> & result) const=0;
            /// This method must be overridden by the user to define if a sharp or broad model is provided.
            virtual bool userModelIsBroad() const=0;
            /// This will be over ridden by the user's getParam function
            virtual void getParams(std::vector<double> & params) const =0;

            /** Perform the convolution calculation for one pixel. May implement other methods
             * besides MC.
             *
             * @param it :: iterator that contains event to be simulated
             * @param event :: MDEvent that corresponds to a pixel in a named detector/run
             * @param error :: the error estimate of the calculated scattering
             * @return :: the calculated scattering value for this pixel given the SQW model
             */
            double sqwConvolution(const Mantid::API::IMDIterator& it, size_t & event,  double & error) const;

            /** Perform the convolution calculation for one pixel using MonteCarlo/Sobol method
             *
             * @param it :: iterator that contains event to be simulated
             * @param event :: MDEvent that corresponds to a pixel in a named detector/run
             * @param error :: the error estimate of the calculated scattering
             * @return :: the calculated scattering value for this pixel given the SQW model
             */
            double sqwConvolutionMC(const Mantid::API::IMDIterator& it, size_t & event, double & error) const;


            /// Find magnetic form factor at q^2 point
            double magneticForm(const double qSquared) const;
            /// For MC integration return next point in space
            void getNextPoint(std::vector<double>&) const;
            /// Initialise random number generators
            void initRandom();
            /// Initialise particular random method, sobol or random
            void initRandom(const bool methodSobol);
            /// Reset random number generators
            void resetRandomNumbers();
            /// A pointer to the random number generator
            Kernel::RandomNumberGenerator *m_randGen;
            /// function to evaluate y/(1-exp(-y)) including y=0 and large -ve y
            double pop(const double) const;
            /// function to calculate bose factor
            double bose(const double, const double) const;
            /// function to perform look up of magnetic form factor
            double formTable(const double) const;
            /// Sample_area_table is lookup function for moderator parameters
            double sampleAreaTable( const double ) const;
            /// function to build bmatrix
            void bMatrix(const double, const double, const double, const double, const double,
                  const double, const double, const double,
                  const Kernel::Matrix<double> & , const Kernel::Matrix<double> & ,
                  Kernel::Matrix<double> & ) const;
            /// function to build matrices dMat and dinvMat
            void dMatrix(const double , const double , Kernel::Matrix<double> & ,
                                        Kernel::Matrix<double> & ) const;
            /// generate a random scaled vector in the selected space of up to 13 dimensions
            void mcYVec(const double ranvec[], const boost::shared_ptr<Mantid::MDAlgorithms::RunParam> run, const std::vector<double> & detectorBB,
                  const double detTimeBin, std::vector<double> & yVec, double & eta2, double & eta3 ) const;
            /// map from Yvec values to dQ/dE values
            void mcMapYtoQEVec(const double wi, const double wf, const std::vector<double> & q0, const Kernel::Matrix<double> & bMat,
                const Kernel::Matrix<double> & dInvMat, const std::vector<double> & yVec,
                const double eta2, const double eta3,
                std::vector<double> & perturbQE) const;
            /// get transform matrices, vectors for reciprocal space
            int rlatt(const std::vector<double> & a, const std::vector<double> & ang,
                       std::vector<double> & arlu, std::vector<double> & angrlu,
                       Kernel::Matrix<double> & dMat );

        private:
            /// Pointers to the run data for each run
            std::vector< boost::shared_ptr<MDAlgorithms::RunParam> > m_runData;
            /// Pointer to the group of input MDWorkspaces
            API::WorkspaceGroup_sptr m_mdWorkspaces;
            boost::shared_ptr<MagneticFormFactor> m_magForm;

            /// The default seed for MT random numbers
            int m_randSeed;
            /// Cached Sample Bounding box size for current detector/instrument, assuming cuboid sample
            std::vector<double> m_sampleBB;
            double m_detectorDepth, m_detectorWidth, m_detectorHeight;
            // Width of detector timembin
            double m_detectorTimebin;
            /// moderator parameters for current MDPoint
            double m_t_mod_av;
            double m_wa;
            double m_ha;
            // chopper parameters for current MDPoint
            double m_dt_chop_eff;
            double m_tjit_sig;
            // Mosaic parameter for current MDPoint
            double m_eta_sig;
            /// interpolation table for sampleAreaTable (needs generalisation)
            std::vector<double> m_xtab;
            /// Cached run number index for the current MDPoint
            int m_run;
            /// Flags for MC integration options
            std::vector<bool> m_mcOptVec;
            /// Random variables per model
            std::vector<int> m_mcVarCount;
            /// Names for the options within the Monte Carlo vector
            enum McOptions
            {
                mcLineShape       = 0,
                mcAperture        = 1,
                mcChopper         = 2,
                mcChopperJitter   = 3,
                mcSample          = 4,
                mcDetectorDepth   = 5,
                mcDetectorArea    = 6,
                mcDetectorTimeBin = 7,
                mcMosaic          = 8
            };
            /// Number of dimensions in use in MC method
            size_t m_randSize;
            /// Integration method names - only one method now but may be more added later
            enum IntegrationMethod
            {
               mcIntegration = 0
            };
            IntegrationMethod m_integrationMethod;
            // random number generator
            enum RandomMethod
            {
               sobol = 0,
               mTwister = 1
            };
            RandomMethod m_random;
            // GSL Sobol random number state information
            gsl_qrng *m_qRvec;
            int m_event;

            // Attribute values
            /// Min MC steps
            int m_mcLoopMin;
            /// Max MC steps
            int m_mcLoopMax;
            /// MC loop absolute tolerance to exit before Max steps
            double m_mcTol;
        };

    } // namespace MDAlgorithms
} // namespace Mantid

#endif /*MANTID_MDALGORITHMS_SIMULATERESOULTION_H_*/
