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
       /// Destructor
       ~RunParam();

       double getEi(const int irun);
       double getPsi(const int irun);
       double getElo(const int irun);
       double getEhi(const int irun);
       double getDe(const int irun);
       double getX0(const int irun);
       double getXa(const int irun);
       double getX1(const int irun);
       double getWa(const int irun);
       double getHa(const int irun);
       double getS1(const int irun);
       double getS2(const int irun);
       double getS3(const int irun);
       double getS4(const int irun);
       double getS5(const int irun);
       double getThetam(const int irun);
       int getImod(const int irun);
       double getPslit(const int irun);
       double getRadius(const int irun);
       double getRho(const int irun);
       double getHz(const int irun);
       double getTjit(const int irun);
       double getAs(const int irun);
       double getBs(const int irun);
       double getCs(const int irun);
       double getAa(const int irun);
       double getBb(const int irun);
       double getCc(const int irun);
       double getUh(const int irun);
       double getUk(const int irun);
       double getUl(const int irun);
       double getVh(const int irun);
       double getVk(const int irun);
       double getVl(const int irun);
       double getOmega(const int irun);
       double getGs(const int irun);
       double getGl(const int irun);
       double getDpsi(const int irun);
       double getXh(const int irun);
       double getXk(const int irun);
       double getXl(const int irun);
       double getYh(const int irun);
       double getYk(const int irun);
       double getYl(const int irun);
       double getSx(const int irun);
       double getSy(const int irun);
       double getSz(const int irun);
       int getIsam(const int irun);
       double getTemp(const int irun);
       double getEta(const int irun);

       void setEi(const int irun, const double val);
       void setPsi(const int irun, const double val);
       void setElo(const int irun, const double val);
       void setEhi(const int irun, const double val);
       void setDe(const int irun, const double val);
       void setX0(const int irun, const double val);
       void setXa(const int irun, const double val);
       void setX1(const int irun, const double val);
       void setWa(const int irun, const double val);
       void setHa(const int irun, const double val);
       void setS1(const int irun, const double val);
       void setS2(const int irun, const double val);
       void setS3(const int irun, const double val);
       void setS4(const int irun, const double val);
       void setS5(const int irun, const double val);
       void setThetam(const int irun, const double val);
       void setImod(const int irun, const int val);
       void setPslit(const int irun, const double val);
       void setRadius(const int irun, const double val);
       void setRho(const int irun, const double val);
       void setHz(const int irun, const double val);
       void setTjit(const int irun, const double val);
       void setAs(const int irun, const double val);
       void setBs(const int irun, const double val);
       void setCs(const int irun, const double val);
       void setAa(const int irun, const double val);
       void setBb(const int irun, const double val);
       void setCc(const int irun, const double val);
       void setUh(const int irun, const double val);
       void setUk(const int irun, const double val);
       void setUl(const int irun, const double val);
       void setVh(const int irun, const double val);
       void setVk(const int irun, const double val);
       void setVl(const int irun, const double val);
       void setOmega(const int irun, const double val);
       void setGs(const int irun, const double val);
       void setGl(const int irun, const double val);
       void setDpsi(const int irun, const double val);
       void setXh(const int irun, const double val);
       void setXk(const int irun, const double val);
       void setXl(const int irun, const double val);
       void setYh(const int irun, const double val);
       void setYk(const int irun, const double val);
       void setYl(const int irun, const double val);
       void setSx(const int irun, const double val);
       void setSy(const int irun, const double val);
       void setSz(const int irun, const double val);
       void setIsam(const int irun, const int val);
       void setTemp(const int irun, const double val);
       void setEta(const int irun, const double val);

       void readData(const std::string file);
       void writeData(const std::string file);

    private:
       std::vector<double> m_ei;
       std::vector<double> m_psi;
       std::vector<double> m_elo;
       std::vector<double> m_ehi;
       std::vector<double> m_de;
       std::vector<double> m_x0;
       std::vector<double> m_xa;
       std::vector<double> m_x1;
       std::vector<double> m_wa;
       std::vector<double> m_ha;
       std::vector<double> m_s1;
       std::vector<double> m_s2;
       std::vector<double> m_s3;
       std::vector<double> m_s4;
       std::vector<double> m_s5;
       std::vector<double> m_thetam;
       std::vector<int> m_imod;
       std::vector<double> m_pslit;
       std::vector<double> m_radius;
       std::vector<double> m_rho;
       std::vector<double> m_hz;
       std::vector<double> m_tjit;
       std::vector<double> m_as;
       std::vector<double> m_bs;
       std::vector<double> m_cs;
       std::vector<double> m_aa;
       std::vector<double> m_bb;
       std::vector<double> m_cc;
       std::vector<double> m_uh;
       std::vector<double> m_uk;
       std::vector<double> m_ul;
       std::vector<double> m_vh;
       std::vector<double> m_vk;
       std::vector<double> m_vl;
       std::vector<double> m_omega;
       std::vector<double> m_gs;
       std::vector<double> m_gl;
       std::vector<double> m_dpsi;
       std::vector<double> m_xh;
       std::vector<double> m_xk;
       std::vector<double> m_xl;
       std::vector<double> m_yh;
       std::vector<double> m_yk;
       std::vector<double> m_yl;
       std::vector<double> m_sx;
       std::vector<double> m_sy;
       std::vector<double> m_sz;
       std::vector<int> m_isam;
       std::vector<double> m_temp;
       std::vector<double> m_eta;

    };
  }
}
#endif //MDALGORITHM_RUNPARAM_
