//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SingleCrystalTOFIntegration.h"
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
    DECLARE_ALGORITHM(SingleCrystalTOFIntegration)

    using namespace Kernel;
    using namespace API;

    /// Constructor
    SingleCrystalTOFIntegration::SingleCrystalTOFIntegration() :
      API::Algorithm()
    {}

    /// Destructor
    SingleCrystalTOFIntegration::~SingleCrystalTOFIntegration()
    {}

    /** Initialisation method. Declares properties to be used in algorithm.
     *
     */
    void SingleCrystalTOFIntegration::init()
    {

      declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input)
          ,"A 2D workspace with X values of d-spacing");
      declareProperty(new API::WorkspaceProperty<>("OutputWorkspace","",Direction::Output),"Workspace containing the integrated boxes");
      declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Load, ".integrate"), "The input filename of the ISAW Integrate file");

      declareProperty("XMin", -2, "Minimum of X (col) Range to integrate for peak");
      declareProperty("XMax", 2, "Maximum of X (col) Range to integrate for peak");
      declareProperty("YMin", -2, "Minimum of Y (row) Range to integrate for peak");
      declareProperty("YMax", 2, "Maximum of Y (row) Range to integrate for peak");
      declareProperty("TOFBinMin", -5, "Minimum of TOF Bin Range to integrate for peak");
      declareProperty("TOFBinMax", 5, "Maximum of TOF Bin Range to integrate for peak");
      declareProperty("Params", "400.0,-0.004,45000.",
        "A comma separated list of first bin boundary, width, last bin boundary. Optionally\n"
        "this can be followed by a comma and more widths and last boundary pairs.\n"
        "Use bin boundaries close to peak you wish to maximize.\n"
        "Negative width values indicate logarithmic binning.");

    }

    /** Executes the algorithm
     *
     *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
     */
    void SingleCrystalTOFIntegration::exec()
    {
      retrieveProperties();
      std::string filename = getProperty("Filename");
      std::ifstream input(filename.c_str(), std::ios_base::in);
      std::string line;
      int id,seqn,h,k,l,ipk,rflg,nrun,detnum;
      double col,row,chan,l1,l2,twotheta,az,wl,d,inti,sigi,chi,phi,omega,moncnt,t0_shift;

      std::ofstream fout("Mantid.integrate");
      fout<<"2   SEQN    H    K    L     COL     ROW    CHAN       L2  2_THETA       AZ        WL        D   IPK       INTI       SIGI INTIMantid SIGIMantid\n";
      while(std::getline(input, line))
      {
       if(line[0] == '7') std::stringstream(line) >> id >> l1 >> t0_shift;
       if(line[0] == '1') std::stringstream(line) >> id >> nrun >> detnum >> chi >> phi >> omega >> moncnt;
       if(line[0] != '3') continue;
       std::stringstream(line) >> id >> seqn >> h >> k >> l >> col >> row >> chan >> l2
          >> twotheta >> az >> wl >> d >> ipk >> inti >> sigi >> rflg;
       if(ipk < 10) continue;
       fout << std::setw(2) << detnum << std::setw(6) << seqn << std::setw(5) << h << std::setw(5) << k << std::setw(5) << l << std::setw(8) << std::setprecision(2) << col << std::setw(8) << std::setprecision(2) << row << std::setw(8) << std::setprecision(2) << chan << std::setw(9) << std::setprecision(3) << l2
           << std::setw(9) << std::setprecision(5) << twotheta << std::setw(9) << std::setprecision(5) << az << std::setw(10) << std::setprecision(6) << wl << std::setw(9) << std::setprecision(3) << d << std::setw(6) << ipk << std::setw(11) << std::setprecision(2) << inti << std::setw(11) << std::setprecision(2) << sigi ;

      std::ostringstream Peakbank;
      Peakbank <<"bank"<<detnum;
      int XPeak = col-1;
      int YPeak = row-1;
      tofISAW = 7.91*(l2+l1);
      TOFPeak = chan-1;
      TOFmin = TOFPeak+Binmin;
      if (TOFmin<0) TOFmin = 0;
      TOFmax = TOFPeak+Binmax;

      IAlgorithm_sptr sum_alg;
      try
      {
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
      sum_alg->setProperty("SingleNeighbourhood",true);
      sum_alg->setProperty("Xpixel",XPeak+Xmin);
      sum_alg->setProperty("Ypixel",YPeak+Ymin);
      sum_alg->setProperty("DetectorName",Peakbank.str());

      try
      {
        sum_alg->execute();
      }
      catch (std::runtime_error&)
      {
        g_log.error("Unable to successfully run Fit sub-algorithm");
        throw;
      }
      outputW = sum_alg->getProperty("OutputWorkspace");

      IAlgorithm_sptr bin_alg = createSubAlgorithm("Rebin");
      bin_alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", outputW);
      bin_alg->setProperty("OutputWorkspace", "tmp2");
      bin_alg->setPropertyValue("Params", getProperty("Params"));
      try
      {
        bin_alg->execute();
      }
      catch (std::runtime_error&)
      {
        g_log.information("Unable to successfully run Rebin sub-algorithm");
        throw std::runtime_error("Error while executing Rebin as a sub algorithm.");
      }

      outputW = bin_alg->getProperty("OutputWorkspace");
      if (TOFmin > outputW->blocksize()) TOFmin = outputW->blocksize();
      if (TOFmax > outputW->blocksize()) TOFmax = outputW->blocksize();

      double I, sigI;
      fitSpectra(0, I, sigI);
      fout << std::setw(11) << std::setprecision(2) << I << std::setw(11) << std::setprecision(2) << sigI << "\n";

      setProperty("OutputWorkspace",outputW);
    }
    fout.close();
    }

    void SingleCrystalTOFIntegration::retrieveProperties()
    {
      inputW = getProperty("InputWorkspace");
      Xmin = getProperty("XMin");
      Xmax = getProperty("XMax");
      Ymin = getProperty("YMin");
      Ymax = getProperty("YMax");
      Binmin = getProperty("TOFBinMin");
      Binmax = getProperty("TOFBinMax");
      if (Xmin >= Xmax)
        throw std::runtime_error("Must specify Xmin<Xmax");
      if (Ymin >= Ymax)
        throw std::runtime_error("Must specify Ymin<Ymax");
      if (Binmin >= Binmax)
        throw std::runtime_error("Must specify Binmin<Binmax");
    }

   /** Calls Fit as a child algorithm to fit the offset peak in a spectrum
    *  @param s :: The spectrum index to fit
    *  @return The calculated offset value
    */
    void SingleCrystalTOFIntegration::fitSpectra(const int s, double& I, double& sigI)
    {
      // Find point of peak centre
      // Get references to the current spectrum
      MantidVec &X = outputW->dataX(s);
      MantidVec &Y = outputW->dataY(s);
      const double peakLoc = X[TOFPeak];
      const double peakHeight = Y[TOFPeak];
      // Return offset of 0 if peak of Cross Correlation is nan (Happens when spectra is zero)
      if ( boost::math::isnan(peakHeight) ) return;

      /*IAlgorithm_sptr bkg_alg;
      try
      {
        bkg_alg = createSubAlgorithm("Fit",-1,-1,false);
      } catch (Exception::NotFoundError&)
      {
        g_log.error("Can't locate Fit algorithm");
        throw ;
      }
      bkg_alg->setProperty("InputWorkspace",outputW);
      bkg_alg->setProperty("WorkspaceIndex",s);
      double x0 = X[TOFPeak]-1000.0;
      if (x0 < X[0]) x0 = X[0];
      double xn = X[TOFPeak]+1000.0;
      if (xn >  X[outputW->blocksize()]) xn = X[outputW->blocksize()];
      bkg_alg->setProperty("StartX",x0);
      bkg_alg->setProperty("EndX",xn);
      bkg_alg->setProperty("MaxIterations",200);
      bkg_alg->setProperty("Output","bkg");

      // set up fitting function and pass to Fit
      std::ostringstream bkg_str;
      bkg_str <<"name=LinearBackground,A0=16.8446;";
      bkg_alg->setProperty("Function",bkg_str.str());
      bkg_alg->setProperty("Ties","A1=0.0");
      try
      {
        bkg_alg->execute();
      }
      catch (std::runtime_error&)
      {
        g_log.error("Unable to successfully run Fit sub-algorithm");
        throw;
      }
      std::vector<double> params = bkg_alg->getProperty("Parameters");
      std::string bkgfunct = bkg_alg->getPropertyValue("Function");
      //std::cout <<bkgfunct<<"\n";
      double bkg0 = params[0];*/
      double bkg0 = (Y[TOFmin]+Y[TOFmax])*0.5;

      //for (int j=TOFmin; j <= TOFmax; ++j)std::cout <<Y[j]<<"  ";
      //std::cout <<"\n";
      // Now subtract the background from the data
      for (int j=0; j < outputW->blocksize(); ++j)
      {
         Y[j] -= bkg0;
      }

      IAlgorithm_sptr fit_alg;
      try
      {
        fit_alg = createSubAlgorithm("Fit",-1,-1,false);
      } catch (Exception::NotFoundError&)
      {
        g_log.error("Can't locate Fit algorithm");
        throw ;
      }
      fit_alg->setProperty("InputWorkspace",outputW);
      fit_alg->setProperty("WorkspaceIndex",s);
      fit_alg->setProperty("StartX",X[TOFmin+1]);
      fit_alg->setProperty("EndX",X[TOFmax-1]);
      fit_alg->setProperty("MaxIterations",200);
      fit_alg->setProperty("Output","fit");
      std::ostringstream fun_str;
      fun_str << "name=IkedaCarpenterPV,I="<<peakHeight<<",Alpha0=-12.7481,Alpha1=14.5239,Beta0=3.73893,Kappa=57.4813,SigmaSquared=0.00424469,Gamma=26.8937," << "X0="<<peakLoc;
      //fun_str << "name=Gaussian,Height="<<peakHeight<<",Sigma=17.5221,PeakCentre="<<peakLoc;
      fit_alg->setProperty("Function",fun_str.str());
      std::ostringstream tie_str;
      tie_str << "Alpha1=14.5239";
      //tie_str << "Alpha0="<<Alpha0<<",Alpha1="<<Alpha1<<",Beta0="<<Beta0<<",Kappa="<<Kappa<<",SigmaSquared="<<SigmaSquared<<",Gamma="<<Gamma;
      fit_alg->setProperty("Ties",tie_str.str());

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
      //const MantidVec & FitValues = ws->readY(1);

      IFitFunction *out = FunctionFactory::Instance().createInitialized(fit_alg->getPropertyValue("Function"));
      std::vector<double> params = fit_alg->getProperty("Parameters");
      /*Alpha0 = params[1];
      Alpha1 = params[2];
      Beta0 = params[3];
      Kappa = params[4];
      SigmaSquared = params[5];
      Gamma = params[6];*/
      std::string funct = fit_alg->getPropertyValue("Function");
      std::cout <<funct<<"\n";
      IPeakFunction *pk = dynamic_cast<IPeakFunction *>(out);
      const int n=1000;
      double *x = new double[n];
      double *y = new double[n];
      double dx=(X[TOFmax-1]-X[TOFmin+1])/double(n-1);
      for (int i=0; i < n; i++) {
        x[i] = X[TOFmin+1]+i*dx;
      }
      pk->function(y,x,n);

      I = 0.0;
      for (int i=0; i < n; i++) I+=y[i];
      I*=double(DataValues.size())/double(n);
      sigI = sqrt(I+DataValues.size()*bkg0+DataValues.size()*DataValues.size()*bkg0);
      return;
    }



  } // namespace Algorithm
} // namespace Mantid
