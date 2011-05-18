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
  namespace Crystal
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

  
      PeaksWorkspace_sptr peaksW;
      peaksW = boost::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve(getProperty("InPeaksWorkspace")));


      int i, XPeak, YPeak;
      double col, row, I, sigI;

      // Build a map to sort by the peak bin count
      std::vector <std::pair<double, int> > v1;
      for (i = 0; i<peaksW->getNumberPeaks(); i++)
        v1.push_back(std::pair<double, int>(peaksW->getPeaks()[i].getBinCount(), i));

      // To sort in descending order
      stable_sort(v1.rbegin(), v1.rend() );

      std::vector <std::pair<double, int> >::iterator Iter1;
      for ( Iter1 = v1.begin() ; Iter1 != v1.end() ; Iter1++ )
      {
        // Index in the peaksworkspace
        i = (*Iter1).second;

        // Direct ref to that peak
        Peak & peak = peaksW->getPeaks()[i];

        col = peak.getCol();
        row = peak.getRow();
        Geometry::V3D pos = peak.getDetPos();

        XPeak = int(col+0.5)-1;
        YPeak = int(row+0.5)-1;
        TOFPeakd = peak.getTOF();

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
        sum_alg->setProperty("DetectorName", peak.getBankName());
        sum_alg->executeAsSubAlg();
        outputW = sum_alg->getProperty("OutputWorkspace");

        IAlgorithm_sptr bin_alg = createSubAlgorithm("Rebin");
        bin_alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", outputW);
        bin_alg->setProperty("OutputWorkspace", outputW);
        bin_alg->setProperty("PreserveEvents", false);
        const std::vector<double> rb_params = getProperty("Params");
        bin_alg->setProperty<std::vector<double> >("Params", rb_params);
        bin_alg->executeAsSubAlg();

        outputW = bin_alg->getProperty("OutputWorkspace");

        fitSpectra(0, I, sigI);
        //std::cout << peak.getIntensity()<<"  " << I << "  " << peak.getSigmaIntensity() << "  "<< sigI << "\n";
        peak.setIntensity(I);
        peak.setSigmaIntensity(sigI);
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
      MantidVec& X = outputW->dataX(s);
      MantidVec& Y = outputW->dataY(s);
      TOFPeak = VectorHelper::getBinIndex(X, TOFPeakd);
      TOFmin = TOFPeak+Binmin;
      if (TOFmin<0) TOFmin = 0;
      TOFmax = TOFPeak+Binmax;
      int numbins = static_cast<int>(Y.size());
      if (TOFmin > numbins) TOFmin = numbins;
      if (TOFmax > numbins) TOFmax = numbins;
      std::cout <<TOFPeakd<<"  "<<TOFPeak<<"  "<<TOFmin<<"  "<<TOFmax<<"  "<<numbins<<"\n";
      for (int i = TOFmin; i < TOFmax; i++) std::cout << X[i] <<"  ";
      std::cout <<"\n";
      for (int i = TOFmin; i < TOFmax; i++) std::cout << Y[i] <<"  ";
      std::cout <<"\n";

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
      /*fun_str << "name = Gaussian, Height = "<<peakHeight<<", PeakCentre = "<<peakLoc<<", Sigma = 1.";*/
      fit_alg->setProperty("Function", fun_str.str());
      std::ostringstream tie_str;
      fit_alg->executeAsSubAlg();
      MatrixWorkspace_sptr ws = fit_alg->getProperty("OutputWorkspace");

      double chisq = fit_alg->getProperty("OutputChi2overDoF");
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
      I*= double(TOFmax-TOFmin+1)/double(n);
      sigI = sqrt(I+ratio*ratio*bkg0);
      I-= ratio*bkg0;
      delete [] x;
      delete [] y;
      return;
    }



  } // namespace Algorithm
} // namespace Mantid
