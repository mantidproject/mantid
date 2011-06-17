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
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include <boost/algorithm/string.hpp>
#include "MantidKernel/VectorHelper.h"

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
      declareProperty(new WorkspaceProperty<PeaksWorkspace>("InPeaksWorkspace", "", Direction::Input), "Name of the peaks workspace.");

      declareProperty(new WorkspaceProperty<PeaksWorkspace>("OutPeaksWorkspace", "", Direction::Output), "Name of the output peaks workspace with integrated intensities.");

      declareProperty("XMin", -7, "Minimum of X (col) Range to integrate for peak");
      declareProperty("XMax", 7, "Maximum of X (col) Range to integrate for peak");
      declareProperty("YMin", -7, "Minimum of Y (row) Range to integrate for peak");
      declareProperty("YMax", 7, "Maximum of Y (row) Range to integrate for peak");
      declareProperty("TOFBinMin", -4, "Minimum of TOF Bin Range to integrate for peak");
      declareProperty("TOFBinMax", 6, "Maximum of TOF Bin Range to integrate for peak");
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
  
      /// Input peaks workspace
      PeaksWorkspace_sptr inPeaksW = getProperty("InPeaksWorkspace");

      /// Output peaks workspace, create if needed
      PeaksWorkspace_sptr peaksW = getProperty("OutPeaksWorkspace");
      if (peaksW != inPeaksW)
        peaksW = inPeaksW->clone();



      IAlgorithm_sptr sort_alg = createSubAlgorithm("SortEvents");
      sort_alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputW);
      sort_alg->setPropertyValue("SortBy", "X Value");
      sort_alg->executeAsSubAlg();

      inputW = sort_alg->getProperty("InputWorkspace");

      IAlgorithm_sptr bin_alg = createSubAlgorithm("Rebin");
      bin_alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputW);
      bin_alg->setProperty("OutputWorkspace", inputW);
      const std::vector<double> rb_params = getProperty("Params");
      bin_alg->setProperty<std::vector<double> >("Params", rb_params);
      bin_alg->executeAsSubAlg();

      inputW = bin_alg->getProperty("OutputWorkspace");

      IInstrument_sptr inst = inputW->getInstrument();
      if (!inst)
        throw std::runtime_error("The InputWorkspace does not have a valid instrument attached to it!");
    
    
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
                        detList.push_back(det);
                    }
                  }
                }
              }
            }
          }
        }
      }

      //Get some stuff from the input workspace
      const int YLength = static_cast<int>(inputW->blocksize());
      outputW =API::WorkspaceFactory::Instance().create(inputW,detList.size(),YLength+1,YLength);
      //Copy geometry over.
      API::WorkspaceFactory::Instance().initializeFromParent(inputW, outputW, false);
      Progress prog(this, 0.0, 1.0, detList.size());
      PARALLEL_FOR3(inputW, peaksW, outputW)
      for (int det = 0; det < static_cast<int>(detList.size()); det++)
      {
        PARALLEL_START_INTERUPT_REGION
        // Build a map to sort by the peak bin count
        std::vector <std::pair<double, int> > v1;
        for (int i = 0; i<peaksW->getNumberPeaks(); i++)if(detList[det]->getName().compare(peaksW->getPeaks()[i].getBankName())==0)
          v1.push_back(std::pair<double, int>(peaksW->getPeaks()[i].getBinCount(), i));
        if(v1.empty())continue;
  
        // To sort in descending order
        stable_sort(v1.rbegin(), v1.rend() );
        // Save mask for weak peaks
        int ***mask = new int**[Xmax-Xmin+1];
        for (int i = 0; i < Xmax-Xmin+1; ++i) 
        {
          mask[i] = new int*[Ymax-Ymin+1];
          for (int j = 0; j < Ymax-Ymin+1; ++j) 
          {
            mask[i][j] = new int[Binmax-Binmin+1];
          }
        }
        bool haveMask = false;
        //bool haveFit = false;
  
        std::vector <std::pair<double, int> >::iterator Iter1;
        for ( Iter1 = v1.begin() ; Iter1 != v1.end() ; Iter1++ )
        {
          // Index in the peaksworkspace
          int i = (*Iter1).second;
          double PeakIntensity = (*Iter1).first;
  
          // Direct ref to that peak
          Peak & peak = peaksW->getPeaks()[i];
  
          double col = peak.getCol();
          double row = peak.getRow();
          Kernel::V3D pos = peak.getDetPos();
  
          //Average integer postion; subtract 1 because ISAW starts at 1 not 0
          int XPeak = int(col+0.5)-2;
          int YPeak = int(row+0.5)-2;
          double TOFPeakd = peak.getTOF();
  
          sumneighbours(peak.getBankName(), XPeak+Xmin, YPeak+Ymin, Xmax-Xmin+1, Ymax-Ymin+1, TOFPeakd, haveMask, PeakIntensity, mask, static_cast<int>(det));
  
          double I, sigI;
          //Initialize Ikeda-Carpender function variables
          double Alpha0 = 1.6;
          double Alpha1 = 1.5;
          double Beta0 = 31.9;
          double Kappa = 46.0;
          // Find point of peak centre
          // Get references to the current spectrum
          const MantidVec& Y = outputW->readY(det);
          int numbins = static_cast<int>(Y.size());
          const MantidVec& E = outputW->readE(det);
          MantidVec& X = outputW->dataX(det);
          int TOFPeak = VectorHelper::getBinIndex(X, TOFPeakd);
          int TOFmin = TOFPeak+Binmin;
          if (TOFmin<0) TOFmin = 0;
          int TOFmax = TOFPeak+Binmax;
          if (TOFmin > numbins) TOFmin = numbins;
          if (TOFmax > numbins) TOFmax = numbins;
    
    
          const double peakLoc = X[TOFPeak];
          int iSig;
          for (iSig=TOFmin; iSig < TOFmax; iSig++) 
          {
            if(Y[iSig]>0.0 && Y[iSig+1]>0.0)break;
          }
          TOFmin = iSig;
          for (iSig=TOFmax; iSig > TOFmin; iSig--) 
          {
            if(Y[iSig]>0.0 && Y[iSig-1]>0.0)break;
          }
          TOFmax = iSig;
          if(TOFmax <= TOFmin)break;
          for (iSig=TOFmin; iSig <= TOFmax; iSig++) 
          {
            if(((Y[iSig]-Y[TOFPeak]/2.)*(Y[iSig+1]-Y[TOFPeak]/2.))<0.)break;
          }
          double Gamma = X[TOFPeak]-X[iSig];
          double SigmaSquared = Gamma*Gamma;
          std::cout <<"Y ";
          for (iSig=TOFmin; iSig <= TOFmax; iSig++)std::cout <<Y[iSig]<<"  "; 
          std::cout <<"\n";
          double bktime = X[TOFmin] + X[TOFmax];
          double bkg0 = Y[TOFmin] + Y[TOFmax];
          double pktime = 0.0;
          for (int i = TOFmin; i < TOFmax; i++) pktime+= X[i];
          double ratio = pktime/bktime;
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
          fit_alg->setProperty("InputWorkspace", outputW);
          fit_alg->setProperty("WorkspaceIndex", det);
          fit_alg->setProperty("StartX", X[TOFmin]);
          fit_alg->setProperty("EndX", X[TOFmax]);
          fit_alg->setProperty("MaxIterations", 5000);
          //fit_alg->setProperty("Output", "fit");
          std::ostringstream fun_str;
          fun_str << "name=IkedaCarpenterPV,I="<<peakHeight<<",Alpha0="<<Alpha0<<",Alpha1="<<Alpha1<<",Beta0="<<Beta0<<",Kappa="<<Kappa<<",SigmaSquared="<<SigmaSquared<<",Gamma="<<Gamma<<",X0="<<peakLoc;
          fit_alg->setProperty("Function", fun_str.str());
          if(Alpha0 != 1.6 || Alpha1 != 1.5 || Beta0 != 31.9 || Kappa != 46.0)
          {
            std::ostringstream tie_str;
            tie_str << "Alpha0="<<Alpha0<<",Alpha1="<<Alpha1<<",Beta0="<<Beta0<<",Kappa="<<Kappa;
            fit_alg->setProperty("Ties", tie_str.str());
          }
          fit_alg->executeAsSubAlg();
          //MatrixWorkspace_sptr ws = fit_alg->getProperty("OutputWorkspace");
    
          double chisq = fit_alg->getProperty("OutputChi2overDoF");
          /*if(chisq > 0 && chisq < 400 && !haveFit && PeakIntensity < 30.0) // use fit of strong peaks for weak peaks
          {
            std::vector<double> params = fit_alg->getProperty("Parameters");
            Alpha0 = params[1];
            Alpha1 = params[2];
            Beta0 = params[3];
            Kappa = params[4];
            haveFit = true;
          }*/
          std::string funct = fit_alg->getPropertyValue("Function");
    
          //setProperty("OutputWorkspace", ws);
    
          //Evaluate fit at points
          IFitFunction *out = FunctionFactory::Instance().createInitialized(funct);
          IPeakFunction *pk = dynamic_cast<IPeakFunction *>(out);
          const int n = TOFmax-TOFmin+1;
          double *x = new double[n];
          double *y = new double[n];
          //double dx=(X[TOFmax]-X[TOFmin])/double(n-1);
          for (int i=0; i < n; i++) 
          {
            x[i] = X[TOFmin+i];
            //x[i] = X[TOFmin]+i*dx;
          }
          pk->setMatrixWorkspace(outputW, det, -1, -1);
          pk->function(y,x,n);
    
          //Calculate intensity
          I = 0.0;
          
          if(chisq>0.0 && chisq<400.0)
          {
            for (int i = 0; i < n; i++) if ( !boost::math::isnan(y[i]) && !boost::math::isinf(y[i]))I+= y[i];
            if(I>1e10)I=0.0;
            if(I==0.0)for (int i = TOFmin; i <= TOFmax; i++)I+=Y[i];
          }
          else
            for (int i = TOFmin; i <= TOFmax; i++)I+=Y[i];
          I-= ratio*bkg0;
    
          //Calculate errors correctly for nonPoisson distributions
          sigI = 0.0;
          for (int i = TOFmin; i <= TOFmax; i++) sigI+= E[i]*E[i];
          double sigbkg0 = E[TOFmin]*E[TOFmin] + E[TOFmax]*E[TOFmax];
          sigI = sqrt(sigI+ratio*ratio*sigbkg0);
    
          delete [] x;
          delete [] y;
          std::cout << "Intensity "<<PeakIntensity<< "  "<<TOFmax-TOFmin+1<<" chisq "<<chisq<<"  "<<peak.getBankName()<< "  "<<peak.getIntensity()<<"  " << I << "  " << peak.getSigmaIntensity() << "  "<< sigI <<"\n";
          peak.setIntensity(I);
          peak.setSigmaIntensity(sigI);
  
        }
        prog.report(detList[det]->getName());
        delete [] **mask;
        delete [] *mask;
        delete [] mask;
        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION

      // Save the output
      setProperty("OutPeaksWorkspace", peaksW);
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



/** Executes the algorithm
 *
 */
void PeakIntegration::sumneighbours(std::string det_name, int x0, int y0, int SumX, int SumY, double TOFPeakd, bool &haveMask, double PeakIntensity, int ***mask, int idet)
{

  //Get some stuff from the input workspace
  IInstrument_sptr inst = inputW->getInstrument();
  if (!inst)
    throw std::runtime_error("The InputWorkspace does not have a valid instrument attached to it!");

  //To get the workspace index from the detector ID
  detid2index_map * pixel_to_wi = inputW->getDetectorIDToWorkspaceIndexMap(true);

  int outWI = 0;
  //std::cout << " inst->nelements() " << inst->nelements() << "\n";

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

  if (detList.size() == 0)
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
          EventList outEL;
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
          for (int ix=0; ix < SumX; ix++)
            for (int iy=0; iy < SumY; iy++)
            {
              //Find the pixel ID at that XY position on the rectangular detector
              int pixelID = det->getAtXY(x+ix,y+iy)->getID();

              //Find the corresponding workspace index, if any
              if (pixel_to_wi->find(pixelID) != pixel_to_wi->end())
              {
                size_t wi = (*pixel_to_wi)[pixelID];
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
            outWI++;
          }
    }
  }

  haveMask = true;
  //Clean up memory
  delete pixel_to_wi;

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
      mask[i][j] = 0.0;
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
