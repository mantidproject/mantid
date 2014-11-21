#include "MantidMDAlgorithms/CalculateCoverageDGS.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"

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
          //TODO: get Ei from workspace
          throw std::invalid_argument("Could not find Ei");
      }

      //TODO: get dEmin,dEmax from properties
      double dEmin=-Ei;
      double dEmax=Ei;
      // Conversion constant for E->k. k(A^-1) = sqrt(energyToK*E(meV))
      const double energyToK = 8.0*M_PI*M_PI*PhysicalConstants::NeutronMass*PhysicalConstants::meV*1e-20 /
        (PhysicalConstants::h*PhysicalConstants::h);
      //Qmax is at  kf=kfmin or kf=kfmax
      double ki=std::sqrt(energyToK*Ei);
      double kfmin=std::sqrt(energyToK*(Ei-dEmin));
      double kfmax=std::sqrt(energyToK*(Ei-dEmax));
      double QmaxTemp=sqrt(ki*ki+kfmin*kfmin-2*ki*kfmin*cos(ttmax));
      double Qmax=QmaxTemp;
      QmaxTemp=sqrt(ki*ki+kfmax*kfmax-2*ki*kfmax*cos(ttmax));
      if(QmaxTemp>Qmax)
          Qmax=QmaxTemp;


      g_log.warning()<<ttmax<<" "<<Qmax;

      size_t q1NumBins=5,q2NumBins=5,q3NumBins=5,eNumBins=5;
      double q1min=0,q1max=1;
      double q2min=0,q2max=2;
      double q3min=0,q3max=3;
      double emin=0,emax=10;

      //create the output workspace
      std::vector<Mantid::Geometry::MDHistoDimension_sptr> binDimensions;
      MDHistoDimension_sptr out1(new MDHistoDimension("Q1", "Q1", "", static_cast<coord_t>(q1min), static_cast<coord_t>(q1max), q1NumBins));
      MDHistoDimension_sptr out2(new MDHistoDimension("Q2", "Q2", "", static_cast<coord_t>(q2min), static_cast<coord_t>(q2max), q2NumBins));
      MDHistoDimension_sptr out3(new MDHistoDimension("Q3", "Q3", "", static_cast<coord_t>(q3min), static_cast<coord_t>(q3max), q3NumBins));
      MDHistoDimension_sptr out4(new MDHistoDimension("DeltaE", "DeltaE", "meV", static_cast<coord_t>(emin), static_cast<coord_t>(emax), eNumBins));
      binDimensions.push_back(out1);
      binDimensions.push_back(out2);
      binDimensions.push_back(out3);
      binDimensions.push_back(out4);
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
