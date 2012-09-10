/*WIKI* 


Integrate and calculate error of integration of each peak from single crystal data and place results into peak workspace.  Uses IkedaCarpenter function to fit TOF.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCrystal/PeakIntegration.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/ArrayProperty.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <fstream>
#include <ostream>
#include <iomanip>
#include <sstream>
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include <boost/algorithm/string.hpp>
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidAPI/MemoryManager.h"


namespace Mantid
{
  namespace Crystal
  {

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(PeakIntegration)

    using namespace Kernel;
    using namespace Geometry;
    using namespace API;
    using namespace DataObjects;


    /// Constructor
    PeakIntegration::PeakIntegration() :
      API::Algorithm(), pixel_to_wi(NULL)
    {}

    /// Destructor
    PeakIntegration::~PeakIntegration()
    {
      delete pixel_to_wi;
    }

    /** Initialisation method. Declares properties to be used in algorithm.
     *
     */
    void PeakIntegration::init()
    {

      declareProperty(new WorkspaceProperty<PeaksWorkspace>("InPeaksWorkspace", "", Direction::Input), "Name of the peaks workspace.");
      declareProperty(new WorkspaceProperty<>("InputWorkspace", "", Direction::Input, boost::make_shared<InstrumentValidator>()),
                      "A 2D workspace with X values of time of flight");
      declareProperty(new WorkspaceProperty<PeaksWorkspace>("OutPeaksWorkspace", "", Direction::Output), "Name of the output peaks workspace with integrated intensities.");
      declareProperty("IkedaCarpenterTOF", false, "Integrate TOF using IkedaCarpenter fit.\n"
        "Default is false which is best for corrected data.");
      declareProperty("MatchingRunNo", true, "Integrate only peaks where run number of peak matches run number of sample.\n"
        "Default is true.");
      declareProperty("FitSlices", true, "Integrate slices using IntegratePeakTimeSlices algorithm (default).\n"
        "If false, then next 6 variables are used for shoebox with pixels masked that are outside peak cluster.");

      declareProperty("XMin", -7, "Minimum of X (col) Range to integrate for peak");
      declareProperty("XMax", 7, "Maximum of X (col) Range to integrate for peak");
      declareProperty("YMin", -7, "Minimum of Y (row) Range to integrate for peak");
      declareProperty("YMax", 7, "Maximum of Y (row) Range to integrate for peak");
      declareProperty("TOFBinMin", -4, "Minimum of TOF Bin Range to integrate for peak");
      declareProperty("TOFBinMax", 6, "Maximum of TOF Bin Range to integrate for peak");
      setPropertySettings("XMin", new VisibleWhenProperty("FitSlices", IS_NOT_DEFAULT) );
      setPropertySettings("XMax", new VisibleWhenProperty("FitSlices", IS_NOT_DEFAULT) );
      setPropertySettings("YMin", new VisibleWhenProperty("FitSlices", IS_NOT_DEFAULT) );
      setPropertySettings("YMax", new VisibleWhenProperty("FitSlices", IS_NOT_DEFAULT) );
      setPropertySettings("TOFBinMin", new VisibleWhenProperty("FitSlices", IS_NOT_DEFAULT) );
      setPropertySettings("TOFBinMax", new VisibleWhenProperty("FitSlices", IS_NOT_DEFAULT) );

      std::string grp1 = "ShoeBox Limits";
      setPropertyGroup("XMin", grp1);
      setPropertyGroup("XMax", grp1);
      setPropertyGroup("YMin", grp1);
      setPropertyGroup("YMax", grp1);
      setPropertyGroup("TOFBinMin", grp1);
      setPropertyGroup("TOFBinMax", grp1);

    }

    /** Executes the algorithm
     *
     *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
     */
    void PeakIntegration::exec()
    {
      retrieveProperties();

      /// Input peaks workspace
      PeaksWorkspace_sptr inPeaksW = getProperty("InPeaksWorkspace");

      /// Output peaks workspace, create if needed
      PeaksWorkspace_sptr peaksW = getProperty("OutPeaksWorkspace");
      if (peaksW != inPeaksW)
        peaksW = inPeaksW->clone();


      double qspan = 0.12;
      bool slices = getProperty("FitSlices");
      IC = getProperty("IkedaCarpenterTOF");
      bool matchRun = getProperty("MatchingRunNo");
      if (slices)
      {
        if (peaksW->mutableSample().hasOrientedLattice())
        {
          OrientedLattice latt = peaksW->mutableSample().getOrientedLattice();
          qspan = 1./std::max(latt.a(), std::max(latt.b(),latt.c()));//1/6*2Pi about 1

        }
        else
        {
          qspan = 0.12;
        }
        
      }


      //To get the workspace index from the detector ID
      pixel_to_wi = inputW->getDetectorIDToWorkspaceIndexMap(false);//true);

      //Sort events if EventWorkspace so it will run in parallel
      EventWorkspace_const_sptr inWS = boost::dynamic_pointer_cast<const EventWorkspace>( inputW );
      if (inWS)
      {
        inWS->sortAll(TOF_SORT, NULL);
      }


      //Get some stuff from the input workspace
      const int YLength = static_cast<int>(inputW->blocksize());
      outputW =API::WorkspaceFactory::Instance().create(inputW,peaksW->getNumberPeaks(),YLength+1,YLength);
      //Copy geometry over.
      API::WorkspaceFactory::Instance().initializeFromParent(inputW, outputW, true);
      size_t Numberwi = inputW->getNumberHistograms();
      int NumberPeaks = peaksW->getNumberPeaks();
      int MinPeaks = 0;


      for (int i = NumberPeaks-1; i >= 0; i--)
      {
        Peak & peak = peaksW->getPeaks()[i];
        int pixelID = peak.getDetectorID();

        // Find the workspace index for this detector ID
        if (pixel_to_wi->find(pixelID) != pixel_to_wi->end())
        {
          size_t wi = (*pixel_to_wi)[pixelID];
          if((matchRun && peak.getRunNumber() != inputW->getRunNumber()) || wi >= Numberwi) peaksW->removePeak(i);
        }
        else  // This is for appending peak workspaces when running SNSSingleCrystalReduction one bank at at time
          if(i+1 > MinPeaks) MinPeaks = i+1;
      }
      NumberPeaks = peaksW->getNumberPeaks();
      if (NumberPeaks <= 0)
      {
        g_log.error("RunNumbers of InPeaksWorkspace and InputWorkspace do not match");
        return;
      }


      Progress prog(this, MinPeaks, 1.0, NumberPeaks);
      PARALLEL_FOR3(inputW, peaksW, outputW)
      for (int i = MinPeaks; i< NumberPeaks; i++)
      {

        PARALLEL_START_INTERUPT_REGION
  
        // Direct ref to that peak
        Peak & peak = peaksW->getPeaks()[i];


        double col = peak.getCol();
        double row = peak.getRow();

        //Average integer postion
        int XPeak = int(col+0.5);
        int YPeak = int(row+0.5);

        double TOFPeakd = peak.getTOF();
        std::string bankName = peak.getBankName();

        boost::shared_ptr<const IComponent> parent = inputW->getInstrument()->getComponentByName(bankName);

        if (!parent) continue;

        if (parent->type().compare("RectangularDetector") != 0 && !slices) continue;
  
        int TOFPeak=0, TOFmin=0, TOFmax=0;
        if (slices)
        {
          TOFmax = fitneighbours(i, bankName, XPeak, YPeak, i, qspan ,peaksW);

          MantidVec& X = outputW->dataX(i);
          TOFPeak = VectorHelper::getBinIndex(X, TOFPeakd);
        }
        else
        {
          bool haveMask = false;
          // Save mask for weak peaks
          int ***mask = new int**[Xmax-Xmin+1];
          for (int k = 0; k < Xmax-Xmin+1; ++k)
          {
            mask[k] = new int*[Ymax-Ymin+1];
            for (int j = 0; j < Ymax-Ymin+1; ++j)
            {
              mask[k][j] = new int[Binmax-Binmin+1];
            }
          }
          double PeakIntensity = peak.getBinCount();
          MantidVec& X = inputW->dataX(i);
          TOFPeak = VectorHelper::getBinIndex(X, TOFPeakd);
          TOFmin = TOFPeak+Binmin;
          if (TOFmin<0) TOFmin = 0;
          TOFmax = TOFPeak+Binmax;
          sumneighbours(peak.getBankName(), XPeak+Xmin, YPeak+Ymin, Xmax-Xmin+1, Ymax-Ymin+1, TOFPeakd, haveMask, PeakIntensity, mask, i);
        }

        double I=0., sigI=0.;
        // Find point of peak centre
        // Get references to the current spectrum
        const MantidVec& X = outputW->readX(i);
        const MantidVec& Y = outputW->readY(i);
        const MantidVec& E = outputW->readE(i);
        int numbins = static_cast<int>(Y.size());
        if (TOFmin > numbins) TOFmin = numbins;
        if (TOFmax > numbins) TOFmax = numbins;
    
        const double peakLoc = X[TOFPeak];
        int iTOF;
        for (iTOF=TOFmin; iTOF < TOFmax; iTOF++) 
        {
          if(Y[iTOF]>0.0 && Y[iTOF+1]>0.0)break;
        }
        TOFmin = iTOF;
        for (iTOF=TOFmax; iTOF > TOFmin; iTOF--) 
        {
          if(Y[iTOF]>0.0 && Y[iTOF-1]>0.0)break;
        }
        TOFmax = iTOF;
        if(TOFmax <= TOFmin)continue;
        const int n = TOFmax-TOFmin+1;
        double pktime = 0.0;

        for (iTOF = TOFmin; iTOF < TOFmax; iTOF++) pktime+= X[iTOF];
        if(n >= 8 && IC)//Number of fitting parameters large enough if Ikeda-Carpenter fit
        {
          for (iTOF=TOFmin; iTOF <= TOFmax; iTOF++) 
          {
            if(((Y[iTOF]-Y[TOFPeak]/2.)*(Y[iTOF+1]-Y[TOFPeak]/2.))<0.)break;
          }
          double Gamma = fabs(X[TOFPeak]-X[iTOF]);
          double SigmaSquared = Gamma*Gamma;
          const double peakHeight =  Y[TOFPeak]*Gamma;//Intensity*HWHM
      
          IAlgorithm_sptr fit_alg;
          try
          {
            fit_alg = createSubAlgorithm("Fit", -1, -1, false);
          } catch (Exception::NotFoundError&)
          {
            g_log.error("Can't locate Fit algorithm");
            throw ;
          }
          //Initialize Ikeda-Carpender function variables
          double Alpha0 = 1.6;
          double Alpha1 = 1.5;
          double Beta0 = 31.9;
          double Kappa = 46.0;
          std::ostringstream fun_str;
          fun_str << "name=IkedaCarpenterPV,I="<<peakHeight<<",Alpha0="<<Alpha0<<",Alpha1="<<Alpha1<<",Beta0="<<Beta0<<",Kappa="<<Kappa<<",SigmaSquared="<<SigmaSquared<<",Gamma="<<Gamma<<",X0="<<peakLoc;
          fit_alg->setPropertyValue("Function", fun_str.str());
          if(Alpha0 != 1.6 || Alpha1 != 1.5 || Beta0 != 31.9 || Kappa != 46.0)
          {
            std::ostringstream tie_str;
            tie_str << "Alpha0="<<Alpha0<<",Alpha1="<<Alpha1<<",Beta0="<<Beta0<<",Kappa="<<Kappa;
            fit_alg->setProperty("Ties", tie_str.str());
          }
          fit_alg->setProperty("InputWorkspace", outputW);
          fit_alg->setProperty("WorkspaceIndex", i);
          fit_alg->setProperty("StartX", X[TOFmin]);
          fit_alg->setProperty("EndX", X[TOFmax]);
          fit_alg->setProperty("MaxIterations", 5000);
          fit_alg->setProperty("CreateOutput", true);
          fit_alg->setProperty("Output", "fit");
          fit_alg->executeAsSubAlg();
          std::string wsname("fit_Workspace");
          MatrixWorkspace_sptr fitWS = fit_alg->getProperty("OutputWorkspace");
      
          /*double chisq = fit_alg->getProperty("OutputChi2overDoF");
          if(chisq > 0 && chisq < 400 && !haveFit && PeakIntensity < 30.0) // use fit of strong peaks for weak peaks
          {
            std::vector<double> params = fit_alg->getProperty("Parameters");
            Alpha0 = params[1];
            Alpha1 = params[2];
            Beta0 = params[3];
            Kappa = params[4];
            haveFit = true;
          }*/
          std::string funct = fit_alg->getPropertyValue("Function");
      
      
          //Evaluate fit at points
          const Mantid::MantidVec& y = fitWS->readY(1);
      
          //Calculate intensity
          for (iTOF = 0; iTOF < n; iTOF++) if ( !boost::math::isnan(y[iTOF]) && !boost::math::isinf(y[iTOF]))I+= y[iTOF];
        }
        else
          for (iTOF = TOFmin; iTOF <= TOFmax; iTOF++)I+=Y[iTOF];
    

        if (slices && !IC)
        {
          sigI = peak.getSigmaIntensity();
        }
        else
        {
          //Calculate errors correctly for nonPoisson distributions
          for (iTOF = TOFmin; iTOF <= TOFmax; iTOF++) sigI+= E[iTOF]*E[iTOF];
          sigI = sqrt(sigI);
        }

        peak.setIntensity(I);
        peak.setSigmaIntensity(sigI);

        prog.report();
        PARALLEL_END_INTERUPT_REGION

      }


      PARALLEL_CHECK_INTERUPT_REGION

      // Save the output
      setProperty("OutPeaksWorkspace", peaksW);
    }



    void PeakIntegration::retrieveProperties()
    {
      inputW = getProperty("InputWorkspace");
      if (inputW->readY(0).size() <= 1)
        throw std::runtime_error("Must Rebin data with more than 1 bin");
      //Check if detectors are RectangularDetectors
      bool slices = getProperty("FitSlices");
      Instrument_const_sptr inst = inputW->getInstrument();
      boost::shared_ptr<RectangularDetector> det;
      for (int i=0; i < inst->nelements(); i++)
      {
        det = boost::dynamic_pointer_cast<RectangularDetector>( (*inst)[i] );
        if (det) break;
      }
      if (!det && !slices)
        throw std::runtime_error("PeakIntegration only works for instruments with Rectangular Detectors.");

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



/** Executes the algorithm
 *
 */
void PeakIntegration::sumneighbours(std::string det_name, int x0, int y0, int SumX, int SumY, double TOFPeakd, bool &haveMask, double PeakIntensity, int ***mask, int idet)
{

  //Get some stuff from the input workspace
  Instrument_const_sptr inst = inputW->getInstrument();

  //Build a list of Rectangular Detectors
  std::vector<boost::shared_ptr<RectangularDetector> > detList;
  for (int i=0; i < inst->nelements(); i++)
  {
    boost::shared_ptr<RectangularDetector> det;
    boost::shared_ptr<ICompAssembly> assem;
    boost::shared_ptr<ICompAssembly> assem2;

    det = boost::dynamic_pointer_cast<RectangularDetector>( (*inst)[i] );
    if (det) 
    {
      if(det_name.empty() || (!det_name.empty() && det->getName().compare(det_name)==0)) 
        detList.push_back(det);
    }
    else
    {
      //Also, look in the first sub-level for RectangularDetectors (e.g. PG3).
      // We are not doing a full recursive search since that will be very long for lots of pixels.
      assem = boost::dynamic_pointer_cast<ICompAssembly>( (*inst)[i] );
      if (assem)
      {
        for (int j=0; j < assem->nelements(); j++)
        {
          det = boost::dynamic_pointer_cast<RectangularDetector>( (*assem)[j] );
          if (det)
          {
            if(det_name.empty() || (!det_name.empty() && det->getName().compare(det_name)==0)) 
              detList.push_back(det);
          }
          else
          {
            //Also, look in the second sub-level for RectangularDetectors (e.g. PG3).
            // We are not doing a full recursive search since that will be very long for lots of pixels.
            assem2 = boost::dynamic_pointer_cast<ICompAssembly>( (*assem)[j] );
            if (assem2)
            {
              for (int k=0; k < assem2->nelements(); k++)
              {
                det = boost::dynamic_pointer_cast<RectangularDetector>( (*assem2)[k] );
                if (det) 
                {
                  if(det_name.empty() || (!det_name.empty() && det->getName().compare(det_name)==0))  
                    detList.push_back(det);
                }
              }
            }
          }
        }
      }
    }
  }

  if (detList.empty())
    throw std::runtime_error("This instrument does not have any RectangularDetector's. PeakIntegration cannot operate on this instrument at this time.");

  int TOFmin=0, TOFmax=0;
  //Loop through the RectangularDetector's we listed before.
  for (int i=0; i < static_cast<int>(detList.size()); i++)
  {
    std::string det_name("");
    boost::shared_ptr<RectangularDetector> det;
    det = detList[i];
    if (det)
    {
      int xend = det->xpixels();
      int yend = det->ypixels();
      if(x0 < 0)x0 = 0;
      if(x0 >= det->xpixels())x0 = det->xpixels()-1;
      xend = x0 + SumX;
      if(xend >= det->xpixels())SumX = det->xpixels()-x0;
      xend = x0 + SumX;
      if(y0 < 0)y0 = 0;
      if(y0 >= det->ypixels())y0 = det->ypixels()-1;
      yend = y0 + SumY;
      if(yend >= det->ypixels())SumY = det->ypixels()-y0;
      yend = y0 + SumY;
      int x, y;
      det_name = det->getName();
      //TODO: Check validity of the parameters

      //det->getAtXY()
      for (x=x0; x<xend; x += SumX)
        for (y=y0; y<yend; y += SumY)
        {
          //Initialize the output event list
          MantidVec& Xout=outputW->dataX(idet);
          MantidVec& Yout=outputW->dataY(idet);
          MantidVec& Eout=outputW->dataE(idet);
          Xout = inputW->readX(0);
          Yout = inputW->readY(0);
          Eout = inputW->readE(0);
    
          double ***data = new double**[SumX];
          for (int i = 0; i < SumX; ++i) 
          {
            data[i] = new double*[SumY];
            for (int j = 0; j < SumY; ++j) 
            {
              data[i][j] = new double[Binmax-Binmin+1];
            }
          }
          double ***error = new double**[SumX];
          for (int i = 0; i < SumX; ++i) 
          {
            error[i] = new double*[SumY];
            for (int j = 0; j < SumY; ++j) 
            {
              error[i][j] = new double[Binmax-Binmin+1];
            }
          }
          double **tmp = new double*[SumX];
          for (int i = 0; i < SumX; ++i) tmp[i] = new double[SumY];
          double **smtmp = new double*[SumX];
          for (int i = 0; i < SumX; ++i) smtmp[i] = new double[SumY];
          int **tmpmask = new int*[SumX];
          for (int i = 0; i < SumX; ++i) tmpmask[i] = new int[SumY];
          outputW->getSpectrum(idet)->clearDetectorIDs();
          for (int ix=0; ix < SumX; ix++)
            for (int iy=0; iy < SumY; iy++)
            {
              //Find the pixel ID at that XY position on the rectangular detector
              int pixelID = det->getAtXY(x+ix,y+iy)->getID();

              //Find the corresponding workspace index, if any
              if (pixel_to_wi->find(pixelID) != pixel_to_wi->end())
              {
                size_t wi = (*pixel_to_wi)[pixelID];
                //Set detectorIDs
                outputW->getSpectrum(idet)->addDetectorIDs( inputW->getSpectrum(wi)->getDetectorIDs() );
                MantidVec& X = inputW->dataX(wi);
                const MantidVec& Y = inputW->readY(wi);
                const MantidVec& E = inputW->readE(wi);
                int TOFPeak = VectorHelper::getBinIndex(X, TOFPeakd);
                TOFmin = TOFPeak+Binmin;
                if (TOFmin<0) TOFmin = 0;
                TOFmax = TOFPeak+Binmax;
                int numbins = static_cast<int>(Y.size());
                if (TOFmin > numbins) TOFmin = numbins;
                if (TOFmax > numbins) TOFmax = numbins;
          
                for (int i = TOFmin; i <= TOFmax; i++)
                {
                   data[ix][iy][i-TOFmin]=Y[i];
                   error[ix][iy][i-TOFmin]=E[i];
                }
          


              }
            }
            for (int k = TOFmin; k <= TOFmax; k++)
            {
              for (int i = 0; i < SumX; i++)
              {
                for (int j = 0; j < SumY; j++)
                {
                 tmp[i][j] = data[i][j][k-TOFmin];
                }
              }
              if(!haveMask || PeakIntensity > 30.0)
              //Large or medium peak
              {
                smooth(tmp,SumX,SumY,smtmp);
                cluster(smtmp,SumX,SumY,tmpmask);
                for (int i = 0; i < SumX; i++)
                {
                  for (int j = 0; j < SumY; j++)
                  {
                    mask[i][j][k-TOFmin] = tmpmask[i][j];
                  }
                }
              }
              else
              //Weak peak
              {
                for (int i = 0; i < SumX; i++)
                {
                  for (int j = 0; j < SumY; j++)
                  {
                    tmpmask[i][j] = mask[i][j][k-TOFmin];
                  }
                }
              }
              double sum = 0;
              double var = 0;
              for (int i = 0; i < SumX; i++)
              {
                for (int j = 0; j < SumY; j++)
                {
                  sum+=tmpmask[i][j]*tmp[i][j];
                  var+=tmpmask[i][j]*error[i][j][k-TOFmin]*error[i][j][k-TOFmin];
                }
              }
              Yout[k] = sum;
              Eout[k] = sqrt(var);
            }
            delete [] **data;
            delete [] *data;
            delete [] data;
            delete [] *error;
            delete [] error;
            delete [] *tmp;
            delete [] tmp;
            delete [] *smtmp;
            delete [] smtmp;
            delete [] *tmpmask;
            delete [] tmpmask;
          }
    }
  }

  haveMask = false;

}
int PeakIntegration::fitneighbours(int ipeak, std::string det_name, int x0, int y0, int idet, double qspan,
                                     PeaksWorkspace_sptr &Peaks)
{
  UNUSED_ARG( ipeak);
  UNUSED_ARG( det_name);
  UNUSED_ARG( x0);
  UNUSED_ARG( y0);
  API::IPeak& peak = Peaks->getPeak( ipeak);
  // Number of slices
  int TOFmax = 0;
  //Get some stuff from the input workspace
  /*   Instrument_const_sptr inst = inputW->getInstrument();

  //Build a list of Rectangular Detectors
 std::vector<boost::shared_ptr<RectangularDetector> > detList;
  for (int i=0; i < inst->nelements(); i++)
  {
    boost::shared_ptr<RectangularDetector> det;
    boost::shared_ptr<ICompAssembly> assem;
    boost::shared_ptr<ICompAssembly> assem2;

    det = boost::dynamic_pointer_cast<RectangularDetector>( (*inst)[i] );
    if (det) 
    {
      if(det_name.empty() || (!det_name.empty() && det->getName().compare(det_name)==0)) 
        detList.push_back(det);
    }
    else
    {
      //Also, look in the first sub-level for RectangularDetectors (e.g. PG3).
      // We are not doing a full recursive search since that will be very long for lots of pixels.
      assem = boost::dynamic_pointer_cast<ICompAssembly>( (*inst)[i] );
      if (assem)
      {
        for (int j=0; j < assem->nelements(); j++)
        {
          det = boost::dynamic_pointer_cast<RectangularDetector>( (*assem)[j] );
          if (det)
          {
            if(det_name.empty() || (!det_name.empty() && det->getName().compare(det_name)==0)) 
              detList.push_back(det);
          }
          else
          {
            //Also, look in the second sub-level for RectangularDetectors (e.g. PG3).
            // We are not doing a full recursive search since that will be very long for lots of pixels.
            assem2 = boost::dynamic_pointer_cast<ICompAssembly>( (*assem)[j] );
            if (assem2)
            {
              for (int k=0; k < assem2->nelements(); k++)
              {
                det = boost::dynamic_pointer_cast<RectangularDetector>( (*assem2)[k] );
                if (det) 
                {
                  if(det_name.empty() || (!det_name.empty() && det->getName().compare(det_name)==0))  
                    detList.push_back(det);
                }
              }
            }
          }
        }
      }
    }
  }

  if (detList.empty())
    throw std::runtime_error("This instrument does not have any RectangularDetector's. PeakIntegration cannot operate on this instrument at this time.");
*/
  //Loop through the RectangularDetector's we listed before.

 // for (int i=0; i < static_cast<int>(detList.size()); i++)
//  for( int i=0; i< (int)peaksW->rowCount();i++)
  {
    //std::string det_name("");
   // boost::shared_ptr<RectangularDetector> det;
   // det = detList[i];
   // if (det)
    {
      IAlgorithm_sptr slice_alg = createSubAlgorithm("IntegratePeakTimeSlices");
      slice_alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputW);
      std::ostringstream tab_str;
      tab_str << "LogTable" << ipeak;

      slice_alg->setPropertyValue("OutputWorkspace", tab_str.str());
      slice_alg->setProperty<PeaksWorkspace_sptr>("Peaks", Peaks);
      slice_alg->setProperty("PeakIndex", ipeak);
      slice_alg->setProperty("PeakQspan", qspan);
      slice_alg->executeAsSubAlg();
      Mantid::API::MemoryManager::Instance().releaseFreeMemory();

      MantidVec& Xout=outputW->dataX(idet);
      MantidVec& Yout=outputW->dataY(idet);
      MantidVec& Eout=outputW->dataE(idet);
      TableWorkspace_sptr logtable = slice_alg->getProperty("OutputWorkspace");

      peak.setIntensity( slice_alg->getProperty("Intensity"));
      peak.setSigmaIntensity(slice_alg->getProperty("SigmaIntensity"));


      TOFmax = static_cast<int>(logtable->rowCount());
      for (int iTOF=0; iTOF < TOFmax; iTOF++)
      {
        Xout[iTOF] = logtable->getRef<double>(std::string("Time"), iTOF);
        if(IC)//Ikeda-Carpenter fit
        {
          Yout[iTOF] = logtable->getRef<double>(std::string("TotIntensity"), iTOF);
          Eout[iTOF] = logtable->getRef<double>(std::string("TotIntensityError"), iTOF);
        }
        else
        {
          Yout[iTOF] = logtable->getRef<double>(std::string("ISAWIntensity"), iTOF);
          Eout[iTOF] = logtable->getRef<double>(std::string("ISAWIntensityError"), iTOF);
        }
      }

      outputW->getSpectrum(idet)->clearDetectorIDs();
      //Find the pixel ID at that XY position on the rectangular detector
      int pixelID = peak.getDetectorID();// det->getAtXY(x0,y0)->getID();

      //Find the corresponding workspace index, if any
      if (pixel_to_wi->find(pixelID) != pixel_to_wi->end())
      {
        size_t wi = (*pixel_to_wi)[pixelID];
        //Set detectorIDs
        outputW->getSpectrum(idet)->addDetectorIDs( inputW->getSpectrum(wi)->getDetectorIDs() );
      }
    }

  }

  return TOFmax-1;

}
void PeakIntegration::neighbours(double **matrix, int i, int j, int m, int n, int **mask)
{
  if(i<0 || i>=m || j<0 || j>=n) return;
  if(matrix[i][j] == 0)
     mask[i][j] = 0;
  else
  {
     double sigbkg = 1.;
     double down = (i+1>=m ? 0 : matrix[i+1][j]);
     if(down>sigbkg)mask[i][j]++;
     if(down>sigbkg && mask[i+1][j] == 0) neighbours(matrix, i+1, j, m, n, mask);
     double up = (i-1<0 ? 0 : matrix[i-1][j]);
     if(up>sigbkg)mask[i][j]++;
     if(up>sigbkg && mask[i-1][j] == 0) neighbours(matrix, i-1, j, m, n, mask);
     double left = (j-1<0 ? 0 : matrix[i][j-1]);
     if(left>sigbkg)mask[i][j]++;
     if(left>sigbkg && mask[i][j-1] == 0) neighbours(matrix, i, j-1, m, n, mask);
     double right = (j+1>=n ? 0 : matrix[i][j+1]);
     if(right>sigbkg)mask[i][j]++;
     if(right>sigbkg && mask[i][j+1] == 0) neighbours(matrix, i, j+1, m, n, mask);
     if(mask[i][j] > 1)mask[i][j] = 1;
  }
}
void PeakIntegration::cluster(double **matrix, int m, int n, int **mask)
{
  double max = matrix[0][0];
  int imax = 0;
  int jmax = 0;

  for (int i=0; i<m; i++)
  {
    for (int j=0; j<n; j++)
    {
      mask[i][j] = 0;
      if(matrix[i][j] > max)
      {
        max = matrix[i][j];
        imax = i;
        jmax = j;
      }
    }
  }
  neighbours(matrix, imax, jmax, m, n, mask);
}

void PeakIntegration::smooth(double **matrix, int m, int n, double **smmatrix)
{
  for (int i=0; i<m; i++)
  {
    for (int j=0; j<n; j++)
    {
      smmatrix[i][j] = 0.0;
      for (int k=-1; k<=1; k++)
      {
        for (int l=-1; l<=1; l++)
        {
          if(i+k < 0 || i+k >= m || j+l < 0 || j+l >= m)continue;
          smmatrix[i][j] += (3-abs(k)-abs(l))*matrix[i+k][j+l];
        }
      }
    }
  }
  for (int i=0; i<m; i++)
  {
    for (int j=0; j<n; j++)
    {
      smmatrix[i][j] /= 15.0;
    }
  }
}


} // namespace Crystal
} // namespace Mantid
