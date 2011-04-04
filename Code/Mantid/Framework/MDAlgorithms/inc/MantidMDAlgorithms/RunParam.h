#ifndef MDALGORITHM_RUNPARAM_H_
#define MDALGORITHM_RUNPARAM_H_


//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IMDWorkspace.h"

#include "MantidGeometry/Tolerance.h"
#include "MantidGeometry/Math/mathSupport.h"
#include "MantidGeometry/Math/Matrix.h"

namespace Mantid
{
  namespace MDAlgorithms
  {
    /** 
    Container for run parameter data for Mantid TobyFit

    @author Ron Fowler
    @date 17/02/2011

    Copyright &copy; 2007-2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
       RunParam(double ei,double psi,double elo,double ehi,
          double de, double x0, double xa, double x1,
          double wa, double ha, double s1, double s2,
          double s3, double s4, double s5, double thetam,
          int imod, double pslit, double radius,
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

       /// Destructor
       ~RunParam();

       /// @cond
       /// return incident energy of run
       double getEi();
       /// return psi of run
       double getPsi();
       /// return elo of run
       double getElo();
       /// return ehi of run
       double getEhi();
       /// return de of run
       double getDe();
       /// return x0 of run
       double getX0();
       /// return xa of run
       double getXa();
       /// return x1 of run
       double getX1();
       /// return wa of run
       double getWa();
       /// return ha of run
       double getHa();
       /// return s1 of run
       double getS1();
       /// return s2 of run
       double getS2();
       /// return s3 of run
       double getS3();
       /// return s3 of run
       double getS4();
       /// return s5 of run
       double getS5();
       /// return thetam of run
       double getThetam();
       /// return Imod of run
       int getImod();
       /// return pslit of run chopper
       double getPslit();
       /// return radius of run chopper
       double getRadius();
       /// return rho of run chopper
       double getRho();
       /// return hz of run chopper
       double getHz();
       /// return tjit of run
       double getTjit();
       /// return as of run sample
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
       /// return omega of run
       double getOmega();
       /// return gs of run
       double getGs();
       /// return gl of run
       double getGl();
       /// return dpsi of run
       double getDpsi();
       /// return xh of run
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
       /// return eta of run
       double getEta();

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
       void setImod( const int val);
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

       void readData(const std::string file);
       void writeData(const std::string file);

    private:
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
       int m_imod;
       double m_pslit;
       double m_radius;
       double m_rho;
       double m_hz;
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

       /// @endcond

    };
  }
}
#endif //MDALGORITHM_RUNPARAM_
