//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SCD_TOFINT.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
//#include "MantidAPI/IFunction.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <fstream>
#include <ostream>
#include <iomanip>
#include <sstream>

namespace Mantid
{
  namespace Algorithms
  {

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(SCD_TOFINT)

    using namespace Kernel;
    using namespace API;

    /// Constructor
    SCD_TOFINT::SCD_TOFINT() :
      API::Algorithm()
    {}

    /// Destructor
    SCD_TOFINT::~SCD_TOFINT()
    {}

    /** Initialisation method. Declares properties to be used in algorithm.
     *
     */
    void SCD_TOFINT::init()
    {

      declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input)
          ,"A 2D workspace with X values of d-spacing");
      declareProperty(new API::WorkspaceProperty<>("OutputWorkspace","",Direction::Output),"Workspace containing the integrated boxes");
      declareProperty("XMin", -2, "Minimum of X (col) Range to integrate for peak");
      declareProperty("XMax", 2, "Maximum of X (col) Range to integrate for peak");
      declareProperty("YMin", -2, "Minimum of Y (row) Range to integrate for peak");
      declareProperty("YMax", 2, "Maximum of Y (row) Range to integrate for peak");
      declareProperty("TOFMin", -150.0, "Minimum of TOF Range to integrate for peak");
      declareProperty("TOFMax", 150.0, "Maximum of TOF Range to integrate for peak");
    }

    /** Executes the algorithm
     *
     *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
     */
    void SCD_TOFINT::exec()
    {
      retrieveProperties();
      std::string Peakbank="bank14";
      int XPeak = 53-1;
      int YPeak = 168-1;
      // Between 549 and 550 TOF bins
      double TOFPeak = 3590.45;
      TOFmin += TOFPeak;
      TOFmax += TOFPeak;
      IAlgorithm_sptr sum_alg;
      try
      {
        //set the subalgorithm no to log as this will be run once per spectra
        sum_alg = createSubAlgorithm("SumNeighbours",-1,-1,false);
      } catch (Exception::NotFoundError&)
      {
        g_log.error("Can't locate SumNeighbours algorithm");
        throw ;
      }
      sum_alg->setProperty("InputWorkspace",inputW);
      sum_alg->setProperty("OutputWorkspace","tmp");
      sum_alg->setProperty("SumX",Xmax-Xmin+1);
      sum_alg->setProperty("SumY",Ymax-Ymin+1);
      sum_alg->setProperty("DetectorName",Peakbank);
      sum_alg->setProperty("Xpixel",XPeak+Xmin);
      sum_alg->setProperty("Ypixel",YPeak+Ymin);

      try
      {
        sum_alg->execute();
      }
      catch (std::runtime_error&)
      {
        g_log.error("Unable to successfully run Fit sub-algorithm");
        throw;
      }
      outputW=sum_alg->getProperty("OutputWorkspace");
      double integral=fitSpectra(0);

      setProperty("OutputWorkspace",outputW);
    }

    void SCD_TOFINT::retrieveProperties()
    {
      inputW=getProperty("InputWorkspace");
      Xmin=getProperty("XMin");
      Xmax=getProperty("XMax");
      Ymin=getProperty("YMin");
      Ymax=getProperty("YMax");
      TOFmin=getProperty("TOFMin");
      TOFmax=getProperty("TOFMax");
      if (Xmin>=Xmax)
        throw std::runtime_error("Must specify Xmin<Xmax");
      if (Ymin>=Ymax)
        throw std::runtime_error("Must specify Ymin<Ymax");
      if (TOFmin>=TOFmax)
        throw std::runtime_error("Must specify TOFmin<TOFmax");
    }

   /** Calls Fit as a child algorithm to fit the offset peak in a spectrum
    *  @param s :: The spectrum index to fit
    *  @return The calculated offset value
    */
    double SCD_TOFINT::fitSpectra(const int s)
    {
      // Find point of peak centre
      const MantidVec & yValues = outputW->readY(s);
      MantidVec::const_iterator it = std::max_element(yValues.begin(),yValues.begin()+600);
      const double peakHeight = *it; 
      const double peakLoc = outputW->readX(s)[it - yValues.begin()];
      // Return offset of 0 if peak of Cross Correlation is nan (Happens when spectra is zero)
      if ( boost::math::isnan(peakHeight) ) return (0.);

      std::cout << peakHeight<<" "<<peakLoc<<"\n";
      IAlgorithm_sptr fit_alg;
      try
      {
        //set the subalgorithm no to log as this will be run once per spectra
        fit_alg = createSubAlgorithm("Fit",-1,-1,false);
      } catch (Exception::NotFoundError&)
      {
        g_log.error("Can't locate Fit algorithm");
        throw ;
      }
      fit_alg->setProperty("InputWorkspace",outputW);
      fit_alg->setProperty("WorkspaceIndex",s);
      fit_alg->setProperty("StartX",TOFmin);
      fit_alg->setProperty("EndX",TOFmax);
      fit_alg->setProperty("MaxIterations",200);
      fit_alg->setProperty("Output","tmp");

      // set up fitting function and pass to Fit
      std::ostringstream fun_str;
      /*fun_str << "name=IkedaCarpenterPV,I="<<peakHeight<<",Alpha0=0.432702,Alpha1=-0.0802024,Beta0=36.4172,Kappa=40.3074,SigmaSquared=42.3954,";
      fun_str << "X0="<<peakLoc<<",Gamma=12.8576";*/
      /*fun_str << "name=LinearBackground,A0=0.0,A1=0.0;(composite=Convolution;name=IkedaCarpenterPV,I="<<peakHeight<<",Alpha0=1.6,Alpha1=1.5,Beta0=31.9,Kappa=46,SigmaSquared=1,Gamma=1,X0="<<peakLoc<<";name=Lorentzian,Height="<<peakHeight<<",PeakCentre="<<peakLoc<<",HWHM=0)";*/
      fun_str << "name=LinearBackground,A0=0.0,A1=0.0;name=IkedaCarpenterPV,I="<<peakHeight<<",Alpha0=1.6,Alpha1=1.5,Beta0=31.9,Kappa=46,SigmaSquared=1,Gamma=1,X0="<<peakLoc;

      fit_alg->setProperty("Function",fun_str.str());
      fit_alg->setProperty("Ties","f0.A1=0.0");

      try
      {
        fit_alg->execute();
      }
      catch (std::runtime_error&)
      {
        g_log.error("Unable to successfully run Fit sub-algorithm");
        throw;
      }

      if ( ! fit_alg->isExecuted() )
      {
        g_log.error("Unable to successfully run Fit sub-algorithm");
        throw std::runtime_error("Unable to successfully run Fit sub-algorithm");
      }
      MatrixWorkspace_sptr ws = fit_alg->getProperty("OutputWorkspace");
      const MantidVec & DataValues = ws->readY(0);
      const MantidVec & FitValues = ws->readY(1);
      for (int i=0; i < static_cast<int>(DataValues.size()); i++) std::cout <<DataValues[i]<<" ";
      std::cout <<"\n";
      for (int i=0; i < static_cast<int>(FitValues.size()); i++) std::cout <<FitValues[i]<<" ";
      std::cout <<"\n";

      IFitFunction *out = FunctionFactory::Instance().createInitialized(fit_alg->getPropertyValue("Function"));
      std::cout <<"After IFit\n";
      IPeakFunction *pk = dynamic_cast<IPeakFunction *>(out);
      std::cout <<"After IPeak\n";
      const int n=100;
      double *x = new double[n];
      double *y = new double[n];
      std::cout <<"After alloc\n";
      double dx=(TOFmax-TOFmin)/double(n-1);
      for (int i=0; i < n; i++) {
        x[i]=TOFmin+i*dx;
        std::cout <<x[i]<<"\n";
      }
      pk->function(y,x,n);
      std::cout <<"After funct\n";
      for (int i=0; i < n; i++) std::cout <<y[i]<<" ";
      std::cout <<"\n";

      std::vector<double> params = fit_alg->getProperty("Parameters");
      double bkg0 = params[0]*n;
      double I = -bkg0;
      for (int i=0; i < n; i++) I+=y[i];
      double sigI = I*I-2.0*I*bkg0+bkg0*bkg0;
      std::cout << "I=" << I << " sigI=" << sigI << "\n";
      return I;
    }



  } // namespace Algorithm
} // namespace Mantid
