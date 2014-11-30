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

namespace Mantid
{
namespace MDAlgorithms
{
  using namespace Mantid::Kernel;
  using Mantid::API::WorkspaceProperty;
  using namespace Mantid::API;
  using namespace Mantid::MDEvents;
  using namespace Mantid::Geometry;

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(CalculateCoverageDGS)



  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  CalculateCoverageDGS::CalculateCoverageDGS()
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
      double Ei=getProperty("IncidentEnergy");
      if (Ei==EMPTY_DBL())
      {
          if (inputWS->run().hasProperty("Ei"))
          {
              Kernel::Property* eiprop = inputWS->run().getProperty("Ei");
              Ei = boost::lexical_cast<double>(eiprop->value());
              if(Ei<=0)
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
          }
          if (dimensioni=="Q2")
          {
              affineMat[i-1][1]=1.;
              q2min=getProperty(dim+"Min");
              q2max=getProperty(dim+"Max");
              q2step=getProperty(dim+"Step");
          }
          if (dimensioni=="Q3")
          {
              affineMat[i-1][2]=1.;
              q3min=getProperty(dim+"Min");
              q3max=getProperty(dim+"Max");
              q3step=getProperty(dim+"Step");
          }
          if (dimensioni=="DeltaE")
          {
              affineMat[i-1][3]=1.;
              double dmin=getProperty(dim+"Min");
              if(dmin==EMPTY_DBL())
              {
                m_dEmin=-static_cast<coord_t>(Ei);
              }
              else
              {
                m_dEmin=static_cast<coord_t>(dmin);
              }
              double dmax=getProperty(dim+"Max");
              if(dmax==EMPTY_DBL())
              {
                m_dEmax=static_cast<coord_t>(Ei);
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
      // Conversion constant for E->k. k(A^-1) = sqrt(energyToK*E(meV))
      const double energyToK = 8.0*M_PI*M_PI*PhysicalConstants::NeutronMass*PhysicalConstants::meV*1e-20 /
        (PhysicalConstants::h*PhysicalConstants::h);
      //Qmax is at  kf=kfmin or kf=kfmax
      double ki=std::sqrt(energyToK*Ei);
      double kfmin=std::sqrt(energyToK*(Ei-m_dEmin));
      double kfmax=std::sqrt(energyToK*(Ei-m_dEmax));
      double QmaxTemp=sqrt(ki*ki+kfmin*kfmin-2*ki*kfmin*cos(ttmax));
      double Qmax=QmaxTemp;
      QmaxTemp=sqrt(ki*ki+kfmax*kfmax-2*ki*kfmax*cos(ttmax));
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

      Mantid::MDEvents::MDHistoWorkspace_sptr coverage=MDHistoWorkspace_sptr(new MDHistoWorkspace(binDimensions));
      setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(coverage));

      //put 1 for the coverage
      std::vector<coord_t> pos;
      pos.push_back(0.5);
      pos.push_back(0.5);
      pos.push_back(0.5);
      pos.push_back(0.5);
      size_t linIndex=coverage->getLinearIndexAtCoord(pos.data());
      coverage->setSignalAt(linIndex,1.);
  }



} // namespace MDAlgorithms
} // namespace Mantid
