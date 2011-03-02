//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMDAlgorithms/RunParam.h"
#include "MantidAPI/IMDWorkspace.h"

#include "MantidGeometry/Tolerance.h"
#include "MantidGeometry/Math/mathSupport.h"
#include "MantidGeometry/Math/Matrix.h"

namespace Mantid
{
  namespace MDAlgorithms
  {
       // Constructor
       RunParam::RunParam()
       {
       }

       RunParam::~RunParam()
       {}

       /// @cond
       double RunParam::getEi(const int irun) {return m_ei[irun];}
       double RunParam::getPsi(const int irun) {return m_psi[irun];}
       double RunParam::getElo(const int irun) {return m_elo[irun];}
       double RunParam::getEhi(const int irun) {return m_ehi[irun];}
       double RunParam::getDe(const int irun) {return m_de[irun];}
       double RunParam::getX0(const int irun) {return m_x0[irun];}
       double RunParam::getXa(const int irun) {return m_xa[irun];}
       double RunParam::getX1(const int irun) {return m_x1[irun];}
       double RunParam::getWa(const int irun) {return m_wa[irun];}
       double RunParam::getHa(const int irun) {return m_ha[irun];}
       double RunParam::getS1(const int irun) {return m_s1[irun];}
       double RunParam::getS2(const int irun) {return m_s2[irun];}
       double RunParam::getS3(const int irun) {return m_s3[irun];}
       double RunParam::getS4(const int irun) {return m_s4[irun];}
       double RunParam::getS5(const int irun) {return m_s5[irun];}
       double RunParam::getThetam(const int irun) {return m_thetam[irun];}
       int RunParam::getImod(const int irun) {return m_imod[irun];}
       double RunParam::getPslit(const int irun) {return m_pslit[irun];}
       double RunParam::getRadius(const int irun) {return m_radius[irun];}
       double RunParam::getRho(const int irun) {return m_rho[irun];}
       double RunParam::getHz(const int irun) {return m_hz[irun];}
       double RunParam::getTjit(const int irun) {return m_tjit[irun];}
       double RunParam::getAs(const int irun) {return m_as[irun];}
       double RunParam::getBs(const int irun) {return m_bs[irun];}
       double RunParam::getCs(const int irun) {return m_cs[irun];}
       double RunParam::getAa(const int irun) {return m_aa[irun];}
       double RunParam::getBb(const int irun) {return m_bb[irun];}
       double RunParam::getCc(const int irun) {return m_cc[irun];}
       double RunParam::getUh(const int irun) {return m_uh[irun];}
       double RunParam::getUk(const int irun) {return m_uk[irun];}
       double RunParam::getUl(const int irun) {return m_ul[irun];}
       double RunParam::getVh(const int irun) {return m_vh[irun];}
       double RunParam::getVk(const int irun) {return m_vk[irun];}
       double RunParam::getVl(const int irun) {return m_vl[irun];}
       double RunParam::getOmega(const int irun) {return m_omega[irun];}
       double RunParam::getGs(const int irun) {return m_gs[irun];}
       double RunParam::getGl(const int irun) {return m_gl[irun];}
       double RunParam::getDpsi(const int irun) {return m_dpsi[irun];}
       double RunParam::getXh(const int irun) {return m_xh[irun];}
       double RunParam::getXk(const int irun) {return m_xk[irun];}
       double RunParam::getXl(const int irun) {return m_xl[irun];}
       double RunParam::getYh(const int irun) {return m_yh[irun];}
       double RunParam::getYk(const int irun) {return m_yk[irun];}
       double RunParam::getYl(const int irun) {return m_yl[irun];}
       double RunParam::getSx(const int irun) {return m_sx[irun];}
       double RunParam::getSy(const int irun) {return m_sy[irun];}
       double RunParam::getSz(const int irun) {return m_sz[irun];}
       int RunParam::getIsam(const int irun) {return m_isam[irun];}
       double RunParam::getTemp(const int irun) {return m_temp[irun];}
       double RunParam::getEta(const int irun) {return m_eta[irun];}

       void RunParam::setEi(const int irun, const double val)
          {m_ei[irun]=val;}
       void RunParam::setPsi(const int irun, const double val)
          {m_psi[irun]=val;}
       void RunParam::setElo(const int irun, const double val)
          {m_elo[irun]=val;}
       void RunParam::setEhi(const int irun, const double val)
          {m_ehi[irun]=val;}
       void RunParam::setDe(const int irun, const double val)
          {m_de[irun]=val;}
       void RunParam::setX0(const int irun, const double val)
          {m_x0[irun]=val;}
       void RunParam::setXa(const int irun, const double val)
          {m_xa[irun]=val;}
       void RunParam::setX1(const int irun, const double val)
          {m_x1[irun]=val;}
       void RunParam::setWa(const int irun, const double val)
          {m_wa[irun]=val;}
       void RunParam::setHa(const int irun, const double val)
          {m_ha[irun]=val;}
       void RunParam::setS1(const int irun, const double val)
          {m_s1[irun]=val;}
       void RunParam::setS2(const int irun, const double val)
          {m_s2[irun]=val;}
       void RunParam::setS3(const int irun, const double val)
          {m_s3[irun]=val;}
       void RunParam::setS4(const int irun, const double val)
          {m_s4[irun]=val;}
       void RunParam::setS5(const int irun, const double val)
          {m_s5[irun]=val;}
       void RunParam::setThetam(const int irun, const double val)
          {m_thetam[irun]=val;}
       void RunParam::setImod(const int irun, const int val)
          {m_imod[irun]=val;}
       void RunParam::setPslit(const int irun, const double val)
          {m_pslit[irun]=val;}
       void RunParam::setRadius(const int irun, const double val)
          {m_radius[irun]=val;}
       void RunParam::setRho(const int irun, const double val)
          {m_rho[irun]=val;}
       void RunParam::setHz(const int irun, const double val)
          {m_hz[irun]=val;}
       void RunParam::setTjit(const int irun, const double val)
          {m_tjit[irun]=val;}
       void RunParam::setAs(const int irun, const double val)
          {m_as[irun]=val;}
       void RunParam::setBs(const int irun, const double val)
          {m_bs[irun]=val;}
       void RunParam::setCs(const int irun, const double val)
          {m_cs[irun]=val;}
       void RunParam::setAa(const int irun, const double val)
          {m_aa[irun]=val;}
       void RunParam::setBb(const int irun, const double val)
          {m_bb[irun]=val;}
       void RunParam::setCc(const int irun, const double val)
          {m_cc[irun]=val;}
       void RunParam::setUh(const int irun, const double val)
          {m_uh[irun]=val;}
       void RunParam::setUk(const int irun, const double val)
          {m_uk[irun]=val;}
       void RunParam::setUl(const int irun, const double val)
          {m_ul[irun]=val;}
       void RunParam::setVh(const int irun, const double val)
          {m_vh[irun]=val;}
       void RunParam::setVk(const int irun, const double val)
          {m_vk[irun]=val;}
       void RunParam::setVl(const int irun, const double val)
          {m_vl[irun]=val;}
       void RunParam::setOmega(const int irun, const double val)
          {m_omega[irun]=val;}
       void RunParam::setGs(const int irun, const double val)
          {m_gs[irun]=val;}
       void RunParam::setGl(const int irun, const double val)
          {m_gl[irun]=val;}
       void RunParam::setDpsi(const int irun, const double val)
          {m_dpsi[irun]=val;}
       void RunParam::setXh(const int irun, const double val)
          {m_xh[irun]=val;}
       void RunParam::setXk(const int irun, const double val)
          {m_xk[irun]=val;}
       void RunParam::setXl(const int irun, const double val)
          {m_xl[irun]=val;}
       void RunParam::setYh(const int irun, const double val)
          {m_yh[irun]=val;}
       void RunParam::setYk(const int irun, const double val)
          {m_yk[irun]=val;}
       void RunParam::setYl(const int irun, const double val)
          {m_yl[irun]=val;}
       void RunParam::setSx(const int irun, const double val)
          {m_sx[irun]=val;}
       void RunParam::setSy(const int irun, const double val)
          {m_sy[irun]=val;}
       void RunParam::setSz(const int irun, const double val)
          {m_sz[irun]=val;}
       void RunParam::setIsam(const int irun, const int val)
          {m_isam[irun]=val;}
       void RunParam::setTemp(const int irun, const double val)
          {m_temp[irun]=val;}
       void RunParam::setEta(const int irun, const double val)
          {m_eta[irun]=val;}

       void RunParam::readData(const std::string file){}
       void RunParam::writeData(const std::string file){}

       /// @endcond

    }
}
