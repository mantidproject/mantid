#ifndef MDALGORITHM_RUNPARAM_H_
#define MDALGORITHM_RUNPARAM_H_


//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IMDWorkspace.h"

#include "MantidKernel/Tolerance.h"
#include "MantidGeometry/Math/mathSupport.h"
#include "MantidKernel/Matrix.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidGeometry/IDetector.h"
#include <boost/shared_ptr.hpp>

namespace Mantid
{
  namespace MDAlgorithms
  {
    /** 
    Container for run parameter data for Mantid TobyFit and the run dependent models that
    use these parameters. These include moderator and chopper models.

    Copyright &copy; 2007-2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
     */
    class DLLExport RunParam
    {
    public:
      /// Default constructor
      RunParam();
      /// Constructor with complete set of parameters
      RunParam(double ei,double psi,double elo,double ehi,
          double de, double x0, double xa, double x1,
          double wa, double ha, double s1, double s2,
          double s3, double s4, double s5, double thetam,
          int modModel, double pslit, double radius,
          double rho, double hz, double tjit,
          double as, double bs, double cs,
          double aa, double bb, double cc,
          double uh, double uk, double ul,
          double vh, double vk, double vl,
          double omega, double gs, double gl,
          double dpsi, double xh, double xk,
          double xl, double yh, double yk,
          double yl, double sx, double sy,
          double sz, int isam, double temp,
          double eta);
      /// Constructor which will read from ExperimentInfo TODO
      RunParam(const int runID);

      /// Destructor
      ~RunParam();


      // These gets are public for now but may change
      // The sets will still need to be public

      /// return incident energy of run
      double getEi() const;
      /// return psi angle of run
      double getPsi() const;
      /// return elo, lower energy bound, only used in in simulation of SPE data
      double getElo();
      /// return ehi, upper energy bound
      double getEhi();
      /// return de of run
      double getDe() const;
      /// return x0 of run
      double getX0() const;
      /// return xa of run
      double getXa() const;
      /// return x1 of run
      double getX1() const;
      /// return wa of run (aperture values)
      double getWa() const;
      /// return ha of run
      double getHa() const;
      /// return s1 of run (moderator model values)
      double getS1() const;
      /// return s2 of run
      double getS2() const;
      /// return s3 of run
      double getS3() const;
      /// return s3 of run
      double getS4() const;
      /// return s5 of run
      double getS5() const;
      /// return thetam of run (moderator angle)
      double getThetam() const;
      /// return modModel of run
      int getModModel() const;
      /// return pslit of run chopper
      double getPslit() const;
      /// return radius of run chopper
      double getRadius() const;
      /// return rho of run chopper
      double getRho() const;
      /// return angular velocity of run chopper
      double getAngVel() const;
      /// return tjit of chopper
      double getTjit() const;
      /// return as of run sample unit cell data
      double getAs();
      /// return bs of run sample
      double getBs();
      /// return cs of run sample
      double getCs();
      /// return aa of run sample
      double getAa();
      /// return bb of run sample
      double getBb();
      /// return cc of run sample
      double getCc();
      /// return uh of run sample
      double getUh();
      /// return uk of run
      double getUk();
      /// return ul of run
      double getUl();
      /// return vh of run
      double getVh();
      /// return vk of run
      double getVk();
      /// return vl of run
      double getVl();
      /// return omega of run (Goniometer alignment)
      double getOmega();
      /// return gs of run (Crystal orientation correction)
      double getGs();
      /// return gl of run
      double getGl();
      /// return dpsi of run
      double getDpsi();
      /// return xh of run (Sample shape alignment)
      double getXh();
      /// return xk of run
      double getXk();
      /// return xl of run
      double getXl();
      /// return yh of run
      double getYh();
      /// return yk of run
      double getYk();
      /// return yl of run
      double getYl();
      /// return sample x dimension for run
      double getSx();
      /// return sample y dimension for run
      double getSy();
      /// return sample z dimension for run
      double getSz();
      /// return iSam setting of run
      int getIsam();
      /// return temp of run
      double getTemp();
      /// return eta of run (Mosaic)
      double getEta();
      /// return detector position, dimensions
      void getDetInfo(const detid_t, Kernel::V3D & position, Kernel::V3D & dimensions, double & deps) const;

    public:
      void setEi( const double val);
      void setPsi( const double val);
      void setElo( const double val);
      void setEhi( const double val);
      void setDe( const double val);
      void setX0( const double val);
      void setXa( const double val);
      void setX1( const double val);
      void setWa( const double val);
      void setHa( const double val);
      void setS1( const double val);
      void setS2( const double val);
      void setS3( const double val);
      void setS4( const double val);
      void setS5( const double val);
      void setThetam( const double val);
      void setModModel( const int val);
      void setPslit( const double val);
      void setRadius( const double val);
      void setRho( const double val);
      void setHz( const double val);
      void setTjit( const double val);
      void setAs( const double val);
      void setBs( const double val);
      void setCs( const double val);
      void setAa( const double val);
      void setBb( const double val);
      void setCc( const double val);
      void setUh( const double val);
      void setUk( const double val);
      void setUl( const double val);
      void setVh( const double val);
      void setVk( const double val);
      void setVl( const double val);
      void setOmega( const double val);
      void setGs( const double val);
      void setGl( const double val);
      void setDpsi( const double val);
      void setXh( const double val);
      void setXk( const double val);
      void setXl( const double val);
      void setYh( const double val);
      void setYk( const double val);
      void setYl( const double val);
      void setSx( const double val);
      void setSy( const double val);
      void setSz( const double val);
      void setIsam( const int val);
      void setTemp( const double val);
      void setEta( const double val);
      void setRunLatticeMatrices( const boost::shared_ptr<Mantid::Geometry::OrientedLattice> lattice);
      void setDetInfo(const detid_t, const Kernel::V3D & position, const Kernel::V3D & dimensions, const double deps);

      void setTransforms();
      const Mantid::Kernel::DblMatrix & getSMat() const;
      const Mantid::Kernel::DblMatrix & getCubInvMat() const;

      void readData(const std::string file);
      void writeData(const std::string file);

      //double moderatorSampleVolumeTable();
      /*
       * Standard deviation of the moderator signal based on the given model in seconds
       */
      double getTauModeratorSignal() const;

      /*
       * Get the average delay of the moderator in us, based on the moderator model.
       * Used to scale line shape delay in mcYVec, maybe removed.
       */
      double getTauModeratorAverageUs() const;

      /*
       * Moderator time only used in old TF convolution models, not the Monte Carlo method
       */
      double getTauModeratorMean() const;

      /*
       * Simple energy resolution only based on moderator and chopper
       * @param eps energy tramsfer (meV)
       * @param x2 sample - detector distance (m)
       * @return std deviation of energy resolution (meV)
       */
      double energyResolutionModChop(const double eps, const double x2 ) const;

      double moderatorDepartTime(const double randomVar) const;
      double moderatorTimeLookUp( const double randomVar) const;
      double areaToTIK(const double area, const double tauF, const double tauS, const double r) const;

      /// Return a random point on the aperture w.r.t. its centre - input is two uniform random values
      void getAperturePoint(const double ran1, const double ran2, double & pW, double & pH) const;

      /// Sample the the chopper time distribution - takes uniform random var [0:1] returns triangular distribution scaled by
      /// effective time variability of chopper
      double chopperTimeDist(const double ranvar) const;
      /// get a chopper jitter time based on random var [0:1]
      double chopperJitter(const double ranvar) const;
      /// return tausqr, the chopper variance based on current parameters
      double tChopVariance() const;
      /// return the std deviation from the chopper
      double getTauChopperSignal() const;

      /// Given a uniform [0:1] random value return point [-1:1] with triangular probability distribution
      double tridev(const double ranvar) const;
      // Map 2 uniform random numbers to 2 Gaussian vars
      void gasdev2d(const double ran1, const double ran2, double & gaus1, double & gaus2) const;
      /// Given three uniform random value [0:1] get a point within the sample w.r.t. sample origin
      void getSamplePoint(const double ran1, const double ran2, const double ran3, double & px, double & py, double & pz) const;

      /// Get the mosaic parameters eta2, eta3 given 2 uniform random variables [0,1]
      void getEta23( const double ranvar1, const double ranvar2, double & eta2, double & eta3) const;
      /// set Chopper times
      void setTauChopperSignal();

    protected:
      double areaIK(const double x, const double tauF, const double tauS, const double r) const;
      double funAreaToTIK(const double x) const;
      double zeroBrent ( const double a, const double b, const double t /*, double ( *f) ( double x )*/ ) const;
      double tChop(const double pSlit, const double radius, const double rho, const double angVel, const double eI ) const;
      double gsqrChop (const double pSlit, const double radius, const double rho, const double angVel, const double eI) const;

    private:
      /// @cond
      // constant to convert mm to m
      static const double mmTom;
      double m_ei;
      double m_psi;
      double m_elo;
      double m_ehi;
      double m_de;
      double m_x0;
      double m_xa;
      double m_x1;
      double m_wa;
      double m_ha;
      double m_s1;
      double m_s2;
      double m_s3;
      double m_s4;
      double m_s5;
      double m_thetam;
      int m_modModel;
      double m_pslit;
      double m_radius;
      double m_rho;
      double m_angVel;
      double m_tjit;
      double m_as;
      double m_bs;
      double m_cs;
      double m_aa;
      double m_bb;
      double m_cc;
      double m_uh;
      double m_uk;
      double m_ul;
      double m_vh;
      double m_vk;
      double m_vl;
      double m_omega;
      double m_gs;
      double m_gl;
      double m_dpsi;
      double m_xh;
      double m_xk;
      double m_xl;
      double m_yh;
      double m_yk;
      double m_yl;
      double m_sx;
      double m_sy;
      double m_sz;
      int m_isam;
      double m_temp;
      double m_eta;
      double m_eta_sig;
      bool m_moderatorChange;
      bool m_chopChange;
      double m_tauChopperSignal, m_tauChopperEffective;
      mutable double m_tauF, m_tauS, m_r, m_offset;
      mutable std::vector<double> m_areaToTIKLookup;
      int m_modLookupRes;
      void initModTime() const;
      Mantid::Kernel::DblMatrix m_sMat;
      Mantid::Kernel::DblMatrix m_cubInvMat;
      // store for detector data - this may be replaced with direct access to detector objects
      // Two vectors give detector position (x2,phi,beta) and size (width,height,depth)
      std::map<detid_t, std::pair<Kernel::V3D, Kernel::V3D>> m_detIdMap;
      // sore for energy width of each detector pixel - for now just one constant - should be peer pixle
      double m_deps;

      /// @endcond

    };
  }
}
#endif //MDALGORITHM_RUNPARAM_
