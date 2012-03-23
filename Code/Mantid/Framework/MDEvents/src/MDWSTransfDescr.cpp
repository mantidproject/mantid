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
    Kernel::Matrix<double> umat;
    

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
      if(is_uv_default) {
         // we need to set up u,v for axis caption as it is defined in workspace UB matrix;
         uProj = TargWSDescription.pLatt->getuVector();
         vProj = TargWSDescription.pLatt->getvVector();      
      }
      umat  = TargWSDescription.pLatt->getU();
      TargWSDescription.Wtransf = buildQTrahsf(TargWSDescription);
      // Obtain the transformation matrix to Cartezian related to Crystal
      mat = TargWSDescription.GoniomMatr*umat*TargWSDescription.Wtransf;
     // and this is the transformation matrix to notional
     //mat = gon*Latt.getUB();
      mat.Invert();
    }


    std::vector<double> rotMat = mat.getVector();
    return rotMat;
}
/** The matrix to convert neutron momentums into the target coordinate system, if the target coordinate system is already defined by existing 
  *   MD workspace */
std::vector<double> MDWSTransfDescr::getTransfMatrix(const std::string &inWsName, API::IMDEventWorkspace_sptr spws,const MDEvents::MDWSDescription &TargWSDescription,bool powderMode)const
{

   bool has_lattice(true);
   if(!TargWSDescription.pLatt.get())  has_lattice=false;

   if(!powderMode && (!has_lattice))
   {   // warn about 3D case without lattice
        convert_log.warning()<<
        " Can not obtain transformation matrix from the input workspace: "<<inWsName<<
        " as no oriented lattice has been defined. \n"
        " Will use unit transformation matrix\n";
   }

   Kernel::DblMatrix  umat(3,3,true);
   if(has_lattice) umat  = TargWSDescription.pLatt->getU();

   Kernel::DblMatrix mat    = TargWSDescription.GoniomMatr*umat*spws->getWTransf();

   mat.Invert();

   return mat.getVector();

}


std::vector<Kernel::V3D>  MDWSTransfDescr::buildOrtho3D(const Kernel::DblMatrix &BM,const Kernel::V3D &u, const Kernel::V3D &v)const
{
    std::vector<Kernel::V3D> dim_directions(3);
    dim_directions[0]=BM*u;
    Kernel::V3D vp   =BM*v;
    dim_directions[2]=dim_directions[0].cross_prod(vp);
    if(dim_directions[2].norm2()<FLT_EPSILON)
    {
        throw(std::invalid_argument("two input vecores u and v can not be parallel"));
    }
    dim_directions[0].normalize();
    dim_directions[2].normalize();

    dim_directions[1]= dim_directions[2].cross_prod(dim_directions[0]);
    dim_directions[1].normalize();
    

    return dim_directions;
}


Kernel::DblMatrix MDWSTransfDescr::buildQTrahsf(MDEvents::MDWSDescription &TargWSDescription)const
{
    //implements strategy Q=R*U*W*B*h where W-transf is W or WB or W*Unit*Lattice_param depending on inputs:
    if(!TargWSDescription.pLatt.get()){      
        throw(std::invalid_argument("this funcntion should be called only on workspace with defined oriented lattice"));
    }

    CoordScaling ScaleID = TargWSDescription.convert_to_factor;

    // if u,v us default, Wmat is unit transformation
    Kernel::DblMatrix  Wmat(3,3,true);
    // derive rotation from u0,v0 u0||ki to u,v
    if(!is_uv_default)
    {  
        Kernel::DblMatrix  U0 = TargWSDescription.pLatt->getU();
        Geometry::OrientedLattice FakeLattice;
        FakeLattice.setUFromVectors(uProj,vProj);
        Kernel::DblMatrix  U1  = FakeLattice.getU();
        // U1 = W*U0? --- rotation from u,v to uProj, vProj
        Wmat = U1*U0.Invert();
        // debug
        //std::vector<double> wr=Wmat.getVector();
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
          for(int i=0;i<3;i++) Scale[i][i] = (2*M_PI)/TargWSDescription.pLatt->a(i);          
          break;
        }
    case HKLScale:   //< non-orthogonal system for non-orthogonal lattice
        {
            if(TargWSDescription.pLatt.get()) Scale = TargWSDescription.pLatt->getB()*(2*M_PI);           
            break;
        }

    default: throw(std::invalid_argument("unrecognized conversion mode"));

    }
    return Wmat*Scale;
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
  
        
        if(ScaleID==HKLScale)
        { // axis for HKL case are just along the directions requested
            dim_names[0]="H";
            dim_names[1]="K";
            dim_names[2]="L";

            dim_directions.resize(3);
            dim_directions[0]=uProj;
            dim_directions[1]=vProj;
            dim_directions[2]=uProj.cross_prod(vProj);        
        }else
        { // in any other (orthogonal) case, the axis are build around projection vectors (either they are parallel to beam or not)
          // correct setup of these vectors has been already done
            dim_names[0]="Qh";
            dim_names[1]="Qk";
            dim_names[2]="Ql";

            dim_directions = this->buildOrtho3D(Bm,uProj,vProj);
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
        if(ScaleID==OrthogonalHKLScale)
        {
                for(int i=0;i<3;i++)TargWSDescription.dimUnits[i] = "in "+MDEvents::sprintfd(2*M_PI/LatPar[i],1.e-3)+" A^-1";
        }
        if(ScaleID==HKLScale)
        {
                for(int i=0;i<3;i++)TargWSDescription.dimUnits[i] = "";
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
void  MDWSTransfDescr::getUVsettings(const std::vector<double> &ut,const std::vector<double> &vt)
{   
//identify if u,v are present among input parameters and use defaults if not
    bool u_default(true),v_default(true);
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
    if(u_default){  
        uProj[0] = 1;         uProj[1] = 0;        uProj[2] = 0;
    }else{
        uProj[0] = ut[0];     uProj[1] = ut[1];    uProj[2] = ut[2];
    }
    if(v_default){
        vProj[0] = 0;         vProj[1] = 1;        vProj[2] = 0;
    }else{
        vProj[0] = vt[0];     vProj[1] = vt[1];    vProj[2] = vt[2];
    }
    if(u_default&&v_default){
        is_uv_default=true;
    }else{
        is_uv_default=false;
    }
}
//
MDWSTransfDescr::MDWSTransfDescr():
is_uv_default(true)
{
    uProj[0] = 1;        uProj[1] = 0;     uProj[2] = 0;
    vProj[0] = 0;        vProj[1] = 1;     vProj[2] = 0;
}

}
}