#include "MantidMDEvents/MDWSTransfDescr.h"
#include <float.h>

namespace Mantid
{
namespace MDEvents
{
// logger for the algorithm workspaces  
Kernel::Logger& MDWSTransfDescr::convert_log =Kernel::Logger::get("MD-Algorithms");


/** The matrix to convert neutron momentums into the target coordinate system   */
std::vector<double> MDWSTransfDescr::getTransfMatrix(const std::string &inWsName,MDEvents::MDWSDescription &TargWSDescription,bool powderMode)const
{
  
    Kernel::Matrix<double> mat(3,3,true);
    Kernel::Matrix<double> ub;
    

    bool has_lattice(true);
    if(!TargWSDescription.pLatt.get())  has_lattice=false;

    if(!powderMode && (!has_lattice))
    {
    // warn about 3D case without lattice
        convert_log.warning()<<
        " Can not obtain transformation matrix from the input workspace: "<<inWsName<<
        " as no oriented lattice has been defined. \n"
        " Will use unit transformation matrix\n";
    }
    //
    if(has_lattice){

      TargWSDescription.Wtransf = buildQTrahsf(TargWSDescription);
      // Obtain the transformation matrix to Cartezian related to Crystal
      mat = TargWSDescription.GoniomMatr*TargWSDescription.Wtransf;
     // and this is the transformation matrix to notional
     //mat = gon*Latt.getUB();
      mat.Invert();
    }


    std::vector<double> rotMat = mat.getVector();
    return rotMat;
}


Kernel::DblMatrix MDWSTransfDescr::buildQTrahsf(MDEvents::MDWSDescription &TargWSDescription)const
{
    //implements strategy Q=R*U*B*W*h where W-transf is W or WB or W*Unit*Lattice_param depending on inputs:
    if(!TargWSDescription.pLatt.get()){      
        throw(std::invalid_argument("this funcntion should be called only on workspace with defined oriented lattice"));
    }

    CoordScaling ScaleID = TargWSDescription.convert_to_factor;

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
    switch (ScaleID)
    {
    case NoScaling:    //< momentums in A^-1
        {
              break;
        }
    case SingleScale: //< momentuns divided by  2*Pi/Lattice -- equivalend to d-spacing in some sense
        {
          double dMax(-1.e+32);
          for(int i=0;i<3;i++)  dMax =(dMax>TargWSDescription.pLatt->a(i))?(dMax):(TargWSDescription.pLatt->a(i));
          for(int i=0;i<3;i++)  Scale[i][i] = (2*M_PI)/dMax;
         
          break;
        }
    case OrthogonalHKLScale://< each momentum component divided by appropriate lattice parameter; equivalent to hkl for orthogonal axis
        {
          if(TargWSDescription.pLatt.get()) Scale= TargWSDescription.pLatt->getUB()*(2*M_PI);
          break;
        }
    case HKLScale:   //< non-orthogonal system for non-orthogonal lattice
        {
          if(TargWSDescription.pLatt.get()) Scale = TargWSDescription.pLatt->getUB()*(2*M_PI);
          break;
        }

    default: throw(std::invalid_argument("unrecognized conversion mode"));

    }

    return Scale*Wmat;
}

/** Build meaningful dimension names for different conversion modes
  */
void MDWSTransfDescr::setQ3DDimensionsNames(MDEvents::MDWSDescription &TargWSDescription)
{
        
        std::vector<Kernel::V3D> dim_directions;
        // set default dimension names:
        std::vector<std::string> dim_names = TargWSDescription.getDefaultDimIDQ3D(TargWSDescription.emode);
        if(TargWSDescription.dimNames.size()<=dim_names.size()){
            TargWSDescription.dimNames         = dim_names;
        }else{
            for(size_t i=0;i<dim_names.size();i++)
            {
                 TargWSDescription.dimNames[i]    = dim_names[i];
            }
        }      

        // define B-matrix and Lattice parameters to one in case if no OrientedLattice is there
        Kernel::DblMatrix Bm(3,3,true);
        std::vector<double> LatPar(3,1);
        if(TargWSDescription.pLatt.get())
        { // redefine B-matrix and Lattice parameters from real oriented lattice if there is one
            Bm=TargWSDescription.pLatt->getB();
            for(int i=0;i<3;i++)LatPar[i]=TargWSDescription.pLatt->a(i);
        }
       // axis units:
        CoordScaling ScaleID = TargWSDescription.convert_to_factor;
  
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
        for(int i=0;i<3;i++)TargWSDescription.dimNames[i]=MDEvents::makeAxisName(dim_directions[i],dim_names);

        if (ScaleID == NoScaling)
        {
            for(int i=0;i<3;i++)TargWSDescription.dimUnits[i] = "A^-1";
        }
        if(ScaleID==SingleScale)
        {
            double dMax(-1.e+32);
            for(int i=0;i<3;i++)dMax =(dMax>LatPar[i])?(dMax):(LatPar[i]);
            for(int i=0;i<3;i++)TargWSDescription.dimUnits[i] = "in "+MDEvents::sprintfd(2*M_PI/dMax,1.e-3)+" A^-1";
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
            for(int i=0;i<3;i++)TargWSDescription.dimUnits[i] = "in "+MDEvents::sprintfd(len[i],1.e-3)+" A^-1";
        }
 
}

void MDWSTransfDescr::setModQDimensionsNames(MDEvents::MDWSDescription &TargWSDescription)
{ //TODO: nothing meanigful has been done at the moment, should enable scaling if different coord transf modes

   
    // set default dimension names:
    std::vector<std::string> dimNames = TargWSDescription.getDefaultDimIDModQ(TargWSDescription.emode);
    if(TargWSDescription.dimNames.size()<=dimNames.size()){
        TargWSDescription.dimNames         = dimNames;
    }else{
        for(size_t i=0;i<dimNames.size();i++)
        {
             TargWSDescription.dimNames[i]    = dimNames[i];
        }
    }
    
}

//
void  MDWSTransfDescr::getUVsettings(const std::vector<double> &ut,const std::vector<double> &vt, const std::vector<double> &wt)
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
//
MDWSTransfDescr::MDWSTransfDescr():
is_uv_default(true)
{
    uProj[0] = 1;        uProj[1] = 0;     uProj[2] = 0;
    vProj[0] = 0;        vProj[1] = 1;     vProj[2] = 0;
    wProj[0] = 0;        wProj[1] = 0;     wProj[2] = 1;
}

}
}
