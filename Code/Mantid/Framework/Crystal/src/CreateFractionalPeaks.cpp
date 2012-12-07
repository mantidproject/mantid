/*WIKI*
 * This Algorithm creates a PeaksWorkspace with peaks occurring at specific fractional h,k,or l values.
 *
 * There are options to create Peaks offset from peaks from the input PeaksWorkspace, or to create peaks offset
 * from h,k, and l values in a range.  There is an option to include offsets=0 in this new PeakWorkspace
 *
 * The input PeaksWorkspace must contain an orientation matrix and have been INDEXED by THIS MATRIX if the new
 * peaks are not created from a range of h ,k, and l values
 *
 * WIKI*/
/*
 * CreateFractionalPeaks.cpp
 *
 *  Created on: Dec 5, 2012
 *      Author: ruth
 */
#include "MantidCrystal/CreateFractionalPeaks.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/IComponent.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

#include <math.h>
//#include "MantidKernel/Strings.h"
namespace Mantid
{
  using namespace Mantid::DataObjects;
  using namespace  Mantid::API;
  using namespace std;
  using namespace Mantid::Geometry;
  using namespace Mantid::Kernel;
  namespace Crystal
  {

    Kernel::Logger& CreateFractionalPeaks::g_log = Kernel::Logger::get("CreateFractionalPeaks");

    DECLARE_ALGORITHM(CreateFractionalPeaks)

    CreateFractionalPeaks::~CreateFractionalPeaks()
    {

    }



CreateFractionalPeaks::CreateFractionalPeaks():Algorithm()
    {


    }
        /// Sets documentation strings for this algorithm
    void CreateFractionalPeaks::initDocs()
    {
      this->setWikiSummary("creates a PeaksWorkspace with peaks occurring at specific fractional h,k,or l values");
      this->setOptionalMessage("The offsets can be from peaks in a range or peaks in the input PeaksWorkspace");
    }

    /// Initialise the properties
    void CreateFractionalPeaks::init()
    {
      declareProperty(new WorkspaceProperty<PeaksWorkspace> ("Peaks", "", Direction::Input),
                "Workspace of Peaks with orientation matrix that indexed the peaks and instrument loaded");

      declareProperty(new WorkspaceProperty<PeaksWorkspace> ("FracPeaks", "", Direction::Output),
                "Workspace of Peaks with peaks with fractional h,k, and/or l values");

      declareProperty("hFracDenom", 0,"Denominator of fraction in h direction");

      declareProperty("kFracDenom", 0,"Denominator of fraction in k direction");

      declareProperty("lFracDenom", 0,"Denominator of fraction in l direction");

      declareProperty("IncludeIntHKLPeaks", false,"Include the peaks with integer h,k,l values");

      declareProperty("IncludeAllPeaksInRange", false,"If false only offsets from peaks from Peaks are used");

      declareProperty(new Kernel::ArrayProperty<double>(string("HRange"),string("-8,8"),
             boost::shared_ptr<Kernel::ArrayLengthValidator<double> >
                       ( new Kernel::ArrayLengthValidator<double>(2,2))),
            "H range min,max");

      declareProperty(new Kernel::ArrayProperty<double>(string("KRange"),string("-8,8"),
          boost::shared_ptr<Kernel::ArrayLengthValidator<double> >(
                 new Kernel::ArrayLengthValidator<double>(2,2)))
          ,"K range min,max");

      declareProperty(new Kernel::ArrayProperty<double>(string("LRange"),string("-8,8"),
          boost::shared_ptr<Kernel::ArrayLengthValidator<double> >(
              new Kernel::ArrayLengthValidator<double>(2,2)))
          ,"L range min,max");

      setPropertySettings("HRange", new Kernel::EnabledWhenProperty(string("IncludeAllPeaksInRange"),
                Kernel::IS_NOT_EQUAL_TO, "0"));

      setPropertySettings("KRange", new Kernel::EnabledWhenProperty(string("IncludeAllPeaksInRange"),
                Kernel::IS_NOT_EQUAL_TO, "0"));

      setPropertySettings("LRange", new Kernel::EnabledWhenProperty(string("IncludeAllPeaksInRange"),
                Kernel::IS_NOT_EQUAL_TO,"0"));


    }

    /// Run the algorithm
    void CreateFractionalPeaks::exec()
    {
       PeaksWorkspace_sptr Peaks=getProperty("Peaks");

       int hFracDenom= getProperty("hFracDenom");
       int kFracDenom= getProperty("kFracDenom");
       int lFracDenom= getProperty("lFracDenom");

       bool includeIntPeaks= getProperty("IncludeIntHKLPeaks");
       bool includePeaksInRange= getProperty("IncludeAllPeaksInRange");

       if( includeIntPeaks && Peaks->getNumberPeaks()<=0)
       {
         g_log.error()<<"There are No peaks in the input PeaksWorkspace\n";
         return;
       }
       API::Sample samp= Peaks->sample();

       Geometry::OrientedLattice &ol = samp.getOrientedLattice();

       Geometry::Instrument_const_sptr Instr = Peaks->getInstrument();

       boost::shared_ptr<IPeaksWorkspace> OutPeaks=WorkspaceFactory::Instance().createPeaks();
       OutPeaks->setInstrument(Instr);
      // AnalysisDataService::Instance().addOrReplace(getPropertyValue("FracPeaks"),OutPeaks);
       V3D hkl;
       int peakNum =0;
       int NPeaks = Peaks->getNumberPeaks();
       Kernel::Matrix<double> Gon;
       Gon.identityMatrix();
       vector<double>v1 = getProperty("HRange");
       vector<double>v2 = getProperty("KRange");
       vector<double>v3 = getProperty("LRange");
       int N=NPeaks;
       if( includePeaksInRange)
       {
         N=(int)((v1[1]-v1[0]+1)*(v2[1]-v2[0]+1)*(v3[1]-v3[0]+1)+.5);
         N=max<int>(100,N);
       }
       IPeak& peak0 =Peaks->getPeak(0);
       int RunNumber = peak0.getRunNumber();
       Gon=peak0.getGoniometerMatrix();
       Progress prog(this, 0,  1,N);
       if( includePeaksInRange)
       {


         hkl[0]=v1[0];
         hkl[1]=v2[0];
         hkl[2]=v3[0];
       }else
       {
         hkl[0]=peak0.getH();
         hkl[1]=peak0.getK();
         hkl[2] =peak0.getL();


       }
       Kernel::DblMatrix UB= ol.getUB();

       bool done = false;;
       while( !done)
       {
         if(hkl[0] != 0 || hkl[1] !=0 || hkl[2] !=0)
         for( int hoffset=-1;hoffset<=1;hoffset++)
           for(int  koffset=-1;koffset<=1;koffset++)
             for( int loffset=-1;loffset<=1;loffset++)
               if (includeIntPeaks || hoffset!=0 || koffset!=0 || loffset!=0 )
                 if ((hFracDenom != 0 || hoffset == 0) && (kFracDenom != 0 || koffset == 0)
                    && (lFracDenom != 0 || loffset == 0))
                try
                {
                  V3D hkl1(hkl);

                  hkl1[0] += hoffset / max<double> (1.0, (double) hFracDenom);
                  hkl1[1] += koffset / max<double> (1.0, (double) kFracDenom);
                  hkl1[2] += loffset / max<double> (1.0, (double) lFracDenom);
                  //TODO check for duplicates with set of already set hkl &[hoffset,koffset,loffset values
                  Kernel::V3D Qs = UB * hkl1 ;
                  Qs*= 2.0;
                  Qs*=M_PI;
                  if( Qs[2] <=0)
                    continue;

                  //IPeak* peak = Peaks->createPeak(Qs, 1);
                  boost::shared_ptr<IPeak> peak(Peaks->createPeak(Qs, 1));
                  peak->setGoniometerMatrix(Gon);
                  peak->setQSampleFrame(Qs);
                  if (Qs[2]>0 && peak->findDetector())
                  {
                    peak->setHKL(hkl1);
                    peak->setRunNumber(RunNumber);
                    OutPeaks->addPeak(*peak);
                    //Has to be filled in with data
                    //peak.setInitialEnergy(peak0.getInitialEnergy());
                    // peak.setIntensity(peak0.getIntensity());
                    //peak.setSigmaIntensity(peak0.getSigmaIntensity());
                  }
                }catch(...)
                {
                  //setQLabFrame throws an exception if wl <0
                }
         if( includePeaksInRange)
         {
           hkl[0]++;
           if( hkl[0]>v1[1])
           {
             hkl[0]=v1[0];
             hkl[1]++;
             if( hkl[1]> v2[1])
             {

               hkl[1]=v2[0];
               hkl[2]++;
               if( hkl[2]> v3[1])
                 done = true;
             }
           }
         }else
         {
           peakNum++;
           if( peakNum >= NPeaks)
             done = true;
           else
           {// peak0= Peaks->getPeak(peakNum);
             IPeak& peak1= Peaks->getPeak(peakNum);
           //??? could not assign to peak0 above. Did not work
            // the peak that peak0 was associated with did NOT change
             hkl[0]=peak1.getH();
             hkl[1]=peak1.getK();
             hkl[2] =peak1.getL();
             Gon=peak1.getGoniometerMatrix();
             RunNumber = peak1.getRunNumber();

           }
         }
         prog.report();
       }


     setProperty("FracPeaks",OutPeaks);



    }


  }//namespace Crystal
  }//namespace Mantid
