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

       double RunParam::getEi(const int irun);
       double RunParam::getPsi(const int irun);
       double RunParam::getElo(const int irun);
       double RunParam::getEhi(const int irun);
       double RunParam::getDe(const int irun);
       double RunParam::getX0(const int irun);
       double RunParam::getXa(const int irun);
       double RunParam::getX1(const int irun);
       double RunParam::getWa(const int irun);
       double RunParam::getHa(const int irun);
       double RunParam::getS1(const int irun);
       double RunParam::getS2(const int irun);
       double RunParam::getS3(const int irun);
       double RunParam::getS4(const int irun);
       double RunParam::getS5(const int irun);
       double RunParam::getThetam(const int irun);
       int RunParam::getImod(const int irun);
       double RunParam::getPslit(const int irun);
       double RunParam::getRadius(const int irun);
       double RunParam::getRho(const int irun);
       double RunParam::getHz(const int irun);
       double RunParam::getTjit(const int irun);
       double RunParam::getAs(const int irun);
       double RunParam::getBs(const int irun);
       double RunParam::getCs(const int irun);
       double RunParam::getAa(const int irun);
       double RunParam::getBb(const int irun);
       double RunParam::getCc(const int irun);
       double RunParam::getUh(const int irun);
       double RunParam::getUk(const int irun);
       double RunParam::getUl(const int irun);
       double RunParam::getVh(const int irun);
       double RunParam::getVk(const int irun);
       double RunParam::getVl(const int irun);
       double RunParam::getOmega(const int irun);
       double RunParam::getGs(const int irun);
       double RunParam::getGl(const int irun);
       double RunParam::getDpsi(const int irun);
       double RunParam::getXh(const int irun);
       double RunParam::getXk(const int irun);
       double RunParam::getXl(const int irun);
       double RunParam::getYh(const int irun);
       double RunParam::getYk(const int irun);
       double RunParam::getYl(const int irun);
       double RunParam::getSx(const int irun);
       double RunParam::getSy(const int irun);
       double RunParam::getSz(const int irun);
       int RunParam::getIsam(const int irun);
       double RunParam::getTemp(const int irun);
       double RunParam::getEta(const int irun);

       void RunParam::setEi(const int irun, const double val);
       void RunParam::setPsi(const int irun, const double val);
       void RunParam::setElo(const int irun, const double val);
       void RunParam::setEhi(const int irun, const double val);
       void RunParam::setDe(const int irun, const double val);
       void RunParam::setX0(const int irun, const double val);
       void RunParam::setXa(const int irun, const double val);
       void RunParam::setX1(const int irun, const double val);
       void RunParam::setWa(const int irun, const double val);
       void RunParam::setHa(const int irun, const double val);
       void RunParam::setS1(const int irun, const double val);
       void RunParam::setS2(const int irun, const double val);
       void RunParam::setS3(const int irun, const double val);
       void RunParam::setS4(const int irun, const double val);
       void RunParam::setS5(const int irun, const double val);
       void RunParam::setThetam(const int irun, const double val);
       void RunParam::setImod(const int irun, const int val);
       void RunParam::setPslit(const int irun, const double val);
       void RunParam::setRadius(const int irun, const double val);
       void RunParam::setRho(const int irun, const double val);
       void RunParam::setHz(const int irun, const double val);
       void RunParam::setTjit(const int irun, const double val);
       void RunParam::setAs(const int irun, const double val);
       void RunParam::setBs(const int irun, const double val);
       void RunParam::setCs(const int irun, const double val);
       void RunParam::setAa(const int irun, const double val);
       void RunParam::setBb(const int irun, const double val);
       void RunParam::setCc(const int irun, const double val);
       void RunParam::setUh(const int irun, const double val);
       void RunParam::setUk(const int irun, const double val);
       void RunParam::setUl(const int irun, const double val);
       void RunParam::setVh(const int irun, const double val);
       void RunParam::setVk(const int irun, const double val);
       void RunParam::setVl(const int irun, const double val);
       void RunParam::setOmega(const int irun, const double val);
       void RunParam::setGs(const int irun, const double val);
       void RunParam::setGl(const int irun, const double val);
       void RunParam::setDpsi(const int irun, const double val);
       void RunParam::setXh(const int irun, const double val);
       void RunParam::setXk(const int irun, const double val);
       void RunParam::setXl(const int irun, const double val);
       void RunParam::setYh(const int irun, const double val);
       void RunParam::setYk(const int irun, const double val);
       void RunParam::setYl(const int irun, const double val);
       void RunParam::setSx(const int irun, const double val);
       void RunParam::setSy(const int irun, const double val);
       void RunParam::setSz(const int irun, const double val);
       void RunParam::setIsam(const int irun, const int val);
       void RunParam::setTemp(const int irun, const double val);
       void RunParam::setEta(const int irun, const double val);

       void RunParam::readData(const std::string file);
       void RunParam::writeData(const std::string file);

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
