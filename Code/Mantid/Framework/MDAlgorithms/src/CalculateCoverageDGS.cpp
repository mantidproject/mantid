#include "MantidMDAlgorithms/CalculateCoverageDGS.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/ListValidator.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/VectorHelper.h"

namespace Mantid
{
namespace MDAlgorithms
{
  using namespace Mantid::Kernel;
  using Mantid::API::WorkspaceProperty;
  using namespace Mantid::API;
  using namespace Mantid::MDEvents;
  using namespace Mantid::Geometry;

  namespace
  {
    //function to compare two intersections (h,k,l,Momentum) by scattered momentum
    bool compareMomentum(const Mantid::Kernel::VMD &v1, const Mantid::Kernel::VMD &v2)
    {
      return (v1[3] < v2[3]);
    }
  }
  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(CalculateCoverageDGS)



  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  CalculateCoverageDGS::CalculateCoverageDGS():
      m_hmin(0.f), m_hmax(0.f), m_kmin(0.f), m_kmax(0.f), m_lmin(0.f), m_lmax(0.f),
      m_dEmin(0.f), m_dEmax(0.f), m_Ei(0.),m_ki(0.), m_kfmin(0.), m_kfmax(0.),
      m_hIntegrated(false), m_kIntegrated(false), m_lIntegrated(false), m_dEIntegrated(false),
      m_hX(), m_kX(), m_lX(),m_eX(),m_hIdx(-1), m_kIdx(-1), m_lIdx(-1), m_eIdx(-1),
      m_rubw(3,3),m_normWS()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  CalculateCoverageDGS::~CalculateCoverageDGS()
  {
  }


  //----------------------------------------------------------------------------------------------

  /// Algorithms name for identification. @see Algorithm::name
  const std::string CalculateCoverageDGS::name() const { return "CalculateCoverageDGS"; }

  /// Algorithm's version for identification. @see Algorithm::version
  int CalculateCoverageDGS::version() const { return 1;};

  /// Algorithm's category for identification. @see Algorithm::category
  const std::string CalculateCoverageDGS::category() const { return "Inelastic;MDAlgorithms";}

  /// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
  const std::string CalculateCoverageDGS::summary() const { return "Calculate the reciprocal space coverage for direct geometry spectrometers";};


  /**
 * Stores the X values from each H,K,L dimension as member variables
 */
  void CalculateCoverageDGS::cacheDimensionXValues()
  {
    const double energyToK = 8.0*M_PI*M_PI*PhysicalConstants::NeutronMass*PhysicalConstants::meV*1e-20 /
        (PhysicalConstants::h*PhysicalConstants::h);

    auto &hDim = *m_normWS->getDimension(m_hIdx);
    m_hX.resize(hDim.getNBins());
    for(size_t i = 0; i < m_hX.size(); ++i)
    {
      m_hX[i] = hDim.getX(i);
    }
    auto &kDim = *m_normWS->getDimension(m_kIdx);
    m_kX.resize( kDim.getNBins() );
    for(size_t i = 0; i < m_kX.size(); ++i)
    {
      m_kX[i] = kDim.getX(i);
    }
    auto &lDim = *m_normWS->getDimension(m_lIdx);
    m_lX.resize( lDim.getNBins() );
    for(size_t i = 0; i < m_lX.size(); ++i)
    {
      m_lX[i] = lDim.getX(i);
    }
    //NOTE: store k final instead
    auto &eDim = *m_normWS->getDimension(m_eIdx);
    m_eX.resize( eDim.getNBins() );
    for(size_t i = 0; i < m_eX.size(); ++i)
    {
      m_eX[i] = std::sqrt(energyToK*(m_Ei-eDim.getX(i)));
    }
   }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void CalculateCoverageDGS::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Mantid::Kernel::Direction::Input, boost::make_shared<InstrumentValidator>()), "An input workspace.");

    auto mustBe3D = boost::make_shared<ArrayLengthValidator<double> >(3);
    auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
    mustBePositive->setLower(0.0);

    std::vector<double>  Q1(3,0.),Q2(3,0),Q3(3,0);
    Q1[0]=1.;Q2[1]=1.;Q3[2]=1.;
    declareProperty(new ArrayProperty<double>("Q1Basis",Q1,mustBe3D),"Q1 projection direction in the x,y,z format. Q1, Q2, Q3 must not be coplanar");
    declareProperty(new ArrayProperty<double>("Q2Basis",Q2,mustBe3D),"Q2 projection direction in the x,y,z format. Q1, Q2, Q3 must not be coplanar");
    declareProperty(new ArrayProperty<double>("Q3Basis",Q3,mustBe3D),"Q3 projection direction in the x,y,z format. Q1, Q2, Q3 must not be coplanar");
    declareProperty(new PropertyWithValue<double>("IncidentEnergy",EMPTY_DBL(),mustBePositive,Mantid::Kernel::Direction::Input),"Incident energy. If set, will override Ei in the input workspace");

    std::vector<std::string> options;
    options.push_back("Q1");
    options.push_back("Q2");
    options.push_back("Q3");
    options.push_back("DeltaE");

    for(int i=1;i<=4;i++)
    {
        std::string dim("Dimension");
        dim+=Kernel::toString(i);
        declareProperty(dim,options[i-1],boost::make_shared<StringListValidator>(options),"Dimension to bin or integrate");
        declareProperty(dim+"Min",EMPTY_DBL(),dim+" minimum value. If empty will take minimum possible value.");
        declareProperty(dim+"Max",EMPTY_DBL(),dim+" maximum value. If empty will take maximum possible value.");
        declareProperty(dim+"Step",EMPTY_DBL(),dim+" step size. If empty the dimension will be integrated between minimum and maximum values");
    }
    declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Mantid::Kernel::Direction::Output), "A name for the output data MDHistoWorkspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void CalculateCoverageDGS::exec()
  {
      const double energyToK = 8.0*M_PI*M_PI*PhysicalConstants::NeutronMass*PhysicalConstants::meV*1e-20 /
          (PhysicalConstants::h*PhysicalConstants::h);
      //get the limits
      Mantid::API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
      //cache two theta and phi
      auto instrument=inputWS->getInstrument();
      std::vector<detid_t> detIDS=instrument->getDetectorIDs(true);
      std::vector<double> tt,phi;
      for(int i=0;i<static_cast<int>(detIDS.size());i++)
      {
          auto detector=instrument->getDetector(detIDS[i]);
          tt.push_back(detector->getTwoTheta(V3D(0,0,0),V3D(0,0,1)));
          phi.push_back(detector->getPhi());
      }

      double ttmax=*(std::max_element(tt.begin(),tt.end()));
      m_Ei=getProperty("IncidentEnergy");
      if (m_Ei==EMPTY_DBL())
      {
          if (inputWS->run().hasProperty("Ei"))
          {
              Kernel::Property* eiprop = inputWS->run().getProperty("Ei");
              m_Ei = boost::lexical_cast<double>(eiprop->value());
              if(m_Ei<=0)
              {
                 throw std::invalid_argument("Ei stored in the workspace is not positive");
              }
          }
          else
          {
           throw std::invalid_argument("Could not find Ei in the workspace. Please enter a positive value");
          }
      }

      //Check if there is duplicate elements in dimension selection, and calculate the affine matrix
      Kernel::Matrix<coord_t> affineMat(4,4);
      double q1min(0.),q1max(0.),q2min(0.),q2max(0.),q3min(0.),q3max(0.);
      double q1step(0.),q2step(0.),q3step(0.),dEstep(0.);
      size_t q1NumBins=1,q2NumBins=1,q3NumBins=1,dENumBins=1;
      for(int i=1;i<=4;i++)
      {
          std::string dim("Dimension");
          dim+=Kernel::toString(i);
          std::string dimensioni=getProperty(dim);
          if (dimensioni=="Q1")
          {
              affineMat[i-1][0]=1.;
              q1min=getProperty(dim+"Min");
              q1max=getProperty(dim+"Max");
              q1step=getProperty(dim+"Step");
              m_hIdx=i-1;
          }
          if (dimensioni=="Q2")
          {
              affineMat[i-1][1]=1.;
              q2min=getProperty(dim+"Min");
              q2max=getProperty(dim+"Max");
              q2step=getProperty(dim+"Step");
              m_kIdx=i-1;
          }
          if (dimensioni=="Q3")
          {
              affineMat[i-1][2]=1.;
              q3min=getProperty(dim+"Min");
              q3max=getProperty(dim+"Max");
              q3step=getProperty(dim+"Step");
              m_lIdx=i-1;
          }
          if (dimensioni=="DeltaE")
          {
              affineMat[i-1][3]=1.;
              m_eIdx=i-1;
              double dmin=getProperty(dim+"Min");
              if(dmin==EMPTY_DBL())
              {
                m_dEmin=-static_cast<coord_t>(m_Ei);
              }
              else
              {
                m_dEmin=static_cast<coord_t>(dmin);
              }
              double dmax=getProperty(dim+"Max");
              if(dmax==EMPTY_DBL())
              {
                m_dEmax=static_cast<coord_t>(m_Ei);
              }
              else
              {
                m_dEmax=static_cast<coord_t>(dmax);
              }
              dEstep=getProperty(dim+"Step");
              if (dEstep==EMPTY_DBL())
              {
                  dENumBins=1;
              }
              else
              {
                  dENumBins=static_cast<size_t>((m_dEmax-m_dEmin)/dEstep);
                  if(dEstep*static_cast<double>(dENumBins)+m_dEmin<m_dEmax)
                  {
                      dENumBins+=1;
                      m_dEmax=static_cast<coord_t>(dEstep*static_cast<double>(dENumBins))+m_dEmin;
                  }
              }

          }
      }

      if (affineMat.determinant()==0.)
      {
          g_log.debug()<<affineMat;
          throw std::invalid_argument("Please make sure each dimension is selected only once.");
      }

      //Qmax is at  kf=kfmin or kf=kfmax
      m_ki=std::sqrt(energyToK*m_Ei);
      m_kfmin=std::sqrt(energyToK*(m_Ei-m_dEmin));
      m_kfmax=std::sqrt(energyToK*(m_Ei-m_dEmax));
      double QmaxTemp=sqrt(m_ki*m_ki+m_kfmin*m_kfmin-2*m_ki*m_kfmin*cos(ttmax));
      double Qmax=QmaxTemp;
      QmaxTemp=sqrt(m_ki*m_ki+m_kfmax*m_kfmax-2*m_ki*m_kfmax*cos(ttmax));
      if(QmaxTemp>Qmax)
          Qmax=QmaxTemp;

      //get goniometer
      DblMatrix gon=DblMatrix(3,3,true);
      if (inputWS->run().getGoniometer().isDefined())
      {
          gon=inputWS->run().getGoniometerMatrix();
      }

      //get the UB
      DblMatrix UB=DblMatrix(3,3,true)*(0.5/M_PI);
      if (inputWS->sample().hasOrientedLattice())
      {
          UB=inputWS->sample().getOrientedLattice().getUB();
      }

      //get the W matrix
      DblMatrix W=DblMatrix(3,3);
      std::vector<double> Q1Basis = getProperty("Q1Basis");
      std::vector<double> Q2Basis = getProperty("Q2Basis");
      std::vector<double> Q3Basis = getProperty("Q3Basis");
      W.setRow(0,Q1Basis);
      W.setRow(1,Q2Basis);
      W.setRow(2,Q3Basis);

      m_rubw=gon*UB*W*(2.0*M_PI);
      m_rubw.Invert();

      //calculate maximum original limits
      Geometry::OrientedLattice ol;
      ol.setUB(m_rubw);
      m_hmin=static_cast<coord_t>(-Qmax*ol.a());
      m_hmax=static_cast<coord_t>(Qmax*ol.a());
      m_kmin=static_cast<coord_t>(-Qmax*ol.b());
      m_kmax=static_cast<coord_t>(Qmax*ol.b());
      m_lmin=static_cast<coord_t>(-Qmax*ol.c());
      m_lmax=static_cast<coord_t>(Qmax*ol.c());

      //adjust Q steps/dimensions
      if(q1min==EMPTY_DBL())
      {
          q1min=m_hmin;
      }
      if(q1max==EMPTY_DBL())
      {
          q1max=m_hmax;
      }
      if(q1min>=q1max)
      {
          throw std::invalid_argument("Q1max has to be greater than Q1min");
      }
      if (q1step==EMPTY_DBL())
      {
          q1NumBins=1;
      }
      else
      {
          q1NumBins=static_cast<size_t>((q1max-q1min)/q1step);
          if(q1step*static_cast<double>(q1NumBins)+q1min<q1max)
          {
              q1NumBins+=1;
              q1max=q1step*static_cast<double>(q1NumBins)+q1min;
          }
      }

      if(q2min==EMPTY_DBL())
      {
          q2min=m_kmin;
      }
      if(q2max==EMPTY_DBL())
      {
          q2max=m_kmax;
      }
      if(q2min>=q2max)
      {
          throw std::invalid_argument("Q2max has to be greater than Q2min");
      }
      if (q2step==EMPTY_DBL())
      {
          q2NumBins=1;
      }
      else
      {
          q2NumBins=static_cast<size_t>((q2max-q2min)/q2step);
          if(q2step*static_cast<double>(q2NumBins)+q2min<q2max)
          {
              q2NumBins+=1;
              q2max=q2step*static_cast<double>(q2NumBins)+q2min;
          }
      }

      if(q3min==EMPTY_DBL())
      {
          q3min=m_lmin;
      }
      if(q3max==EMPTY_DBL())
      {
          q3max=m_lmax;
      }
      if(q3min>=q3max)
      {
          throw std::invalid_argument("Q3max has to be greater than Q3min");
      }
      if (q3step==EMPTY_DBL())
      {
          q3NumBins=1;
      }
      else
      {
          q3NumBins=static_cast<size_t>((q3max-q3min)/q3step);
          if(q3step*static_cast<double>(q3NumBins)+q3min<q3max)
          {
              q3NumBins+=1;
              q3max=q3step*static_cast<double>(q3NumBins)+q3min;
          }
      }



      //create the output workspace
      std::vector<Mantid::Geometry::MDHistoDimension_sptr> binDimensions;
      MDHistoDimension_sptr out1(new MDHistoDimension("Q1", "Q1", "", static_cast<coord_t>(q1min), static_cast<coord_t>(q1max), q1NumBins));
      MDHistoDimension_sptr out2(new MDHistoDimension("Q2", "Q2", "", static_cast<coord_t>(q2min), static_cast<coord_t>(q2max), q2NumBins));
      MDHistoDimension_sptr out3(new MDHistoDimension("Q3", "Q3", "", static_cast<coord_t>(q3min), static_cast<coord_t>(q3max), q3NumBins));
      MDHistoDimension_sptr out4(new MDHistoDimension("DeltaE", "DeltaE", "meV", static_cast<coord_t>(m_dEmin), static_cast<coord_t>(m_dEmax), dENumBins));
      for(size_t row=0;row<=3;row++)
      {
          if(affineMat[row][0] == 1.)
          {
              binDimensions.push_back(out1);
          }
          if(affineMat[row][1] == 1.)
          {
              binDimensions.push_back(out2);
          }
          if(affineMat[row][2] == 1.)
          {
              binDimensions.push_back(out3);
          }
          if(affineMat[row][3] == 1.)
          {
              binDimensions.push_back(out4);
          }
      }

      m_normWS=MDHistoWorkspace_sptr(new MDHistoWorkspace(binDimensions));
      m_normWS->setTo(0.,0.,0.);
      setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(m_normWS));

      cacheDimensionXValues();

      const int64_t ndets = static_cast<int64_t>(tt.size());

      PARALLEL_FOR1(inputWS)
      for(int64_t i = 0; i < ndets; i++)
      {
        PARALLEL_START_INTERUPT_REGION
        auto intersections = calculateIntersections(tt[i], phi[i]);
        if(intersections.empty()) continue;
        auto intersectionsBegin = intersections.begin();
        for (auto it = intersectionsBegin + 1; it != intersections.end(); ++it)
        {
            const auto & curIntSec = *it;
            const auto & prevIntSec = *(it-1);
            // the full vector isn't used so compute only what is necessary
            double delta = curIntSec[3] - prevIntSec[3];
            if(delta < 1e-10) continue; // Assume zero contribution if difference is small
            // Average between two intersections for final position
            std::vector<coord_t> pos(4);
            std::transform(curIntSec.getBareArray(), curIntSec.getBareArray() + 4,
                prevIntSec.getBareArray(), pos.begin(),
                VectorHelper::SimpleAverage<double>());
            //transform kf to energy transfer
            pos[3]=static_cast<coord_t>(m_Ei-pos[3]*pos[3]/energyToK);

            std::vector<coord_t> posNew = affineMat*pos;
            size_t linIndex = m_normWS->getLinearIndexAtCoord(posNew.data());
            if(linIndex == size_t(-1)) continue;
           PARALLEL_CRITICAL(updateMD)
           {
            m_normWS->setSignalAt(linIndex, 1.);
           }
        }
        PARALLEL_END_INTERUPT_REGION
       }
       PARALLEL_CHECK_INTERUPT_REGION
  }

  /**
 * Calculate the points of intersection for the given detector with cuboid surrounding the
 * detector position in HKL
 * @param theta Polar angle withd detector
 * @param phi Azimuthal angle with detector
 * @return A list of intersections in HKL+kf space
 */
 std::vector<Kernel::VMD> CalculateCoverageDGS::calculateIntersections(const double theta, const double phi)
 {
    V3D qout(sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta)),qin(0.,0.,m_ki);
    qout = m_rubw*qout;
    qin = m_rubw*qin;
    double hStart = qin.X()-qout.X()*m_kfmin, hEnd = qin.X()-qout.X()*m_kfmax;
    double kStart = qin.Y()-qout.Y()*m_kfmin, kEnd = qin.Y()-qout.Y()*m_kfmax;
    double lStart = qin.Z()-qout.Z()*m_kfmin, lEnd = qin.Z()-qout.Z()*m_kfmax;
    double eps = 1e-10;
    auto hNBins = m_hX.size();
    auto kNBins = m_kX.size();
    auto lNBins = m_lX.size();
    auto eNBins = m_eX.size();
    std::vector<Kernel::VMD> intersections;
    intersections.reserve(hNBins + kNBins + lNBins + eNBins+8);//8 is 3*(min,max for each Q component)+kfmin+kfmax

    //calculate intersections with planes perpendicular to h
    if (fabs(hStart-hEnd) > eps)
    {
      double fmom=(m_kfmax-m_kfmin)/(hEnd-hStart);
      double fk=(kEnd-kStart)/(hEnd-hStart);
      double fl=(lEnd-lStart)/(hEnd-hStart);
      if(!m_hIntegrated)
      {
        for(size_t i = 0;i<hNBins;i++)
        {
          double hi = m_hX[i];
          if((hi>=m_hmin)&&(hi<=m_hmax) && ((hStart-hi)*(hEnd-hi)<0))
          {
            // if hi is between hStart and hEnd, then ki and li will be between kStart, kEnd and lStart, lEnd and momi will be between m_kfmin and m_kfmax
            double ki = fk*(hi-hStart)+kStart;
            double li = fl*(hi-hStart)+lStart;
            if ((ki>=m_kmin)&&(ki<=m_kmax)&&(li>=m_lmin)&&(li<=m_lmax))
            {
                double momi = fmom*(hi-hStart)+m_kfmin;
                Mantid::Kernel::VMD v(hi,ki,li,momi);
                intersections.push_back(v);
            }
          }
        }
      }
      double momhMin = fmom*(m_hmin-hStart)+m_kfmin;
      if ((momhMin-m_kfmin)*(momhMin-m_kfmax)<0)//m_kfmin>m_kfmax
      {
       //khmin and lhmin
       double khmin = fk*(m_hmin-hStart)+kStart;
       double lhmin = fl*(m_hmin-hStart)+lStart;
       if((khmin>=m_kmin)&&(khmin<=m_kmax)&&(lhmin>=m_lmin)&&(lhmin<=m_lmax))
       {
         Mantid::Kernel::VMD v(m_hmin,khmin,lhmin,momhMin);
         intersections.push_back(v);
       }
      }
      double momhMax = fmom*(m_hmax-hStart)+m_kfmin;
      if ((momhMax-m_kfmin)*(momhMax-m_kfmax)<=0)
      {
        //khmax and lhmax
        double khmax = fk*(m_hmax-hStart)+kStart;
        double lhmax = fl*(m_hmax-hStart)+lStart;
        if((khmax>=m_kmin)&&(khmax<=m_kmax)&&(lhmax>=m_lmin)&&(lhmax<=m_lmax))
        {
          Mantid::Kernel::VMD v(m_hmax,khmax,lhmax,momhMax);
          intersections.push_back(v);
        }
      }
    }

    //calculate intersections with planes perpendicular to k
    if (fabs(kStart-kEnd) > eps)
    {
      double fmom=(m_kfmax-m_kfmin)/(kEnd-kStart);
      double fh=(hEnd-hStart)/(kEnd-kStart);
      double fl=(lEnd-lStart)/(kEnd-kStart);
      if(!m_kIntegrated)
      {
        for(size_t i = 0;i<kNBins;i++)
        {
          double ki = m_kX[i];
          if((ki>=m_kmin)&&(ki<=m_kmax) && ((kStart-ki)*(kEnd-ki)<0))
          {
            // if ki is between kStart and kEnd, then hi and li will be between hStart, hEnd and lStart, lEnd and momi will be between m_kfmin and m_kfmax
            double hi = fh*(ki-kStart)+hStart;
            double li = fl*(ki-kStart)+lStart;
            if ((hi>=m_hmin)&&(hi<=m_hmax)&&(li>=m_lmin)&&(li<=m_lmax))
            {
                double momi = fmom*(ki-kStart)+m_kfmin;
                Mantid::Kernel::VMD v(hi,ki,li,momi);
                intersections.push_back(v);
            }
          }
        }
      }
      double momkMin = fmom*(m_kmin-kStart)+m_kfmin;
      if ((momkMin-m_kfmin)*(momkMin-m_kfmax)<0)
      {
       //hkmin and lkmin
       double hkmin = fh*(m_kmin-kStart)+hStart;
       double lkmin = fl*(m_kmin-kStart)+lStart;
       if((hkmin>=m_hmin)&&(hkmin<=m_hmax)&&(lkmin>=m_lmin)&&(lkmin<=m_lmax))
       {
         Mantid::Kernel::VMD v(hkmin,m_kmin,lkmin,momkMin);
         intersections.push_back(v);
       }
      }
      double momkMax = fmom*(m_kmax-kStart)+m_kfmin;
      if ((momkMax-m_kfmin)*(momkMax-m_kfmax)<=0)
      {
        //hkmax and lkmax
        double hkmax = fh*(m_kmax-kStart)+hStart;
        double lkmax = fl*(m_kmax-kStart)+lStart;
        if((hkmax>=m_hmin)&&(hkmax<=m_hmax)&&(lkmax>=m_lmin)&&(lkmax<=m_lmax))
        {
          Mantid::Kernel::VMD v(hkmax,m_kmax,lkmax,momkMax);
          intersections.push_back(v);
        }
      }
    }


    //calculate intersections with planes perpendicular to l
    if (fabs(lStart-lEnd) > eps)
    {
      double fmom=(m_kfmax-m_kfmin)/(lEnd-lStart);
      double fh=(hEnd-hStart)/(lEnd-lStart);
      double fk=(kEnd-kStart)/(lEnd-lStart);
      if(!m_lIntegrated)
      {
        for(size_t i = 0;i<lNBins;i++)
        {
          double li = m_lX[i];
          if((li>=m_lmin)&&(li<=m_lmax) && ((lStart-li)*(lEnd-li)<0))
          {
            double hi = fh*(li-lStart)+hStart;
            double ki = fk*(li-lStart)+kStart;
            if ((hi>=m_hmin)&&(hi<=m_hmax)&&(ki>=m_kmin)&&(ki<=m_kmax))
            {
                double momi = fmom*(li-lStart)+m_kfmin;
                Mantid::Kernel::VMD v(hi,ki,li,momi);
                intersections.push_back(v);
            }
          }
        }
      }
      double momlMin = fmom*(m_lmin-lStart)+m_kfmin;
      if ((momlMin-m_kfmin)*(momlMin-m_kfmax)<=0)
      {
       //hlmin and klmin
       double hlmin = fh*(m_lmin-lStart)+hStart;
       double klmin = fk*(m_lmin-lStart)+kStart;
       if((hlmin>=m_hmin)&&(hlmin<=m_hmax)&&(klmin>=m_kmin)&&(klmin<=m_kmax))
       {
         Mantid::Kernel::VMD v(hlmin,klmin,m_lmin,momlMin);
         intersections.push_back(v);
       }
      }
      double momlMax = fmom*(m_lmax-lStart)+m_kfmin;
      if ((momlMax-m_kfmin)*(momlMax-m_kfmax)<0)
      {
        //hlmax and klmax
        double hlmax = fh*(m_lmax-lStart)+hStart;
        double klmax = fk*(m_lmax-lStart)+kStart;
        if((hlmax>=m_hmin)&&(hlmax<=m_hmax)&&(klmax>=m_kmin)&&(klmax<=m_kmax))
        {
          Mantid::Kernel::VMD v(hlmax,klmax,m_lmax,momlMax);
          intersections.push_back(v);
        }
      }
    }

    //intersections with dE
    if(!m_dEIntegrated)
    {
        for(size_t i = 0;i<eNBins;i++)
        {
            double kfi=m_eX[i];
            if((kfi-m_kfmin)*(kfi-m_kfmax)<=0)
            {
                double h = qin.X()-qout.X()*kfi;
                double k = qin.Y()-qout.Y()*kfi;
                double l = qin.Z()-qout.Z()*kfi;
                if((h>=m_hmin)&&(h<=m_hmax)&&(k>=m_kmin)&&(k<=m_kmax)&&(l>=m_lmin)&&(l<=m_lmax))
                {
                    Mantid::Kernel::VMD v(h,k,l,kfi);
                    intersections.push_back(v);
                }
            }
        }
    }

    //endpoints
    if ((hStart>=m_hmin)&&(hStart<=m_hmax)&&(kStart>=m_kmin)&&(kStart<=m_kmax)&&(lStart>=m_lmin)&&(lStart<=m_lmax))
    {
      Mantid::Kernel::VMD v(hStart,kStart,lStart,m_kfmin);
      intersections.push_back(v);
    }
    if ((hEnd>=m_hmin)&&(hEnd<=m_hmax)&&(kEnd>=m_kmin)&&(kEnd<=m_kmax)&&(lEnd>=m_lmin)&&(lEnd<=m_lmax))
    {
      Mantid::Kernel::VMD v(hEnd,kEnd,lEnd,m_kfmax);
      intersections.push_back(v);
    }

    //sort intersections by final momentum
    typedef std::vector<Mantid::Kernel::VMD>::iterator IterType;
    std::stable_sort<IterType,bool (*)(const Mantid::Kernel::VMD&,const Mantid::Kernel::VMD&)>(intersections.begin(),intersections.end(),compareMomentum);


    return intersections;
 }

} // namespace MDAlgorithms
} // namespace Mantid
