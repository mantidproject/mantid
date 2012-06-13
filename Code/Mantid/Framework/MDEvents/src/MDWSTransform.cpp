#include "MantidMDEvents/MDWSTransform.h"
#include "MantidMDEvents/MDTransfAxisNames.h"
#include "MantidKernel/Strings.h"
#include <float.h>

namespace Mantid
{
namespace MDEvents
{
// logger for the algorithm workspaces  
Kernel::Logger& MDWSTransform::convert_log =Kernel::Logger::get("MD-Algorithms");
using namespace ConvertToMD;

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
        convert_log.warning()<<
        " Can not obtain transformation matrix from the input workspace: "<<inWsName<<
        " as no oriented lattice has been defined. \n"
        " Will use unit transformation matrix\n";
    }
    //
    if(has_lattice){

      TargWSDescription.Wtransf = buildQTrahsf(TargWSDescription,ScaleID);
      // Obtain the transformation matrix to Cartezian related to Crystal
      mat = TargWSDescription.GoniomMatr*TargWSDescription.Wtransf;
     // and this is the transformation matrix to notional
     //mat = gon*Latt.getUB();
      mat.Invert();
    }


    std::vector<double> rotMat = mat.getVector();
    return rotMat;
}


Kernel::DblMatrix MDWSTransform::buildQTrahsf(MDEvents::MDWSDescription &TargWSDescription,ConvertToMD::CoordScaling ScaleID)const
{
    //implements strategy Q=R*U*B*W*h where W-transf is W or WB or W*Unit*Lattice_param depending on inputs:
    if(!TargWSDescription.hasLattice()){      
        throw(std::invalid_argument("this funcntion should be called only on workspace with defined oriented lattice"));
    }

    // if u,v us default, Wmat is unit transformation
    Kernel::DblMatrix  Wmat(3,3,true);
    // derive rotation from u0,v0 u0||ki to u,v
    if(!is_uv_default)
    {  
        Wmat[0][0]=uProj[0];
        Wmat[1][0]=uProj[1];
        Wmat[2][0]=uProj[2];
        Wmat[0][1]=vProj[0];
        Wmat[1][1]=vProj[1];
        Wmat[2][1]=vProj[2];
        Wmat[0][2]=wProj[0];
        Wmat[1][2]=wProj[1];
        Wmat[2][2]=wProj[2];
    }
    if(ScaleID==OrthogonalHKLScale)
    {
        std::vector<Kernel::V3D> dim_directions;
        std::vector<Kernel::V3D> uv(2);
        uv[0]=uProj;
        uv[1]=vProj;
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

void MDWSTransform::setQ3DDimensionsNames(MDEvents::MDWSDescription &TargWSDescription,ConvertToMD::CoordScaling ScaleID)const
{
        
        std::vector<Kernel::V3D> dim_directions;
        // set default dimension names:
        std::vector<std::string> dim_names = TargWSDescription.getDimNames();

        // define B-matrix and Lattice parameters to one in case if no OrientedLattice is there
        Kernel::DblMatrix Bm(3,3,true);
        std::vector<double> LatPar(3,1);
        if(TargWSDescription.hasLattice())
        { // redefine B-matrix and Lattice parameters from real oriented lattice if there is one
            boost::shared_ptr<Geometry::OrientedLattice> spLatt = TargWSDescription.getLattice();
            Bm=spLatt->getB();
            for(int i=0;i<3;i++)LatPar[i]=spLatt->a(i);
        }
  
        dim_names[0]="H";
        dim_names[1]="K";
        dim_names[2]="L";

        dim_directions.resize(3);
        dim_directions[0]=uProj;
        dim_directions[1]=vProj;
        dim_directions[2]=wProj;
        if(ScaleID==OrthogonalHKLScale)
        {
            std::vector<Kernel::V3D> uv(2);
            uv[0]=uProj;
            uv[1]=vProj;
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

//
void  MDWSTransform::setUVvectors(const std::vector<double> &ut,const std::vector<double> &vt, const std::vector<double> &wt)
{   
//identify if u,v are present among input parameters and use defaults if not
    bool u_default(true),v_default(true),w_default(true);
    if(!ut.empty()){
        if(ut.size()==3){ u_default =false;
        }else{convert_log.warning() <<" u projection vector specified but its dimensions are not equal to 3, using default values [1,0,0]\n";
        }
    }
    if(!vt.empty()){
        if(vt.size()==3){ v_default  =false;
        }else{ convert_log.warning() <<" v projection vector specified but its dimensions are not equal to 3, using default values [0,1,0]\n";
        }
    }
    if(!wt.empty()){
        if(wt.size()==3){ w_default  =false;
        }else{ convert_log.warning() <<" w projection vector specified but its dimensions are not equal to 3, using default values [0,0,1]\n";
        }
    }
    if(u_default){
        uProj = Kernel::V3D(1.,0.,0.);
    }else{
        uProj = Kernel::V3D(ut[0], ut[1], ut[2]);
    }
    if(v_default){
        vProj = Kernel::V3D(0.,1.,0.);
    }else{
        vProj = Kernel::V3D(vt[0], vt[1], vt[2]);
    }
    if(w_default){
        wProj = Kernel::V3D(0.,0.,1.);
    }else{
        wProj = Kernel::V3D(wt[0], wt[1], wt[2]);
    }
    if(u_default&&v_default&&w_default){
        is_uv_default=true;
    }else{
        is_uv_default=false;
    }
    //check if u, v, w are coplanar
    if (fabs((uProj.cross_prod(vProj)).scalar_prod(wProj))<Kernel::Tolerance)
    {
        uProj = Kernel::V3D(1.,0.,0.);
        vProj = Kernel::V3D(0.,1.,0.);
        wProj = Kernel::V3D(0.,0.,1.);
        is_uv_default=true;
        throw std::invalid_argument("Projections are coplanar");
    }
}
/** function which convert input string representing coordinate scaling to correspondent enum */
CoordScaling MDWSTransform::getQScaling(const std::string &ScID)const
{
    int nScaling = Kernel::Strings::isMember(QScalingID,ScID);

    if (nScaling<0)throw(std::invalid_argument(" The Q scale with ID: "+ScID+" is unavalible"));

    return CoordScaling(nScaling);
}

//
MDWSTransform::MDWSTransform():
is_uv_default(true),
QScalingID(NCoordScalings)
{
    uProj[0] = 1;        uProj[1] = 0;     uProj[2] = 0;
    vProj[0] = 0;        vProj[1] = 1;     vProj[2] = 0;
    wProj[0] = 0;        wProj[1] = 0;     wProj[2] = 1;

 
    QScalingID[NoScaling]="Q in A^-1";
    QScalingID[SingleScale]="Q in lattice units";
    QScalingID[OrthogonalHKLScale]="Orthogonal HKL";
    QScalingID[HKLScale]="HKL";

}

}
}
