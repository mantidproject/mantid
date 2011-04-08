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

      declareProperty(new WorkspaceProperty<>("InputWorkspace", "", Direction::Input)
          , "A 2D workspace with X values of time of flight");
      declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "Name of the output workspace.");

      declareProperty(new WorkspaceProperty<PeaksWorkspace>("InPeaksWorkspace", "", Direction::Input), "Name of the peaks workspace.");

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

    }

    /** Executes the algorithm
     *
     *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
     */
    void PeakIntegration::exec()
    {
      retrieveProperties();
      Alpha0 = 1.89157;
      Alpha1 = 1.69777;
      Beta0 = 9.38995;
      Kappa = 4.3558;
      SigmaSquared = 396.9;
      Gamma = 0.57103;
  
      // create TOF axis from Params
      const std::vector<double> rb_params = getProperty("Params");
      MantidVecPtr XValues;
      const int numbins = VectorHelper::createAxisFromRebinParams(rb_params, XValues.access());
      PeaksWorkspace_sptr peaksW;
      peaksW = boost::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve(getProperty("InPeaksWorkspace")));


      int i, XPeak, YPeak, h, k, l, ipk, detnum;
      double col, row, chan, l1, l2, wl, I, sigI;

      std::vector <std::pair<int, int> > v1;

      for (i = 0; i<peaksW->getNumberPeaks(); i++)
      v1.push_back(std::pair<int, int>(peaksW->get_ipk(i),i));
      // To sort in descending order
      stable_sort(v1.rbegin(), v1.rend() );
 
      std::vector <std::pair<int, int> >::iterator Iter1;
      for ( Iter1 = v1.begin() ; Iter1 != v1.end() ; Iter1++ )
      {
        i = (*Iter1).second;
        l1 = peaksW->get_L1(i);
        detnum = peaksW->get_Bank(i);
        Geometry::V3D hkl = peaksW->get_hkl(i);
        h = hkl[0];
        k = hkl[1];
        l = hkl[2];
        col = peaksW->get_column(i);
        row = peaksW->get_row(i);
        chan = peaksW->get_time_channel(i);
        Geometry::V3D pos = peaksW->getPosition(i);
        l2 = pos.norm();
        wl = peaksW->get_wavelength(i);
        ipk = peaksW->get_ipk(i);

        std::ostringstream Peakbank;
        Peakbank <<"bank"<<detnum;
        XPeak = int(col+0.5)-1;
        YPeak = int(row+0.5)-1;
        tofISAW = wl * (l1+l2) / 3.956058e-3;
        TOFPeak = VectorHelper::getBinIndex(XValues.access(), tofISAW);
  
  
        TOFmin = TOFPeak+Binmin;
        if (TOFmin<0) TOFmin = 0;
        TOFmax = TOFPeak+Binmax;
  
        IAlgorithm_sptr sum_alg;
        try
        {
          sum_alg = createSubAlgorithm("SumNeighbours", -1, -1, false);
        } 
        catch (Exception::NotFoundError&)
        {
          g_log.error("Can't locate SumNeighbours algorithm");
          throw ;
        }
        sum_alg->setProperty("InputWorkspace", inputW);
        sum_alg->setProperty("OutputWorkspace", "tmp");
        sum_alg->setProperty("SumX", Xmax-Xmin+1);
        sum_alg->setProperty("SumY", Ymax-Ymin+1);
        sum_alg->setProperty("SingleNeighbourhood", true);
        sum_alg->setProperty("Xpixel", XPeak+Xmin);
        sum_alg->setProperty("Ypixel", YPeak+Ymin);
        sum_alg->setProperty("DetectorName", Peakbank.str());
  
        try
        {
          sum_alg->execute();
        }
        catch (std::runtime_error&)
        {
          g_log.error("Unable to successfully run SumNeighbours sub-algorithm");
          throw;
        }
        outputW = sum_alg->getProperty("OutputWorkspace");
  
        IAlgorithm_sptr bin_alg = createSubAlgorithm("Rebin");
        bin_alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", outputW);
        bin_alg->setProperty("OutputWorkspace", outputW);
        bin_alg->setProperty("PreserveEvents", false);
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
  
        fitSpectra(0, I, sigI);
        std::cout << peaksW->getPeakIntegrationCount(i)<<"  " << I << "  "<<peaksW->getPeakIntegrationError(i)<< "  "<< sigI << "\n";
  
        peaksW->setPeakIntegrateCount(I, i);
        peaksW->setPeakIntegrateError(sigI, i);
      }
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
      if (Xmin >=  Xmax)
        throw std::runtime_error("Must specify Xmin<Xmax");
      if (Ymin >=  Ymax)
        throw std::runtime_error("Must specify Ymin<Ymax");
      if (Binmin >=  Binmax)
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
      MantidVec & X = outputW->dataX(s);
      MantidVec & Y = outputW->dataY(s);

      double bktime = X[TOFmin] + X[TOFmax];
      double bkg0 = Y[TOFmin] + Y[TOFmax];
      double pktime = 0.0;
      for (int i = TOFmin; i < TOFmax; i++) pktime+= X[i];
      double ratio = pktime/bktime;

      const double peakLoc = X[TOFPeak];
      const double peakHeight =  Y[TOFPeak]*(X[TOFPeak]-X[TOFPeak-1]);//Intensity*HWHM

      IAlgorithm_sptr fit_alg;
      try
      {
        fit_alg = createSubAlgorithm("Fit", -1, -1, false);
      } catch (Exception::NotFoundError&)
      {
        g_log.error("Can't locate Fit algorithm");
        throw ;
      }
      fit_alg->setProperty("InputWorkspace", outputW);
      fit_alg->setProperty("WorkspaceIndex", s);
      fit_alg->setProperty("StartX", X[TOFmin]);
      fit_alg->setProperty("EndX", X[TOFmax]);
      fit_alg->setProperty("MaxIterations", 5000);
      fit_alg->setProperty("Output", "fit");
      std::ostringstream fun_str;
      fun_str << "name=IkedaCarpenterPV,I="<<peakHeight<<",Alpha0="<<Alpha0<<",Alpha1="<<Alpha1<<",Beta0="<<Beta0<<",Kappa="<<Kappa<<",SigmaSquared="<<SigmaSquared<<",Gamma="<<Gamma<<",X0="<<peakLoc;
      /*fun_str << "name = Gaussian, Height = "<<peakHeight<<", PeakCentre = "<<peakLoc<<", Sigma = "<<SigmaSquared<<";name=Lorentzian, Height = "
              <<peakHeight<<", PeakCentre = "<<peakLoc<<", HWHM = "<<Gamma;*/
      fit_alg->setProperty("Function", fun_str.str());
      std::ostringstream tie_str;

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
      const MantidVec &  DataValues = ws->readY(0);

      double chisq = fit_alg->getProperty("Output Chi^2/DoF");
      if(chisq < 0.0) // Find some chisq for a good fit to initialize parameters for next peak
      {
        std::vector<double> params = fit_alg->getProperty("Parameters");
        IKI = params[0];
        Alpha0 = params[1];
        Alpha1 = params[2];
        Beta0 = params[3];
        Kappa = params[4];
        SigmaSquared = params[5];
        Gamma = params[6];
        X0 = params[7];
      }
      std::string funct = fit_alg->getPropertyValue("Function");
      std::cout <<funct<<"\n";

      setProperty("OutputWorkspace", ws);

      IFitFunction *out = FunctionFactory::Instance().createInitialized(funct);
      IPeakFunction *pk = dynamic_cast<IPeakFunction *>(out);
      const int n=1000;
      double *x = new double[n];
      double *y = new double[n];
      double dx=(X[TOFmax]-X[TOFmin])/double(n-1);
      for (int i=0; i < n; i++) 
      {
        x[i] = X[TOFmin]+i*dx;
      }
      pk->setMatrixWorkspace(outputW, 0, -1, -1);
      pk->function(y,x,n);

      I = 0.0;
      for (int i = 0; i < n; i++) I+= y[i];
      I*= double(DataValues.size())/double(n);
      sigI = sqrt(I+ratio*ratio*bkg0);
      I-= ratio*bkg0;
      delete [] x;
      delete [] y;
      return;
    }



  } // namespace Algorithm
} // namespace Mantid
