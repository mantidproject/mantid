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

       RunParam::RunParam(double ei,double psi,double elo,double ehi,
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
          double eta) :

          m_ei(ei), m_psi(psi), m_elo(elo),m_ehi(ehi),
          m_de(de), m_x0(x0), m_xa(xa), m_x1(x1),
          m_wa(wa), m_ha(ha), m_s1(s1), m_s2(s2),
          m_s3(s3), m_s4(s4), m_s5(s5), m_thetam(thetam),
          m_imod(imod), m_pslit(pslit), m_radius(radius),
          m_rho(rho), m_hz(hz), m_tjit(tjit),
          m_as(as), m_bs(bs), m_cs(cs),
          m_aa(aa), m_bb(bb), m_cc(cc),
          m_uh(uh), m_uk(uk), m_ul(ul),
          m_vh(vh), m_vk(vk), m_vl(vl),
          m_omega(omega), m_gs(gs), m_gl(gl),
          m_dpsi(dpsi), m_xh(xh), m_xk(xk),
          m_xl(xl), m_yh(yh), m_yk(yk),
          m_yl(yl), m_sx(sx), m_sy(sy),
          m_sz(sz), m_isam(isam), m_temp(temp),
          m_eta(eta)
       {};

       RunParam::~RunParam()
       {}

       /// @cond
       double RunParam::getEi() {return m_ei;}
       double RunParam::getPsi() {return m_psi;}
       double RunParam::getElo() {return m_elo;}
       double RunParam::getEhi() {return m_ehi;}
       double RunParam::getDe() {return m_de;}
       double RunParam::getX0() {return m_x0;}
       double RunParam::getXa() {return m_xa;}
       double RunParam::getX1() {return m_x1;}
       double RunParam::getWa() {return m_wa;}
       double RunParam::getHa() {return m_ha;}
       double RunParam::getS1() {return m_s1;}
       double RunParam::getS2() {return m_s2;}
       double RunParam::getS3() {return m_s3;}
       double RunParam::getS4() {return m_s4;}
       double RunParam::getS5() {return m_s5;}
       double RunParam::getThetam() {return m_thetam;}
       int RunParam::getImod() {return m_imod;}
       double RunParam::getPslit() {return m_pslit;}
       double RunParam::getRadius() {return m_radius;}
       double RunParam::getRho() {return m_rho;}
       double RunParam::getHz() {return m_hz;}
       double RunParam::getTjit() {return m_tjit;}
       double RunParam::getAs() {return m_as;}
       double RunParam::getBs() {return m_bs;}
       double RunParam::getCs() {return m_cs;}
       double RunParam::getAa() {return m_aa;}
       double RunParam::getBb() {return m_bb;}
       double RunParam::getCc() {return m_cc;}
       double RunParam::getUh() {return m_uh;}
       double RunParam::getUk() {return m_uk;}
       double RunParam::getUl() {return m_ul;}
       double RunParam::getVh() {return m_vh;}
       double RunParam::getVk() {return m_vk;}
       double RunParam::getVl() {return m_vl;}
       double RunParam::getOmega() {return m_omega;}
       double RunParam::getGs() {return m_gs;}
       double RunParam::getGl() {return m_gl;}
       double RunParam::getDpsi() {return m_dpsi;}
       double RunParam::getXh() {return m_xh;}
       double RunParam::getXk() {return m_xk;}
       double RunParam::getXl() {return m_xl;}
       double RunParam::getYh() {return m_yh;}
       double RunParam::getYk() {return m_yk;}
       double RunParam::getYl() {return m_yl;}
       double RunParam::getSx() {return m_sx;}
       double RunParam::getSy() {return m_sy;}
       double RunParam::getSz() {return m_sz;}
       int RunParam::getIsam() {return m_isam;}
       double RunParam::getTemp() {return m_temp;}
       double RunParam::getEta() {return m_eta;}

       void RunParam::setEi(const double val)
          {m_ei=val;}
       void RunParam::setPsi(const double val)
          {m_psi=val;}
       void RunParam::setElo(const double val)
          {m_elo=val;}
       void RunParam::setEhi(const double val)
          {m_ehi=val;}
       void RunParam::setDe(const double val)
          {m_de=val;}
       void RunParam::setX0(const double val)
          {m_x0=val;}
       void RunParam::setXa(const double val)
          {m_xa=val;}
       void RunParam::setX1(const double val)
          {m_x1=val;}
       void RunParam::setWa(const double val)
          {m_wa=val;}
       void RunParam::setHa(const double val)
          {m_ha=val;}
       void RunParam::setS1(const double val)
          {m_s1=val;}
       void RunParam::setS2(const double val)
          {m_s2=val;}
       void RunParam::setS3(const double val)
          {m_s3=val;}
       void RunParam::setS4(const double val)
          {m_s4=val;}
       void RunParam::setS5(const double val)
          {m_s5=val;}
       void RunParam::setThetam(const double val)
          {m_thetam=val;}
       void RunParam::setImod(const int val)
          {m_imod=val;}
       void RunParam::setPslit(const double val)
          {m_pslit=val;}
       void RunParam::setRadius(const double val)
          {m_radius=val;}
       void RunParam::setRho(const double val)
          {m_rho=val;}
       void RunParam::setHz(const double val)
          {m_hz=val;}
       void RunParam::setTjit(const double val)
          {m_tjit=val;}
       void RunParam::setAs(const double val)
          {m_as=val;}
       void RunParam::setBs(const double val)
          {m_bs=val;}
       void RunParam::setCs(const double val)
          {m_cs=val;}
       void RunParam::setAa(const double val)
          {m_aa=val;}
       void RunParam::setBb(const double val)
          {m_bb=val;}
       void RunParam::setCc(const double val)
          {m_cc=val;}
       void RunParam::setUh(const double val)
          {m_uh=val;}
       void RunParam::setUk(const double val)
          {m_uk=val;}
       void RunParam::setUl(const double val)
          {m_ul=val;}
       void RunParam::setVh(const double val)
          {m_vh=val;}
       void RunParam::setVk(const double val)
          {m_vk=val;}
       void RunParam::setVl(const double val)
          {m_vl=val;}
       void RunParam::setOmega(const double val)
          {m_omega=val;}
       void RunParam::setGs(const double val)
          {m_gs=val;}
       void RunParam::setGl(const double val)
          {m_gl=val;}
       void RunParam::setDpsi(const double val)
          {m_dpsi=val;}
       void RunParam::setXh(const double val)
          {m_xh=val;}
       void RunParam::setXk(const double val)
          {m_xk=val;}
       void RunParam::setXl(const double val)
          {m_xl=val;}
       void RunParam::setYh(const double val)
          {m_yh=val;}
       void RunParam::setYk(const double val)
          {m_yk=val;}
       void RunParam::setYl(const double val)
          {m_yl=val;}
       void RunParam::setSx(const double val)
          {m_sx=val;}
       void RunParam::setSy(const double val)
          {m_sy=val;}
       void RunParam::setSz(const double val)
          {m_sz=val;}
       void RunParam::setIsam(const int val)
          {m_isam=val;}
       void RunParam::setTemp(const double val)
          {m_temp=val;}
       void RunParam::setEta(const double val)
          {m_eta=val;}

       void RunParam::readData(const std::string file)
       {
         UNUSED_ARG(file);
       }
       void RunParam::writeData(const std::string file)
       {
         UNUSED_ARG(file);
       }
    
       /// @endcond
    }
}
