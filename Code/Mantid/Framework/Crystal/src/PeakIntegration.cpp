//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCrystal/PeakIntegration.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
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
    DECLARE_ALGORITHM(PeakIntegration)

    using namespace Kernel;
    using namespace API;
    using namespace DataObjects;


    /// Constructor
    PeakIntegration::PeakIntegration() :
      API::Algorithm()
    {}

    /// Destructor
    PeakIntegration::~PeakIntegration()
    {}

    /** Initialisation method. Declares properties to be used in algorithm.
     *
     */
    void PeakIntegration::init()
    {

      declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input)
          ,"A 2D workspace with X values of time of flight");
      declareProperty(new API::WorkspaceProperty<>("OutputWorkspace","",Direction::Output),"Workspace containing the integrated boxes");
      declareProperty(new WorkspaceProperty<PeaksWorkspace>("InPeaksWorkspace","",Direction::Input), "Name of the peaks workspace.");

      declareProperty("XMin", -2, "Minimum of X (col) Range to integrate for peak");
      declareProperty("XMax", 2, "Maximum of X (col) Range to integrate for peak");
      declareProperty("YMin", -2, "Minimum of Y (row) Range to integrate for peak");
      declareProperty("YMax", 2, "Maximum of Y (row) Range to integrate for peak");
      declareProperty("TOFBinMin", -5, "Minimum of TOF Bin Range to integrate for peak");
      declareProperty("TOFBinMax", 5, "Maximum of TOF Bin Range to integrate for peak");
      declareProperty(
        new ArrayProperty<double>("Params", new RebinParamsValidator),
        "A comma separated list of first bin boundary, width, last bin boundary. Optionally\n"
        "this can be followed by a comma and more widths and last boundary pairs.\n"
        "Negative width values indicate logarithmic binning.");
      declareProperty("LargePeak", 10, "Intensity of center of peak defined as large");

    }

    /** Executes the algorithm
     *
     *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
     */
    void PeakIntegration::exec()
    {
      retrieveProperties();
  
      // create TOF axis from Params
      MantidVecPtr XValues;
      const std::vector<double> rb_params=getProperty("Params");
      const int numbins = VectorHelper::createAxisFromRebinParams(rb_params, XValues.access());
      PeaksWorkspace_sptr peaksW;
      peaksW = boost::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve(getProperty("InPeaksWorkspace")));


      int largepeak = getProperty("LargePeak");
      int h,k,l,ipk,detnum;
      double col,row,chan,l1,l2,wl,d;

      std::ofstream fout("Mantid.integrate");
      fout<<"2   SEQN    H    K    L     COL     ROW    CHAN       L2  2_THETA       AZ        WL        D   IPK       INTI       SIGI INTIMantid SIGIMantid\n";
      for (int i=0; i<peaksW->getNumberPeaks(); i++)
      {
       l1=peaksW->get_L1(i);
       detnum=peaksW->get_Bank(i);
       Geometry::V3D hkl=peaksW->get_hkl(i);
       h=hkl[0];
       k=hkl[1];
       l=hkl[2];
       col=peaksW->get_column(i);
       row=peaksW->get_row(i);
       chan=peaksW->get_time_channel(i);
       l2=peaksW->get_L2(i);
       wl=peaksW->get_wavelength(i);
       d=peaksW->get_dspacing(i);
       ipk=peaksW->get_ipk(i);
       if(ipk < largepeak) continue;
       fout << std::fixed << std::setw(2) << detnum << std::setw(6) << i << std::setw(5) << h << std::setw(5) << k << std::setw(5) << l << std::setw(8) << std::setprecision(2) << col << std::setw(8) << std::setprecision(2) << row << std::setw(8) << std::setprecision(2) << chan << std::setw(9) << std::setprecision(3) << l2
           << std::setw(10) << std::setprecision(6) << wl << std::setw(9) << std::setprecision(3) << d << std::setw(6) << ipk ;

      std::ostringstream Peakbank;
      Peakbank <<"bank"<<detnum;
      int XPeak = int(col+0.5)-1;
      int YPeak = int(row+0.5)-1;
      tofISAW = wl * (l1+l2) / 3.956058e-1;
      TOFPeak = VectorHelper::getBinIndex(XValues.access(),tofISAW);


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
      bin_alg->setProperty<std::vector<double> >("Params", rb_params);
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
      if (TOFmin > numbins) TOFmin = numbins;
      if (TOFmax > numbins) TOFmax = numbins;

      double I, sigI;
      fitSpectra(0, I, sigI);
      fout << std::setw(11) << std::setprecision(2) << I << std::setw(11) << std::setprecision(2) << sigI << "\n";

      setProperty("OutputWorkspace",outputW);
    }
    fout.close();
    }

    void PeakIntegration::retrieveProperties()
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
    void PeakIntegration::fitSpectra(const int s, double& I, double& sigI)
    {
      // Find point of peak centre
      // Get references to the current spectrum
      MantidVec &X = outputW->dataX(s);
      MantidVec &Y = outputW->dataY(s);
      const double peakLoc = X[TOFPeak];
      const double peakHeight =  Y[TOFPeak];
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
      if (xn >  X[numbins]) xn = X[numbins];
      bkg_alg->setProperty("StartX",x0);
      bkg_alg->setProperty("EndX",xn);
      bkg_alg->setProperty("MaxIterations",1000);
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

      for (int j=TOFmin; j <= TOFmax; ++j)std::cout <<X[j]<<"  ";
      std::cout <<"\n";
      for (int j=TOFmin; j <= TOFmax; ++j)std::cout <<Y[j]<<"  ";
      std::cout <<"\n";
      // Now subtract the background from the data
      for (int j=0; j < outputW->blocksize(); ++j)
      {
         Y[j] -= bkg0;
      }
      for (int j=TOFmin; j <= TOFmax; ++j)std::cout <<Y[j]<<"  ";
      std::cout <<"\n";

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
      fit_alg->setProperty("StartX",X[TOFmin]);
      fit_alg->setProperty("EndX",X[TOFmax]);
      fit_alg->setProperty("MaxIterations",1000);
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

      std::vector<double> params = fit_alg->getProperty("Parameters");
      /*Alpha0 = params[1];
      Alpha1 = params[2];
      Beta0 = params[3];
      Kappa = params[4];
      SigmaSquared = params[5];
      Gamma = params[6];*/
      std::string funct = fit_alg->getPropertyValue("Function");
      std::cout <<funct<<"\n";

      //Fill output workspace with interval; use Fit to calculate values of Fit in interval
      const int n=outputW->blocksize();
      double dx=(X[TOFmax]-X[TOFmin])/double(n);
      double x0=X[TOFmin];
      for (int i=0; i <= n; i++) {
        X[i] = x0+i*dx;
        Y[i] = 0.0;
      }
      IAlgorithm_sptr fit2_alg;
      try
      {
        fit2_alg = createSubAlgorithm("Fit",-1,-1,false);
      } catch (Exception::NotFoundError&)
      {
        g_log.error("Can't locate Fit algorithm");
        throw ;
      }
      fit2_alg->setProperty("InputWorkspace",outputW);
      fit2_alg->setProperty("WorkspaceIndex",s);
      fit2_alg->setProperty("StartX",X[0]);
      fit2_alg->setProperty("EndX",X[n]);
      fit2_alg->setProperty("Function",funct);
      fit2_alg->setProperty("MaxIterations",0);
      fit2_alg->setProperty("Output","fit_iter0");

      try
      {
        fit2_alg->execute();
      }
      catch (std::runtime_error&)
      {
        g_log.error("Unable to successfully run Fit sub-algorithm");
        throw;
      }

      if ( ! fit2_alg->isExecuted() )
      {
        g_log.error("Unable to successfully run Fit sub-algorithm");
        throw std::runtime_error("Unable to successfully run Fit sub-algorithm");
      }

      MatrixWorkspace_sptr wsn = fit2_alg->getProperty("OutputWorkspace");
      const MantidVec & FitValues = wsn->readY(1);
      std::cout << n<<"  "<<FitValues[n-1]<<"\n";
      for (int i=0; i < n; i++) {
        Y[i] = FitValues[i];
      }
      I = 0.0;
      for (int i=0; i < n; i++) I+=FitValues[i];
      I*=double(DataValues.size())/double(n);
      sigI = sqrt(I+DataValues.size()*bkg0+DataValues.size()*DataValues.size()*bkg0);
      return;
    }



  } // namespace Algorithm
} // namespace Mantid
