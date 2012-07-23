#include "MantidMDEvents/MDWSTransform.h"
#include "MantidMDEvents/MDTransfAxisNames.h"
#include "MantidKernel/Strings.h"
#include <float.h>

namespace Mantid
{
namespace MDEvents
{
// logger for the algorithm workspaces  
Kernel::Logger& MDWSTransform::g_Log =Kernel::Logger::get("MD-Algorithms");
using namespace CnvrtToMD;

std::vector<double> MDWSTransform::getTransfMatrix(MDEvents::MDWSDescription &TargWSDescription,const std::string &QScaleRequested)const
{
  CoordScaling ScaleID = getQScaling(QScaleRequested);
  if(TargWSDescription.AlgID.compare("Q3D")==0)
  {
    this->setQ3DDimensionsNames(TargWSDescription,ScaleID);
  }

  return getTransfMatrix(TargWSDescription,ScaleID);
}

/** The matrix to convert neutron momentums into the target coordinate system   */
std::vector<double> MDWSTransform::getTransfMatrix(MDEvents::MDWSDescription &TargWSDescription,CoordScaling ScaleID)const
{

  Kernel::Matrix<double> mat(3,3,true);
  Kernel::Matrix<double> ub;

  bool powderMode = TargWSDescription.isPowder();

  bool has_lattice(true);
  if(!TargWSDescription.hasLattice())  has_lattice=false;

  if(!powderMode && (!has_lattice))
  {
    std::string inWsName = TargWSDescription.getWSName();
    // warn about 3D case without lattice
    g_Log.warning()<<
      " Can not obtain transformation matrix from the input workspace: "<<inWsName<<
      " as no oriented lattice has been defined. \n"
      " Will use unit transformation matrix\n";
  }
  //
  if(has_lattice)
  {

    TargWSDescription.m_Wtransf = buildQTrahsf(TargWSDescription,ScaleID);
    // Obtain the transformation matrix to Cartezian related to Crystal
    mat = TargWSDescription.m_GoniomMatr*TargWSDescription.m_Wtransf;
    // and this is the transformation matrix to notional
    //mat = gon*Latt.getUB();
    mat.Invert();
  }


  std::vector<double> rotMat = mat.getVector();
  return rotMat;
}


Kernel::DblMatrix MDWSTransform::buildQTrahsf(MDEvents::MDWSDescription &TargWSDescription,CnvrtToMD::CoordScaling ScaleID)const
{
  //implements strategy Q=R*U*B*W*h where W-transf is W or WB or W*Unit*Lattice_param depending on inputs:
  if(!TargWSDescription.hasLattice()){      
    throw(std::invalid_argument("this funcntion should be called only on workspace with defined oriented lattice"));
  }

  // if u,v us default, Wmat is unit transformation
  Kernel::DblMatrix  Wmat(3,3,true);
  // derive rotation from u0,v0 u0||ki to u,v
  if(!m_isUVdefault)
  {  
    Wmat[0][0]=m_UProj[0];
    Wmat[1][0]=m_UProj[1];
    Wmat[2][0]=m_UProj[2];
    Wmat[0][1]=m_VProj[0];
    Wmat[1][1]=m_VProj[1];
    Wmat[2][1]=m_VProj[2];
    Wmat[0][2]=m_WProj[0];
    Wmat[1][2]=m_WProj[1];
    Wmat[2][2]=m_WProj[2];
  }
  if(ScaleID==OrthogonalHKLScale)
  {
    std::vector<Kernel::V3D> dim_directions;
    std::vector<Kernel::V3D> uv(2);
    uv[0]=m_UProj;
    uv[1]=m_VProj;
    dim_directions = Kernel::V3D::makeVectorsOrthogonal(uv);
    for(size_t i=0;i<3;++i)
      for (size_t j=0;j<3;++j)
        Wmat[i][j]=dim_directions[j][i];
  }
  Kernel::DblMatrix Scale(3,3,true);
  boost::shared_ptr<Geometry::OrientedLattice> spLatt = TargWSDescription.getLattice();
  switch (ScaleID)
  {
  case NoScaling:    //< momentums in A^-1
    {
      break;
    }
  case SingleScale: //< momentuns divided by  2*Pi/Lattice -- equivalend to d-spacing in some sense
    {
      double dMax(-1.e+32);
      for(int i=0;i<3;i++)  dMax =(dMax>spLatt->a(i))?(dMax):(spLatt->a(i));
      for(int i=0;i<3;i++)  Scale[i][i] = (2*M_PI)/dMax;

      break;
    }
  case OrthogonalHKLScale://< each momentum component divided by appropriate lattice parameter; equivalent to hkl for orthogonal axis
    {
      if(TargWSDescription.hasLattice())
        for(int i=0;i<3;i++){ Scale[i][i] = (2*M_PI)/spLatt->a(i);}             
        break;
    }
  case HKLScale:   //< non-orthogonal system for non-orthogonal lattice
    {
      if(TargWSDescription.hasLattice()) Scale = spLatt->getUB()*(2*M_PI);
      break;
    }

  default: throw(std::invalid_argument("unrecognized conversion mode"));

  }

  return Scale*Wmat;
}

/** Build meaningful dimension names for different conversion modes
*/
void MDWSTransform::setQ3DDimensionsNames(MDEvents::MDWSDescription &TargWSDescription,const std::string &QScaleRequested)const
{
  //axis units: convert string representation to any availible
  CoordScaling ScaleID = getQScaling(QScaleRequested);
  this->setQ3DDimensionsNames(TargWSDescription,ScaleID);

}

void MDWSTransform::setQ3DDimensionsNames(MDEvents::MDWSDescription &TargWSDescription,CnvrtToMD::CoordScaling ScaleID)const
{

  std::vector<Kernel::V3D> dim_directions;
  // set default dimension names:
  std::vector<std::string> dim_names = TargWSDescription.getDimNames();

  // define B-matrix and Lattice parameters to one in case if no OrientedLattice is there
  Kernel::DblMatrix Bm(3,3,true);
  std::vector<double> LatPar(3,1);
  if(TargWSDescription.hasLattice())
  { // redefine B-matrix and Lattice parameters from real oriented lattice if there is one
    auto spLatt = TargWSDescription.getLattice();
    Bm=spLatt->getB();
    for(int i=0;i<3;i++)LatPar[i]=spLatt->a(i);
  }

  dim_names[0]="H";
  dim_names[1]="K";
  dim_names[2]="L";

  dim_directions.resize(3);
  dim_directions[0]=m_UProj;
  dim_directions[1]=m_VProj;
  dim_directions[2]=m_WProj;
  if(ScaleID==OrthogonalHKLScale)
  {
    std::vector<Kernel::V3D> uv(2);
    uv[0]=m_UProj;
    uv[1]=m_VProj;
    dim_directions = Kernel::V3D::makeVectorsOrthogonal(uv);
  }
  // axis names:
  for(int i=0;i<3;i++)TargWSDescription.setDimName(i,MDEvents::makeAxisName(dim_directions[i],dim_names));

  if (ScaleID == NoScaling)
  {
    for(int i=0;i<3;i++)TargWSDescription.setDimUnit(i,"A^-1");
  }
  if(ScaleID==SingleScale)
  {
    double dMax(-1.e+32);
    for(int i=0;i<3;i++)dMax =(dMax>LatPar[i])?(dMax):(LatPar[i]);
    for(int i=0;i<3;i++)TargWSDescription.setDimUnit(i,"in "+MDEvents::sprintfd(2*M_PI/dMax,1.e-3)+" A^-1");
  }
  if((ScaleID==OrthogonalHKLScale)||(ScaleID==HKLScale))
  {
    //get the length along each of the axes
    std::vector<double> len;
    Kernel::V3D x;
    x=Bm*dim_directions[0];
    len.push_back(2*M_PI*x.norm());
    x=Bm*dim_directions[1];
    len.push_back(2*M_PI*x.norm());
    x=Bm*dim_directions[2];
    len.push_back(2*M_PI*x.norm());
    for(int i=0;i<3;i++)TargWSDescription.setDimUnit(i,"in "+MDEvents::sprintfd(len[i],1.e-3)+" A^-1");
  }

}

void MDWSTransform::setModQDimensionsNames(MDEvents::MDWSDescription &TargWSDescription,const std::string &QScaleRequested)const
{ //TODO: nothing meanigful has been done at the moment, should enable scaling if different coord transf modes?

  UNUSED_ARG(TargWSDescription);
  UNUSED_ARG(QScaleRequested);
}
/** check if input vector is defined */
bool MDWSTransform::v3DIsDefault(const std::vector<double> &vect,const std::string &message)const
{
  bool def = true;
  if(!vect.empty())
  {

    if(vect.size()==3){ 
      def =false;
    }else{
        g_Log.warning() << message;
    }
  }
  return def;
}
//
void  MDWSTransform::setUVvectors(const std::vector<double> &ut,const std::vector<double> &vt, const std::vector<double> &wt)
{   
  //identify if u,v are present among input parameters and use defaults if not
  bool u_default(true),v_default(true),w_default(true);

  u_default = v3DIsDefault(ut," u projection vector specified but its dimensions are not equal to 3, using default values [1,0,0]\n");
  v_default = v3DIsDefault(vt," v projection vector specified but its dimensions are not equal to 3, using default values [0,1,0]\n");
  w_default = v3DIsDefault(wt," w projection vector specified but its dimensions are not equal to 3, using default values [0,0,1]\n");

  if(u_default){ m_UProj = Kernel::V3D(1.,0.,0.);
  }else{ m_UProj = Kernel::V3D(ut[0], ut[1], ut[2]);
  }

  if(v_default){m_VProj = Kernel::V3D(0.,1.,0.);
  }else{ m_VProj = Kernel::V3D(vt[0], vt[1], vt[2]);
  }

  if(w_default){m_WProj = Kernel::V3D(0.,0.,1.);
  }else{ m_WProj = Kernel::V3D(wt[0], wt[1], wt[2]);
  }

  m_isUVdefault  = u_default&&v_default&&w_default;

  //check if u, v, w are coplanar
  if (fabs((m_UProj.cross_prod(m_VProj)).scalar_prod(m_WProj))<Kernel::Tolerance)
  {
    m_UProj = Kernel::V3D(1.,0.,0.);
    m_VProj = Kernel::V3D(0.,1.,0.);
    m_WProj = Kernel::V3D(0.,0.,1.);
    m_isUVdefault=true;
    throw std::invalid_argument("Projections are coplanar");
  }
}
/** function which convert input string representing coordinate scaling to correspondent enum */
CoordScaling MDWSTransform::getQScaling(const std::string &ScID)const
{
  int nScaling = Kernel::Strings::isMember(m_QScalingID,ScID);

  if (nScaling<0)throw(std::invalid_argument(" The Q scale with ID: "+ScID+" is unavalible"));

  return CoordScaling(nScaling);
}

//
MDWSTransform::MDWSTransform():
  m_isUVdefault(true),
  m_QScalingID(NCoordScalings)
{
  m_UProj[0] = 1;   m_UProj[1] = 0;  m_UProj[2] = 0;
  m_VProj[0] = 0;   m_VProj[1] = 1;  m_VProj[2] = 0;
  m_WProj[0] = 0;   m_WProj[1] = 0;  m_WProj[2] = 1;


  m_QScalingID[NoScaling]="Q in A^-1";
  m_QScalingID[SingleScale]="Q in lattice units";
  m_QScalingID[OrthogonalHKLScale]="Orthogonal HKL";
  m_QScalingID[HKLScale]="HKL";

}

}
}
