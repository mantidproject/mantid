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

namespace Mantid
{
    namespace MDAlgorithms
    {
        /**
        Semi-abstract class for fitting with instrument resolution function.
        This class implements the MC simulation of the resolution function.
        A function defining the scattering S(Q,W) is required in an inherited
        class to form the real fit function.

        @author Ron Fowler
        @date 07/04/2011

        Copyright &copy; 2007-11 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
        // This class will  define the "function" method used by GenericFit but will not define the
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
        protected:
            /// function to return the calculated signal at cell r, given the energy dependent model applied to points
            virtual double function(Mantid::API::IMDIterator& r) const;
            /// This will be over ridden by the user's SQW function TODO argument list is not general enough
            virtual double sqwBroad(const std::vector<double> & point, const std::vector<double> & fgParams,
                    const double temp, const Geometry::Matrix<double> & ubinv) const=0;
            /// This will be over ridden by the user's getParam function
            virtual void getParams() const = 0;
            /// Perform convolution on one MDPoint
            void sqwConvolution(boost::shared_ptr<Mantid::Geometry::MDPoint> & point,
                    double & fgSignal, double & fgError) const;
            /// Perform convolution on one MDPoint
            void sqwConvolutionMC(boost::shared_ptr<Mantid::Geometry::MDPoint> & point,
                    double & fgSignal, double & fgError) const;
            /// Pointer to the cut data
            boost::shared_ptr<Mantid::API::IMDWorkspace> imdwCut;
            /// For MC integration return next point in space
            void getNextPoint(std::vector<double>&, int);
            /// Initialise random number generators
            void initRandomNumbers();
            /// Reset random number generators
            void resetRandomNumbers();
            /// A pointer to the random number generator
            Kernel::RandomNumberGenerator *m_randGen;
            /// Sample S(Q,eps) function from tobyfit
            double sqwBroad601(const std::vector<double> &, const std::vector<double> & ,
                        const double, const Geometry::Matrix<double> & );
            /// function to evaluate y/(1-exp(-y)) including y=0 and large -ve y
            double pop(const double) const;
            /// function to bose factor
            double bose(const double, const double) const;
            /// function to perform look up of magnetic form factor
            double formTable(const double) const;
            /// Sample_area_table is lookup function for moderator parameters
            double sampleAreaTable( const double );
            /// convert 2 uniform random values to two gaussian distribution values
            void gasdev2d( const double, const double, double &, double &);
            /// tridev function from tf - maps a uniform distribution to triangular form?
            double tridev(const double);
            void monteCarloSampleVolume( const double, const double, const double, double &, double &, double & );
            /// function to build bmatrix
            void bMatrix(const double, const double, const double, const double, const double,
                  const double, const double, const double,
                  const Geometry::Matrix<double> & , const Geometry::Matrix<double> & ,
                  Geometry::Matrix<double> & );
            /// function to build matrices dMat and dinvMat
            void dMatrix(const double , const double , Geometry::Matrix<double> & ,
                                        Geometry::Matrix<double> & );
            /// Energy resoultion function for moderator and chopper from TF
            double enResModChop(const double , const double , const double , const double ,
                  const double , const double , const double );
            /// generate a random scaled vector in the selected space of up to 13 dimensions
            void mcYVec(const double detWidth, const double detHeight, const double detTimeBin,
                  std::vector<double> & yVec, double & eta2, double & eta3 );
            /// get transform matrices, vectors for reciprocal space
            int rlatt(const std::vector<double> & a, const std::vector<double> & ang,
                       std::vector<double> & arlu, std::vector<double> & angrlu,
                       Geometry::Matrix<double> & dMat );
        private:
            /// The default seed for MT random numbers
            int m_randSeed;
            /// Flag for random number method - may change to enum to allow for several methods
            bool m_sobol;
            /// Cached Sample Bounding box size for current detector/instrument, assuming cuboid sample
            std::vector<double> m_sampleBB;
            std::vector<double> m_detectorBB;
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
        };

    } // namespace MDAlgorithms
} // namespace Mantid

#endif /*MANTID_MDALGORITHMS_SIMULATERESOULTION_H_*/
