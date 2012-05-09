/*
 * SCDPanelErrors.cpp
 *
 *  Created on: Feb 27, 2012
 *      Author: ruth
 */
#include "MantidCurveFitting/SCDPanelErrors.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Instrument/Parameter.h"
#include <boost/lexical_cast.hpp>
#include <stdio.h>
#include <math.h>

using namespace Mantid::API;
using namespace std;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::Kernel::Units;

//TODO: add const's and & to arguments.  PeaksWorkspace_ptr should be const_ptr

namespace Mantid
{
  namespace CurveFitting
  {

    DECLARE_FUNCTION(SCDPanelErrors)

    // Assumes UB from optimize UB maps hkl to qxyz/2PI. So conversion factors to an from
    // UB ified q's are below.
     Kernel::Logger& SCDPanelErrors::g_log = Kernel::Logger::get("SCDPanelErrors");


    static const double UBq2Q(2 * M_PI);
    static const double Q2UBq = 1 / UBq2Q;


    void CheckSizetMax( size_t v1, size_t v2, size_t v3,std::string ErrMess)
    {
      if( v1 > 32000 ||v2 > 32000 ||v3 > 32000 )
        throw std::invalid_argument("Size_t val very large "+ErrMess);
    }


    SCDPanelErrors::SCDPanelErrors() :
      API::ParamFunction(), IFunction1D()
    {
      NLatticeParametersSet = 0;
      a_set = b_set = c_set = alpha_set = beta_set = gamma_set = PeakName_set = BankNames_set = endX_set
          = startX_set = NGroups_set = false;//_WIN32

     // g_log.setLevel(7);

      tolerance = .6;
      startX = endX = -1;

      boost::shared_ptr<PeaksWorkspace> pwks1(new PeaksWorkspace());

      peaks = pwks1;

      B0 = DblMatrix(3, 3);

      BankNames = "";

      PeakName = "";

      a = b = c = alpha = beta = gamma = 0;

      NGroups =1;

    }




    SCDPanelErrors::~SCDPanelErrors()
    {

    }




    SCDPanelErrors::SCDPanelErrors(DataObjects::PeaksWorkspace_sptr &pwk, std::string& Component_name,
        double ax, double bx, double cx, double alphax, double betax, double gammax, double tolerance1) :
      API::ParamFunction(), IFunction1D()
    {

      peaks = pwk;
      BankNames = Component_name;

      tolerance = tolerance1;
      NGroups = 1;
      NLatticeParametersSet = 0;
      NLatticeParametersSet = 0;
      a_set = b_set = c_set = alpha_set = beta_set = gamma_set = PeakName_set = BankNames_set = endX_set
          = startX_set = NGroups_set = false;

      setAttribute("a", Attribute(ax));
      setAttribute("b", Attribute(bx));
      setAttribute("c", Attribute(cx));
      setAttribute("alpha", Attribute(alphax));
      setAttribute("beta", Attribute(betax));
      setAttribute("gamma", Attribute(gammax));

      setAttribute("PeakWorkspaceName", Attribute("xxx"));

      setAttribute("BankNames", Attribute(Component_name));

      setAttribute("startX", Attribute(-1));
      setAttribute("endX", Attribute(-1));
      init();
#if defined(_WIN32) && !defined(_WIN64)
      g_log.setLevel(7);
#endif
    // g_log.setLevel(7);

    }



    void SCDPanelErrors::init()
    {

      declareParameter("f0_detWidthScale", 1.0, "panel Width");
      declareParameter("f0_detHeightScale", 1.0, "panel Height");

      declareParameter("f0_Xoffset", 0.0, "Panel Center x offset");
      declareParameter("f0_Yoffset", 0.0, "Panel Center y offset");
      declareParameter("f0_Zoffset", 0.0, "Panel Center z offset");

      declareParameter("f0_Xrot", 0.0, "Rotation(degrees) Panel Center in x axis direction");
      declareParameter("f0_Yrot", 0.0, "Rotation(degrees) Panel Center in y axis direction");
      declareParameter("f0_Zrot", 0.0, "Rotation(degrees) Panel Center in z axis direction");

      declareParameter("l0", 0.0, "Initial Flight Path");
      declareParameter("t0", 0.0, "Time offset");

    }

    /**
     * If the workspace is a Peaks workspace, all peaks in the peaks will be fit. Make sure
     * that ALL the peaks are in the desired banks and that ALL the peaks have hkl values
     * are within the desired distance from an integer.
     */
    /*  void SCDPanelErrors::setWorkspace(boost::shared_ptr<const Workspace> ws, bool copyData)
     {

     boost::shared_ptr<const PeaksWorkspace> pks1= boost::dynamic_pointer_cast<const PeaksWorkspace>(ws);

     if( !pks1)
     {
     IFunctionMW::setWorkspace( ws, copyData);
     return;
     }

     boost::shared_ptr<PeaksWorkspace>pks2(pks1->clone());
     boost::shared_ptr<PeaksWorkspace>pwks;
     pwks= pks2;
     peaks= pks2;
     set<string> bankNames;
     for(int i=0; i< (int) pwks->rowCount();i++)
     {
     IPeak & pk =pwks->getPeak(i);
     string namee = pk.getBankName();
     bankNames.insert( namee );
     }

     set<string>::iterator it;
     vector<string> Bnames;
     for( it = bankNames.begin(); it !=bankNames.end(); it++)
     {
     Bnames.push_back( (*it));
     }

     DataObjects::Workspace2D_sptr  wspc= calcWorkspace( pwks,Bnames,.6);
     IFunctionMW::setWorkspace( wspc , false);

     }

     */



    boost::shared_ptr<DataObjects::PeaksWorkspace> SCDPanelErrors::getPeaks() const
    {
      boost::shared_ptr<DataObjects::PeaksWorkspace> pwks;
      pwks = peaks;
      if (pwks->rowCount() < 1)
      {
        pwks = AnalysisDataService::Instance().retrieveWS<PeaksWorkspace> (PeakName);

        if (!pwks || pwks->rowCount() < 1)
        {
          g_log.error("There are no peaks in the peaks workspace or no PeakWorkspace");
          throw std::invalid_argument("There are no peaks in the peaks workspace or no PeakWorkspace");
        }
        return pwks;
      }
      else
        return peaks;
    }


    void SCDPanelErrors::Check(PeaksWorkspace_sptr & pkwsp, const double *xValues, const size_t nData) const
    {
      if (NLatticeParametersSet < (int) nAttributes())
      {
        g_log.error("Not all lattice parameters have been set");
        throw std::invalid_argument("Not all lattice parameters have been set");

      }

      if (!pkwsp)
      {
        g_log.error("Cannot find a PeaksWorkspace ");
        throw std::invalid_argument("Cannot find a PeaksWorkspace ");
      }

      if (pkwsp->getNumberPeaks() < 4)
      {
        g_log.error("Not enough peaks to fit ");
        throw std::invalid_argument("Not enough peaks to fit ");
      }

      if ((startX >  (int)nData - 1) || (endX > (int) nData - 1))
      {
        g_log.error("startX and endX attributes are out of range");
        throw std::invalid_argument("startX and endX attributes are out of range");
      }

      size_t StartX ;

      if( startX < 0)
        StartX = (size_t)0;
      else
        StartX= (size_t)startX;

      size_t EndX = endX;

      if (endX < 0)
        EndX =  nData - 1;

      if (xValues[StartX] != floor(xValues[StartX]) )
      {
        g_log.error("Improper workspace set xVals must be integer");
        throw invalid_argument("Improper workspace. xVals must be integer");
      }

      if (xValues[StartX] < 0 || xValues[StartX] >= pkwsp->rowCount())
      {

        g_log.error("Improper workspace set");
        throw invalid_argument("Improper workspace. xVals correspond to an index in the PeaksWorkspace");
      }

      if( (EndX-StartX+1)/3 < 4)
      {
        g_log.error("Not enough peaks to process banks "+ BankNames);
        throw invalid_argument("Not enough peaks to process banks "+ BankNames);
      }

    }


   void SCDPanelErrors::updateBankParams( boost::shared_ptr<const Geometry::IComponent>  bank_const,
                boost::shared_ptr<Geometry::ParameterMap> pmap,
                boost::shared_ptr<const Geometry::ParameterMap>pmapSv)const
   {

       vector<V3D> posv= pmapSv->getV3D( bank_const->getName(),"pos");
       if (posv.size() > 0)
       {
        V3D pos = posv[0];
        pmap->addDouble(bank_const.get(), "x", pos.X());
        pmap->addDouble(bank_const.get(), "y", pos.Y());
        pmap->addDouble(bank_const.get(), "z", pos.Z());
        pmap->addV3D(bank_const.get(), "pos", pos);
       }

       boost::shared_ptr<Parameter> rot = pmapSv->get(bank_const.get(),("rot"));
       if( rot)
       {
         pmap->addQuat(bank_const.get(), "rot",rot->value<Quat>());
       }

       vector<double> scalex = pmapSv->getDouble(bank_const->getName(),"scalex");
       vector<double> scaley = pmapSv->getDouble(bank_const->getName(),"scaley");
       if( scalex.size() > 0)
          pmap->addDouble(bank_const.get(),"scalex", scalex[0]);
       if( scaley.size() > 0)
          pmap->addDouble(bank_const.get(),"scaley", scaley[0]);

       boost::shared_ptr<const Geometry::IComponent> parent = bank_const->getParent();
       if( parent)
         updateBankParams( parent, pmap, pmapSv);

   }

   void SCDPanelErrors::updateSourceParams(boost::shared_ptr<const Geometry::IObjComponent> bank_const,
       boost::shared_ptr<Geometry::ParameterMap> pmap, boost::shared_ptr<const Geometry::ParameterMap> pmapSv) const
    {
      vector<V3D> posv = pmapSv->getV3D(bank_const->getName(), "pos");
      if (posv.size() > 0)
      {
        V3D pos = posv[0];
        pmap->addDouble(bank_const.get(), "x", pos.X());
        pmap->addDouble(bank_const.get(), "y", pos.Y());
        pmap->addDouble(bank_const.get(), "z", pos.Z());
        pmap->addV3D(bank_const.get(), "pos", pos);
      }

      boost::shared_ptr<Parameter> rot = pmapSv->get(bank_const.get(), "rot");
      if (rot)
        pmap->addQuat(bank_const.get(), "rot", rot->value<Quat>());
    }


    Instrument_sptr SCDPanelErrors::getNewInstrument(const DataObjects::Peak & peak) const
    {

      Geometry::Instrument_const_sptr instSave = peak.getInstrument();
      boost::shared_ptr<Geometry::ParameterMap> pmap(new ParameterMap());
      boost::shared_ptr<const Geometry::ParameterMap> pmapSv = instSave->getParameterMap();

      if (!instSave)
      {
        g_log.error(" Not all peaks have an instrument");
        throw invalid_argument(" Not all peaks have an instrument");
      }
      boost::shared_ptr<Geometry::Instrument> instChange(new Geometry::Instrument());

      bool isParameterized = false;


      if (!instSave->isParametrized())
      {

        boost::shared_ptr<Geometry::Instrument> instClone(instSave->clone());
        boost::shared_ptr<Geometry::Instrument> Pinsta(new Geometry::Instrument(instSave, pmap));

        instChange = Pinsta;
        isParameterized = true;

      }
      else //catch(...)
      {


        isParameterized = true;

        boost::shared_ptr<const IComponent> inst3 = boost::dynamic_pointer_cast<const IComponent>(
            instSave);
      // updateParams(pmapSv, pmap, inst3);

        boost::shared_ptr<Geometry::Instrument> P1(new Geometry::Instrument(instSave->baseInstrument(),
            pmap));
        instChange = P1;

      }

      if (!instChange)
      {
        g_log.error("Cannot 'clone' instrument");
        throw logic_error("Cannot clone instrument");
      }
      std::vector<std::string> GroupBanks;

      boost::split(GroupBanks, BankNames, boost::is_any_of("!"));

      for( size_t group=0; group < (size_t)GroupBanks.size(); ++group)
      {
        string prefix="f"+boost::lexical_cast<std::string>(group)+"_";

        std::vector<std::string> bankNames;

        boost::split(bankNames, GroupBanks[group], boost::is_any_of("/"));
      //Set new settings in the instrument
        vector<string>::iterator it;

      for (it = bankNames.begin(); it != bankNames.end(); ++it)
      {
        string bankNm = *it;

        //---------------------Now get Panel-----------------------------

        boost::shared_ptr<const Geometry::IComponent> bank_const =
            instChange->getComponentByName(bankNm);
        updateBankParams( bank_const, pmap, pmapSv);
        if (!bank_const)
        {
          g_log.error() << "No component named " << bankNm << std::endl;
          throw invalid_argument("No component named " + bankNm);
        }

        boost::shared_ptr<const Geometry::Component> bank = boost::dynamic_pointer_cast<
            const Geometry::Component>(bank_const);

        if (!bank)
        {
          g_log.error() << "No non const component named " << bankNm << std::endl;
          throw invalid_argument("No nonconst component named " + bankNm);
        }

        //------------------------ Rotate Panel------------------------

        Quat Rot0 = bank_const->getRelativeRot();

        Quat Rot = Quat(getParameter(prefix+"Xrot"), Kernel::V3D(1.0, 0.0, 0.0)) * Rot0;

        Rot = Quat(getParameter(prefix+"Yrot"), Kernel::V3D(0.0, 1.0, 0.0)) * Rot;

        Rot = Quat(getParameter(prefix+"Zrot"), Kernel::V3D(0.0, 0.0, 1.0)) * Rot;

        std::string name = bankNm;

        pmap->addQuat(bank.get(), "rot", Rot);

        //-------------------------Move Panel ------------------------


        V3D pos1 = bank->getRelativePos();

        pmap->addPositionCoordinate(bank.get(), string("x"), getParameter(prefix+"Xoffset") + pos1.X());
        pmap->addPositionCoordinate(bank.get(), string("y"), getParameter(prefix+"Yoffset") + pos1.Y());
        pmap->addPositionCoordinate(bank.get(), string("z"), getParameter(prefix+"Zoffset") + pos1.Z());

        //--------------------------- Scale Panel( only for Rectangle Detectors)-------------------------------------

        boost::shared_ptr<const Geometry::RectangularDetector> Rect = boost::dynamic_pointer_cast<
            const Geometry::RectangularDetector>(bank);

        Kernel::V3D scale = bank->getScaleFactor();
        if (scale != Kernel::V3D(1.0, 1.0, 1.0))
          pmap->addV3D(bank.get(), "sca", scale);

        if (Rect)
        {
          double scalex = getParameter(prefix+"detWidthScale");
          double scaley = getParameter(prefix+"detHeightScale");
          double scalez = 1;
          string baseName = Rect->base()->getName();
          if (pmapSv->getDouble(baseName, string("scalex")).size() > 0)
            scalex *= pmapSv->getDouble(baseName, string("scalex"))[0];

          if (pmapSv->getDouble(baseName, string("scaley")).size() > 0)
            scaley *= pmapSv->getDouble(baseName, string("scaley"))[0];

          if (pmapSv->getDouble(baseName, string("scalez")).size() > 0)
            scalez *= pmapSv->getDouble(baseName, string("scalez"))[0];

          V3D RectScaleFactor = Rect->getScaleFactor();

          if (RectScaleFactor.X() == 0 || RectScaleFactor.Y() == 0 || RectScaleFactor.Z() == 0)
            RectScaleFactor = V3D(1, 1, 1);

          V3D scale(scalex, scaley, 1.0);

          pmap->addDouble(bank.get(), "scalex", scalex * RectScaleFactor.X());
          pmap->addDouble(bank.get(), "scaley", scaley * RectScaleFactor.Y());
          pmap->addDouble(bank.get(), "scalez", scalez * RectScaleFactor.Z());
          pmap->addV3D(bank.get(), "scale", scale * RectScaleFactor);

        }//if Rect
      }//For each bank name
      }//for each group
      //-------------------- Move source(L0)-------------------------------
      boost::shared_ptr<const Geometry::IObjComponent> source = instChange->getSource();
      boost::shared_ptr<const Geometry::IObjComponent> sample = instChange->getSample();
      updateSourceParams( source, pmap, pmapSv);
      updateSourceParams( sample, pmap, pmapSv);

      //For L0 change the source position
      Kernel::V3D sourcePos1 = source->getPos();
      Kernel::V3D ParentPos = sourcePos1 - source->getRelativePos();
      Kernel::V3D samplePos = sample->getPos();
      Kernel::V3D direction = (sourcePos1 - samplePos);
      sourcePos1 = samplePos + direction * getParameter("l0") / direction.norm();
      Kernel::V3D sourceRelPos1 = sourcePos1 - ParentPos;

      pmap->addPositionCoordinate(source.get(), string("x"), sourceRelPos1.X());
      pmap->addPositionCoordinate(source.get(), string("y"), sourceRelPos1.Y());
      pmap->addPositionCoordinate(source.get(), string("z"), sourceRelPos1.Z());

      return instChange;
    }




    Peak SCDPanelErrors::createNewPeak(const Peak & peak_old, Geometry::Instrument_sptr  instrNew) const
    {
      Geometry::Instrument_const_sptr inst = peak_old.getInstrument();
      if (inst->getComponentID() != instrNew->getComponentID())
      {
        g_log.error("All peaks must have the same instrument");
        throw invalid_argument("All peaks must have the same instrument");
      }

      double T0 = peak_old.getTOF() + getParameter("t0");

      int ID = peak_old.getDetectorID();

      Kernel::V3D hkl = peak_old.getHKL();
      //peak_old.setDetectorID(ID); //set det positions
      Peak peak(instrNew, ID, peak_old.getWavelength(), hkl, peak_old.getGoniometerMatrix());

      Wavelength wl;

      wl.initialize(getParameter("l0"), peak.getL2(), peak.getScattering(), 0,
          peak_old.getInitialEnergy(), 0.0);

      peak.setWavelength(wl.singleFromTOF(T0));
      peak.setIntensity(peak_old.getIntensity());
      peak.setSigmaIntensity(peak_old.getSigmaIntensity());
      peak.setRunNumber(peak_old.getRunNumber());
      peak.setBinCount(peak_old.getBinCount());

      peak.setDetectorID(ID);
      return peak;
    }




    void SCDPanelErrors::function1D(double *out, const double *xValues, const size_t nData) const
    {
      g_log.debug()<<"Start function 1D"<<endl;
      size_t StartX;
      size_t EndX;
      V3D panelCenter_old;
      V3D panelCenterNew;
      if (startX < 0 || endX < 0)
      {
        StartX = 0;
        EndX =  nData - 1;
      }else
      {
        StartX = (size_t) startX;
        EndX =(size_t) endX;
        if( EndX >=nData  || EndX < StartX )
           EndX = nData -1;
      }


      if (nData <= (size_t) 0)
        return;

      double r = checkForNonsenseParameters();

      if( r != 0 )
      {
         for( size_t i=0;i < nData; ++i )
             out[i]=100 + r;

         g_log.debug() << "Parametersxx  for " << BankNames << ">=";
          for( size_t i=0; i < nParams(); ++i)
              g_log.debug() << getParameter(i) << ",";

          g_log.debug() << endl;

          return;
        }

      boost::shared_ptr<DataObjects::PeaksWorkspace> pwks = getPeaks();

      Check(pwks, xValues, nData);

      g_log.debug() << "BankNames " << BankNames << "   Number of peaks" << (EndX - StartX + 1) / 3
          << std::endl;

      boost::shared_ptr<Geometry::Instrument> instChange = getNewInstrument(pwks->getPeak(0));

      //---------------------------- Calculate q and hkl vectors-----------------
      Kernel::Matrix<double> UB(3, 3, false);

      vector<Kernel::V3D> hkl_vectors;
      vector<Kernel::V3D> q_vectors;
      CheckSizetMax(StartX,EndX,EndX,"f(x) main loop");
      for (size_t i = StartX; i <= EndX; i += 3)
      {
        double xIndx = (xValues[i]);
        if (xIndx != floor(xIndx) || xIndx < 0)
        {
          g_log.error()<<"Improper workspace set xVals must be positive integers "
          <<xIndx<<","<<floor(xIndx)<<std::endl;
          throw invalid_argument("Improper workspace. xVals must be positive integers");
        }

        size_t pkIndex = (size_t) xIndx;
        if ( pkIndex >=  pwks->rowCount()) // ||pkIndex < 0
        {

          g_log.error()<<"Improper workspace set "<<pkIndex<<","<<xIndx<<std::endl;
          throw invalid_argument(
              "Improper workspace. xVals correspond to an index in the PeaksWorkspace");
        }

        IPeak & peak_old = pwks->getPeak((int) pkIndex);
        V3D detOld;

        Peak peak = createNewPeak(peak_old, instChange);

        Kernel::V3D hkl = peak_old.getHKL();
        double hkl1[3] =
        { hkl.X(), hkl.Y(), hkl.Z() };

        bool ok = true;
        for (size_t k = 0; k < 3 && ok; ++k) //eliminate tolerance cause only those peaks that are
        // OK should be here
        {
          double off = hkl1[k] - floor(hkl1[k]);

          if (off < tolerance)

            hkl1[k] = floor(hkl1[k]);

          else if (1 - off < tolerance)

            hkl1[k] = floor(hkl1[k]) + 1;

          else

            ok = false;
        }

        if (ok && (hkl.X() != 0 || hkl.Y() != 0 || hkl.Z() != 0))
        {

          hkl_vectors.push_back(Kernel::V3D(hkl1[0], hkl1[1], hkl1[2]));
          q_vectors.push_back(peak.getQSampleFrame());

        }

      }

      //----------------------------------Calculate out ----------------------------------
      bool badParams=false;
      Kernel::DblMatrix UB0;

      try
      {

        Geometry::IndexingUtils::Optimize_UB(UB, hkl_vectors, q_vectors);

        Geometry::OrientedLattice lat;

        lat.setUB(UB);

        const Kernel::DblMatrix U = lat.getU();

        UB0 = U * B0;
      } catch (std::exception & )
      {

        badParams = true;
      } catch (char * )
      {

        badParams = true;
      } catch (...)
      {
        badParams = true;
      }

      if( badParams)
      { CheckSizetMax(StartX,EndX,nData,"deriv xyz final");
        for(size_t i = StartX; i <= EndX; ++i)
           out[i]= 10000;
        g_log.debug()<<"Could Not find a UB matix"<<std::endl;
        return;
      }




      double chiSq = 0;// for debug log message

      for (size_t i = 0; i < q_vectors.size(); ++i)
      {
        Kernel::V3D err = q_vectors[i] - UB0 * hkl_vectors[i] * UBq2Q;

        out[3 * i+StartX] = err[0];
        out[3 * i + 1+StartX] = err[1];
        out[3 * i + 2+StartX] = err[2];
        chiSq += err[0]*err[0] + err[1]*err[1] + err[2]*err[2] ;
       // if( i < 4)
       //   g_log.debug()<<"hkl,q="<<hkl_vectors[i]<<q_vectors[i]<<endl;
        CheckSizetMax(i,i,i,"f(x) loop 2");
      }

      for (size_t i = 3 * q_vectors.size(); i < nData; ++i)
        out[i] = 0;

      g_log.debug() << "Parameters" << std::endl;

      for (size_t i = 0; i < this->nParams(); ++i)
        g_log.debug() << setw(20) << parameterName(i) << setw(20) << getParameter(i) << std::endl;

      g_log.debug() << "      chi Squared=" <<std::setprecision(12)<< chiSq << std::endl;

      //Get values for test program. TODO eliminate
      g_log.debug() << "  out[evenxx]=";
     for (size_t i = 0; i < min<size_t> (nData, 30); ++i)
       g_log.debug() << out[i] << "  ";

   //   g_log.debug() << std::endl;

    }




    Matrix<double> SCDPanelErrors::CalcDiffDerivFromdQ(Matrix<double> const& DerivQ,
                                     Matrix<double> const& Mhkl, Matrix<double> const& MhklT,
                                     Matrix<double> const& InvhklThkl, Matrix<double> const& UB) const
    {
      try
      {
        Matrix<double> dUB = DerivQ * Mhkl * InvhklThkl * Q2UBq;

        Geometry::OrientedLattice lat;
        lat.setUB(Matrix<double> (UB) + dUB * .001);
        const Kernel::DblMatrix U2 = lat.getU();

        Kernel::DblMatrix U2A(U2);
        lat.setUB(Matrix<double> (UB) - dUB * .001);
        const Kernel::DblMatrix U1 = lat.getU();

        Kernel::DblMatrix dU = (U2A - U1) * (1 / .002);
        if( dU == Kernel::DblMatrix())
          std::cout<<"zero dU in CalcDiffDerivFromdQ"<<std::endl;
        Kernel::DblMatrix dUB0 = dU * B0;

        Kernel::DblMatrix dQtheor = dUB0 * MhklT;
        Kernel::DblMatrix Deriv = Matrix<double> (DerivQ) - dQtheor * UBq2Q;

        return Deriv;

      } catch (...)
      {

        for (size_t i = 0; i < nParams(); ++i)
          g_log.debug() << getParameter(i) << ",";

        g_log.debug() << endl;

        throw std::invalid_argument(" Invalid initial data ");
      }
    }

    double SCDPanelErrors::checkForNonsenseParameters()const
    {

      double Dwdth = getParameter(0);
      double Dhght = getParameter(1);
      double x = getParameter(2);
      double y = getParameter(3);
      double z = getParameter(4);
      double rx = getParameter(5);
      double ry = getParameter(6);
      double rz = getParameter(7);
      double L0= getParameter(8);
      double T0 = getParameter(9);

       double r =0;
       if( L0 <1 )
        r = 1-L0;

      if( fabs(T0) > 20)
        r += (T0-20)*2;

      if( Dwdth < .5 || Dwdth > 2)
        r += 3*fabs(1-Dwdth);

      if( Dhght <.5 || Dhght >2)
        r+=3*fabs(1-Dhght);

      if( fabs(x) >.35 )
        r+= fabs(x)*.2;

      if( fabs(y) >.35 )
        r+= fabs(y)*.2;

      if( fabs(z) >.35)
        r+= fabs(z)*.2;

      if( fabs(rx) >15 )
        r+= fabs(rx)*.02;

      if( fabs(ry) >15 )
        r+= fabs(ry)*.02;

      if( fabs(rz) >15 )
        r+= fabs(ry)*.02;

      return 5*r;


    }

    void SCDPanelErrors::functionDeriv1D(Jacobian *out, const double *xValues, const size_t nData)
    {
     size_t StartPos=2;
     size_t StartRot = 5;

      size_t L0param = parameterIndex("l0");
      size_t T0param = parameterIndex("t0");;
      CheckSizetMax(StartPos, L0param,T0param,"Start deriv");
      if (nData <= 0)
        return;
      if (NLatticeParametersSet < (int) nAttributes())
      {
        g_log.error("Not all lattice parameters have been set");
        throw std::invalid_argument("Not all lattice parameters have been set");
      }


      size_t StartX ;
      size_t EndX ;
      if (startX < 0 || endX < 0 || endX < startX)
      {
        StartX = 0;
        EndX =  nData - 1;
      }else
      {
        StartX =(size_t)startX;
        EndX = (size_t) endX;
        if( EndX >= nData || EndX < StartX)
          EndX = nData -1;
      }

      double rr =checkForNonsenseParameters();

      if( rr > 0 )
      {
        for( size_t i = 0; i< nParams(); ++i )
          for(size_t k = 0; k<nData;  ++k )
            out->set( k, i, 10 + rr );

        return;
      }

      CheckSizetMax(StartX,EndX,EndX,"Deriv calc StartX,EndX");
      peaks = getPeaks();
      Check(peaks, xValues, nData);

      Instrument_sptr instrNew = getNewInstrument(peaks->getPeak(0));

      boost::shared_ptr<ParameterMap> pmap = instrNew->getParameterMap();
      vector<int> row, col, peakIndx, NPanelrows, NPanelcols;
      vector<V3D> pos, xvec, yvec, hkl, qlab, qXtal;
      vector<V3D> PanelCenter;
      vector<double> time;
      double K, L0;

      string lastBankName = "";
      V3D last_xvec, last_yvec, last_Center;
      int last_Nrows, last_Ncols;

      V3D SamplePos = instrNew->getSample()->getPos();
      V3D SourcePos = instrNew->getSource()->getPos();
      IPeak & ppeak = peaks->getPeak(0);
      L0 = ppeak.getL1();

      double velocity = (L0 + ppeak.getL2()) / ppeak.getTOF();
      K = 2 * M_PI / ppeak.getWavelength() / velocity;//2pi/lambda = K*velocity

      for (size_t xval = StartX; xval <= EndX; xval += 3)
      {
        double x = floor(xValues[xval]);
        IPeak& peak_old = peaks->getPeak((int) x);

        Peak peak = createNewPeak(peak_old, instrNew);

        peakIndx.push_back((int) x);
        qlab.push_back(peak.getQLabFrame());
        qXtal.push_back(peak.getQSampleFrame());
        row.push_back(peak.getRow());
        col.push_back(peak.getCol());
        time.push_back(peak.getTOF());

        V3D HKL = peak.getHKL();
        hkl.push_back(V3D(floor(.5 + HKL.X()), floor(.5 + HKL.Y()), floor(.5 + HKL.Z())));

        pos.push_back(peak.getDetPos());

        string thisBankName = peak.getBankName();

        if (thisBankName == lastBankName)
        {
          xvec.push_back(last_xvec);
          yvec.push_back(last_yvec);
          PanelCenter.push_back(last_Center);
          NPanelrows.push_back(last_Nrows);
          NPanelcols.push_back(last_Ncols);

        }
        else
        {
          V3D x_vec(1, 0, 0);
          V3D y_vec(0, 1, 0);
          boost::shared_ptr<const IComponent> panel = instrNew->getComponentByName(thisBankName);
          Quat Rot = panel->getRotation();
          Rot.rotate(x_vec);
          Rot.rotate(y_vec);

          boost::shared_ptr<const RectangularDetector> rPanel = boost::dynamic_pointer_cast<
              const RectangularDetector>(panel);
          x_vec *= rPanel->xstep();
          y_vec *= rPanel->ystep();
          int Nrows = rPanel->ypixels();

          int Ncols = rPanel->xpixels();

          NPanelrows.push_back(Nrows);
          NPanelcols.push_back(Ncols);

          last_Nrows = Nrows;
          last_Ncols = Ncols;

          PanelCenter.push_back(rPanel->getAtXY(0, 0)->getPos());

          xvec.push_back(x_vec);
          yvec.push_back(y_vec);
          last_xvec = x_vec;
          last_yvec = y_vec;
          lastBankName = thisBankName;
        }
      }

      Matrix<double> Mhkl(hkl.size(), 3), InvhklThkl(3, 3);
      for (size_t rw = 0; rw <  hkl.size(); ++rw)
        for (size_t cl = 0; cl < 3; ++cl)
          Mhkl[rw][cl] = hkl[rw][cl];

      Matrix<double> MhklT(Mhkl);
      MhklT.Transpose();

      InvhklThkl = MhklT * Mhkl;

      InvhklThkl.Invert();


      Matrix<double> UB(3, 3);
      try
      {
        Geometry::IndexingUtils::Optimize_UB(UB, hkl, qXtal);
        g_log.debug()<<"UB=\n";
        for( int ii=0; ii <3; ++ii)
        {
          for( int jj=0; jj<3; ++jj)
            g_log.debug()<<UB[ii][jj]<<",";
          g_log.debug()<<std::endl;
        }
      } catch (std::exception & s)
      {

        g_log.error("Not enough points to find Optimized UB1 =" + std::string(s.what()));
        throw std::runtime_error("Not enough good points to find Optimized UB");
      } catch (char * s1)
      {
        g_log.error("Not enough points to find Optimized UB2=" + std::string(s1));
        throw std::runtime_error("Not enough good points to find Optimized UB");
      } catch (...)
      {
        g_log.error("Not enough points to find Optimized UB3");
        throw std::runtime_error("Not enough good points to find Optimized UB");
      }



      map<string, size_t> bankName2Group;
      vector<string> Groups;
      boost::split( Groups, BankNames, boost::is_any_of("!"));
      for( size_t gr=0; gr< Groups.size(); ++gr)
      {


        vector<string> banknames;
        boost::split(banknames, Groups[gr],boost::is_any_of("/"));
        for( vector<string>::iterator it=banknames.begin(); it != banknames.end(); ++it)
          bankName2Group[(*it)]=gr;
      }


      vector<V3D> Unrot_dQ[3];
      int pick[3];
      pick[0] = pick[1] = pick[2] = 0;
      Matrix<double> Result(3, qlab.size());
      for( size_t gr=0; gr< (size_t)NGroups; ++gr)
      {
        Unrot_dQ[0].clear();
        Unrot_dQ[1].clear();
        Unrot_dQ[2].clear();

      //-------- xyz offset parameters ----------------------
       StartPos = parameterIndex("f"+boost::lexical_cast<string>(gr)+"_Xoffset");
    //  int startPeak = -1;
       for (size_t param = StartPos; param <=StartPos+(size_t)2; ++param)

      {
        pick[param - StartPos] = 1;
        V3D parxyz(pick[0], pick[1], pick[2]);
        pick[param - StartPos] = 0;
        if( param== StartPos+1 && gr==0)
          g_log.debug()<<"pick=["<<pick[0]<<","<<pick[1]<<","<<pick[2]<<","<<std::endl;
        Matrix<double> Result(3, qlab.size());
        CheckSizetMax(gr,param,param,"xyzoffset1 Deriv");
        for (size_t peak = 0; peak <  qlab.size(); ++peak)
        if( bankName2Group[ peaks->getPeak(peakIndx[peak]).getBankName()]!=gr)
        {   Unrot_dQ[param - StartPos].push_back(V3D(0.0,0.0,0.0));//Save for later calculations


        Result[0][peak] = 0;
        Result[1][peak] = 0;
        Result[2][peak] = 0;

        }else {
         // if( startPeak < 0)
         //   startPeak = peak;
          double L1 = pos[peak].norm();
          double velMag = (L0 + L1) / time[peak];
          double t1 = time[peak] - L0 / velMag;


          double dt1 = parxyz.scalar_prod(pos[peak]) * L0 / velMag / velMag / time[peak] / L1;

          V3D dQlab;
          V3D vel = pos[peak] / L1 * velMag;


          if( param== StartPos+1 && gr==0&& peak==0)
            g_log.debug()<<"DerivCalc1="<<L1<<","<<velMag<<","<<t1
              << dt1<<","<<vel<<std::endl;
          double r = (K / t1 * dt1);
          dQlab.setX(vel.scalar_prod(V3D(1, 0, 0)) * r);
          dQlab.setY(vel.scalar_prod(V3D(0, 1, 0)) * r);
          dQlab.setZ(vel.scalar_prod(V3D(0, 0, 1)) * r);


          r = -K / t1;
          dQlab.setX(dQlab.X() + parxyz.scalar_prod(V3D(1, 0, 0) * r));
          dQlab.setY(dQlab.Y() + parxyz.scalar_prod(V3D(0, 1, 0) * r));
          dQlab.setZ(dQlab.Z() + parxyz.scalar_prod(V3D(0, 0, 1) * r));

          double dvMag = parxyz.scalar_prod(pos[peak]) / time[peak] / L1;

          dQlab.setZ(dQlab.Z() + K * dvMag);

          if( param== StartPos+1 && gr==0&& peak==0)
                      g_log.debug()<<"ereRot="<<dQlab<<std::endl;
          Matrix<double> GonMatrix = peaks->getPeak(peakIndx[peak]).getGoniometerMatrix();
          GonMatrix.Invert();
          V3D dQsamp = GonMatrix * dQlab;


          Unrot_dQ[param - StartPos].push_back(dQlab);//Save for later calculations


          Result[0][peak] = dQsamp.X();
          Result[1][peak] = dQsamp.Y();
          Result[2][peak] = dQsamp.Z();
          if( gr==0 && peak <2)
          g_log.debug()<<"Deriv to xyzoffsets wrt "<<param <<dQsamp<<std::endl;
        }



        Kernel::DblMatrix Deriv = CalcDiffDerivFromdQ(Result, Mhkl, MhklT, InvhklThkl, UB);

       // for (size_t w = 0; w < nData; w++)
      //    out->set(w, param, 0.0);

        size_t x = StartX;
        for (size_t coll = 0; coll <  Deriv.numCols(); ++coll)
          for (size_t roww = 0; roww < 3; ++roww)
          {
            CheckSizetMax(coll,roww,x,"deriv xyz final");
            out->set(x, param, Deriv[roww][coll]);
            x++;
          }
      }


      //-------------------- Derivatives with respect to rotx,roty, and rotz ---------
      StartRot = parameterIndex("f"+boost::lexical_cast<string>(gr)+"_Xrot");

      for (size_t param = StartRot; param <= StartRot+2; ++param)
      {
        Matrix<double> Result(3, qlab.size());
        Matrix<double> Rot2dRot(3, 3); //deriv of rot matrix at angle=0
        Rot2dRot.zeroMatrix();
        int r1 = (int)param - (int)StartRot;
        int r = (r1 + 1) % 3;

        Rot2dRot[r][(r + 1) % 3] = -1;
        r = (r + 1) % 3;
        Rot2dRot[r][(r + 2) % 3] = +1;
        Rot2dRot *= M_PI / 180.;

        for (size_t peak = 0; peak <  qlab.size(); ++peak)
           if (bankName2Group[peaks->getPeak(peakIndx[peak]).getBankName()] != gr)
            {
              Result[0][peak] = 0;
              Result[1][peak] = 0;
              Result[2][peak] = 0;

            }else{
              CheckSizetMax(gr,param,param,"Deriv rot A");
          int Nwrt = 3;
          int NderOf = 3;
          Matrix<double> Bas(NderOf, Nwrt); //partial Qxyz wrt xyx
          Bas.zeroMatrix();

          for (int rr = 0; rr < NderOf; ++rr)
            for (int cc = 0; cc < Nwrt; ++cc)
            {
              Bas[rr][cc] = Unrot_dQ[cc][peak][rr];

            }

          V3D dXvec = Rot2dRot * xvec[peak];
          V3D dYvec = Rot2dRot * yvec[peak];
          V3D dxyz2theta = dXvec * (col[peak] - NPanelcols[peak] / 2.0 + .5) + dYvec * (row[peak]
              - NPanelrows[peak] / 2.0 + .5);

          //dxyz2theta is partials xyz wrt rot x
          V3D unRotDeriv = Bas * dxyz2theta;

          Matrix<double> GonMatrix = peaks->getPeak(peakIndx[peak]).getGoniometerMatrix();
          GonMatrix.Invert();

          V3D RotDeriv = GonMatrix * unRotDeriv;

          for (int kk = 0; kk < 3; ++kk)
            Result[kk][peak] = RotDeriv[kk];
        }

        Kernel::DblMatrix Deriv = CalcDiffDerivFromdQ(Result, Mhkl, MhklT, InvhklThkl, UB);

        //for (size_t w = 0; w < nData; w++)
       //   out->set(w, param, 0.0);


        size_t x = StartX;
        for (size_t coll = 0; coll <  Deriv.numCols(); ++coll)
          for (int roww = 0; roww < 3; ++roww)
          { CheckSizetMax(coll,roww,x,"deriv rot final");
            out->set(x, param, Deriv[roww][coll]);
            x++;
          }
      }

      size_t param =  parameterIndex("f"+boost::lexical_cast<string>(gr)+"_detWidthScale");



      for (size_t peak = 0; peak <  qlab.size(); ++peak)
        if( bankName2Group[ peaks->getPeak(peakIndx[peak]).getBankName()]!=gr)
        {
          Result[0][peak] = 0;
          Result[1][peak] = 0;
          Result[2][peak] = 0;

        }
        else{
          CheckSizetMax(gr,peak,peak,"deriv detw A");
        int Nwrt = 3;
        int NderOf = 3;
        Matrix<double> Bas(NderOf, Nwrt);
        Bas.zeroMatrix();
        for (int rr = 0; rr < NderOf; ++rr)
          for (int cc = 0; cc < Nwrt; ++cc)
            Bas[rr][cc] = Unrot_dQ[cc][peak][rr];

        V3D Xvec = xvec[peak] * (col[peak] - NPanelcols[peak] / 2);//partial xyz wrt widthScale

        V3D unRotDeriv = Bas * Xvec;
        Matrix<double> GonMatrix = peaks->getPeak(peakIndx[peak]).getGoniometerMatrix();
        GonMatrix.Invert();

        V3D RotDeriv = GonMatrix * unRotDeriv;

        for (int kk = 0; kk < 3; ++kk)
          Result[kk][peak] = RotDeriv[kk];

      }

      Kernel::DblMatrix Deriv = CalcDiffDerivFromdQ(Result, Mhkl, MhklT, InvhklThkl, UB);
     // for (size_t w = 0; w < nData; w++)
     //   out->set(w, param, 0.0);

      size_t x = StartX;
      for (size_t coll = 0; coll <  Deriv.numCols(); ++coll)
        for (int roww = 0; roww < 3; ++roww)
        { CheckSizetMax(coll,roww,x,"deriv scalew final");
          out->set(x, param, Deriv[roww][coll]);
          x++;
        }

      param =  parameterIndex("f"+boost::lexical_cast<string>(gr)+"_detHeightScale");

     // param = StartLen+1;//scale Height
      Result.zeroMatrix();
      for (size_t peak = 0; peak <  qlab.size(); ++peak)
        if( bankName2Group[ peaks->getPeak(peakIndx[peak]).getBankName()]!=gr)
        {
          Result[0][peak] = 0;
          Result[1][peak] = 0;
          Result[2][peak] = 0;

         }
        else{
         CheckSizetMax(gr,peak,peak,"deriv detH A");
        int Nwrt = 3;
        int NderOf = 3;
        Matrix<double> Bas(NderOf, Nwrt);
        Bas.zeroMatrix();

        for (int rr = 0; rr < NderOf; ++rr)
          for (int cc = 0; cc < Nwrt; ++cc)
            Bas[rr][cc] = Unrot_dQ[cc][peak][rr];

        V3D Yvec = yvec[peak] * (row[peak] - NPanelrows[peak] / 2);//partial xyz wrt heightScale

        V3D unRotDeriv = Bas * Yvec;

        Matrix<double> GonMatrix = peaks->getPeak(peakIndx[peak]).getGoniometerMatrix();
        GonMatrix.Invert();

        V3D RotDeriv = GonMatrix * unRotDeriv;

        for (int kk = 0; kk < 3; ++kk)
          Result[kk][peak] = RotDeriv[kk];

      }

      Deriv = CalcDiffDerivFromdQ(Result, Mhkl, MhklT, InvhklThkl, UB);
      for (size_t w = 0; w < nData; ++w)
        out->set(w, param, 0.0);

      x = StartX;
      for (size_t coll = 0; coll <  Deriv.numCols(); ++coll)
        for (int roww = 0; roww < 3; ++roww)
        { CheckSizetMax(coll,roww,x,"deriv scaleH final");
          out->set(x, param, Deriv[roww][coll]);
          x++;
        }
  /*  int TestParam = (int)parameterIndex("t0");//"f"+boost::lexical_cast<string>(gr)+"_detHeightScale");
      std::vector<double>out1(nData);
      std::vector<double>out2(nData);
      function1D( out1.data(),xValues, nData);
      double v = getParameter( TestParam);
      setParameter( TestParam, v+.001);
      function1D( out2.data(),xValues, nData);
      setParameter(TestParam, v);
      std::cout<<"off for Xrot of group "<<gr<<"=";
      for( int i=0; i< nData; i++)
      {
        double D =( out2[i]-out1[i])/.001;
        std::cout<<"("<<D<<","<<out->get(i,TestParam) <<")";
        if(fabs(D-out->get(i,TestParam))>.02)
          std::cout<<"*";
      }
*/



    }//for each group

      size_t param = L0param;//L0.  partial unRotQxyz wrt L0 = unRotQxyz/|v|/tof

      Result.zeroMatrix();
      for (size_t peak = 0; peak <  qlab.size(); ++peak)
      {

        double L1 = pos[peak].norm();
        double velMag = (L0 + L1) / time[peak];
        double KK = 1 / velMag / time[peak];

        V3D unRotDeriv = qlab[peak] * KK;

        Matrix<double> GonMatrix = peaks->getPeak(peakIndx[peak]).getGoniometerMatrix();
        GonMatrix.Invert();
        V3D RotDeriv = GonMatrix * unRotDeriv;

        for (int kk = 0; kk < 3; ++kk)
          Result[kk][peak] = RotDeriv[kk];

      }

      Kernel::DblMatrix Deriv = CalcDiffDerivFromdQ(Result, Mhkl, MhklT, InvhklThkl, UB);
      //for (size_t w = 0; w < nData; w++)
      //   out->set(w, param, 0.0);

      size_t x = StartX;
      for (size_t coll = 0; coll <  Deriv.numCols(); ++coll)
        for (int roww = 0; roww < 3; ++roww)
        { CheckSizetMax(coll,roww,x,"deriv L0 final");
          out->set(x, param, Deriv[roww][coll]);
          x++;
        }

      param = T0param;//t0 partial unRotQxyz wrt t0 = -unRotQxyz/tof
      Result.zeroMatrix();
      for (size_t peak = 0; peak <  qlab.size(); ++peak)
      {
        double KK = -1 / time[peak];
        V3D unRotDeriv = qlab[peak] * KK;

        Matrix<double> GonMatrix = peaks->getPeak(peakIndx[peak]).getGoniometerMatrix();
        GonMatrix.Invert();
        V3D RotDeriv = GonMatrix * unRotDeriv;

        for (int kk = 0; kk < 3; ++kk)
          Result[kk][peak] = RotDeriv[kk];

      }

      Deriv = CalcDiffDerivFromdQ(Result, Mhkl, MhklT, InvhklThkl, UB);
      // for (size_t w = 0; w < nData; w++)
      //   out->set(w, param, 0.0);
      x = StartX;
      for (size_t coll = 0; coll <  Deriv.numCols(); ++coll)
        for (int roww = 0; roww < 3; ++roww)
        { CheckSizetMax(coll,roww,x,"deriv t0 final");
          out->set(x, param, Deriv[roww][coll]);
          x++;
        }

   /*      int TestParam = L0param;
         std::vector<double>out1(nData);
           std::vector<double>out2(nData);
           function1D( out1.data(),xValues, nData);
           double v = getParameter( TestParam);
           setParameter( TestParam ,v+.001);
           function1D( out2.data(),xValues, nData);
           setParameter(TestParam, v);
          // std::cout<<"off for Xrot of group "<<gr<<"=";
           for( int i=0; i< nData; i++)
           {
             double D =( out2[i]-out1[i])/.001;
             std::cout<<"("<<D<<","<<out->get(i,TestParam) <<")";
             if(fabs(D-out->get(i,TestParam))>.02)
               std::cout<<"*";
           }
           */
    }




    DataObjects::Workspace2D_sptr SCDPanelErrors::calcWorkspace(DataObjects::PeaksWorkspace_sptr & pwks,
        std::vector<std::string>& bankNames, double tolerance)
    {
      int N = 0;
      Mantid::MantidVecPtr pX;
      if (tolerance < 0)
        tolerance = .5;
      tolerance = min<double> (.5, tolerance);

      Mantid::MantidVec& xRef = pX.access();
      Mantid::MantidVecPtr yvals;
      Mantid::MantidVec &yvalB = yvals.access();

      for (size_t k = 0; k < bankNames.size(); ++k)
        for (size_t j = 0; j < pwks->rowCount(); ++j)
        {
          API::IPeak& peak = pwks->getPeak( (int)j);
          if (peak.getBankName().compare(bankNames[k]) == 0)
            if (peak.getH() != 0 || peak.getK() != 0 || peak.getK() != 0)
              if (peak.getH() - floor(peak.getH()) < tolerance || floor(peak.getH() + 1) - peak.getH()
                  < tolerance)
                if (peak.getK() - floor(peak.getK()) < tolerance || floor(peak.getK() + 1) - peak.getK()
                    < tolerance)
                  if (peak.getL() - floor(peak.getL()) < tolerance || floor(peak.getL() + 1)
                      - peak.getL() < tolerance)
                  {
                    N++;
                    xRef.push_back((double) j);
                    xRef.push_back((double) j);
                    xRef.push_back((double) j);
                    yvalB.push_back(0.0);
                    yvalB.push_back(0.0);
                    yvalB.push_back(0.0);

                  }
        }

      MatrixWorkspace_sptr mwkspc = API::WorkspaceFactory::Instance().create("Workspace2D", (size_t) 3,
          3 * N, 3 * N);

      mwkspc->setX(0, pX);
      mwkspc->setX(1, pX);
      mwkspc->setX(2, pX);
      mwkspc->setData(0, yvals);
      mwkspc->setData(0, yvals);
      mwkspc->setData(0, yvals);

      return boost::dynamic_pointer_cast<DataObjects::Workspace2D>(mwkspc);
    }

      void SCDPanelErrors::setAttribute(const std::string &attName, const Attribute & value)
      {
        if (!hasAttribute(attName))
          throw std::invalid_argument("Not a valid attribute namee " + attName);

        double x = 0;
        if(!(attName == "PeakWorkspaceName" || attName=="BankNames" || attName =="startX"
                            || attName == "endX" || attName == "NGroups"))
           x =value.asDouble();

        if (attName.compare("beta") < 0)
        {
          if (attName.compare("a") == 0)
          {
            a = x;
            if (!a_set)
            {
              a_set = true;
              NLatticeParametersSet++;
            }
          }
          else if (attName == ("b"))
          {
            b = x;
            if (!b_set)
            {
              b_set = true;
              NLatticeParametersSet++;
            }
          }
          else if (attName == ("BankNames"))
          {
            BankNames = value.asString();
            if (!BankNames_set)
            {
              BankNames_set = true;
              NLatticeParametersSet++;
            }
          }
          else if (attName == ("alpha"))
          {
            alpha = x;
            if (!alpha_set)
            {
              alpha_set = true;
              NLatticeParametersSet++;
            }
          }
          else if( attName =="PeakWorkspaceName")
          {
           PeakName= value.asString();

           if( !PeakName_set)
           {
             PeakName_set = true;
             NLatticeParametersSet++;
           }
          }else if( attName == "NGroups")
          {
            if( NGroups_set )
            {
              g_log.error("Cannot set NGroups more than once");
              throw new std::invalid_argument("Cannot set NGroups more than once");
            }
            NGroups = value.asInt();
            for (int k = 1; k < NGroups; ++k)
            {
              std::string prefix = "f"+boost::lexical_cast< std::string >(k) + "_";
              declareParameter(prefix + "detWidthScale", 1.0, "panel Width");
              declareParameter(prefix + "detHeightScale", 1.0, "panelHeight");

              declareParameter(prefix + "Xoffset", 0.0, "Panel Center x offset");
              declareParameter(prefix + "Yoffset", 0.0, "Panel Center y offset");
              declareParameter(prefix + "Zoffset", 0.0, "Panel Center z offset");

              declareParameter(prefix + "Xrot", 0.0, "Rotation(degrees) Panel Center in x axis direction");
              declareParameter(prefix + "Yrot", 0.0, "Rotation(degrees) Panel Center in y axis direction");
              declareParameter(prefix + "Zrot", 0.0, "Rotation(degrees) Panel Center in z axis direction");
            }
            if( !NGroups_set)
            {
              NGroups_set = true;
              NLatticeParametersSet++;
            }
          }
          else
            throw std::invalid_argument("Not a valid attribute namef ");

        }
        else if (attName == "beta")
        {
          beta = x;
          if (!beta_set)
          {
            beta_set = true;
            NLatticeParametersSet++;
          }
        }
        else if (attName == "c")
        {
          c = x;
          if (!c_set)
          {
            c_set = true;
            NLatticeParametersSet++;
          }
        }
        else if (attName == "startX")
        {
          startX =(size_t)value.asInt();
          if (!startX_set)
          {
            startX_set = true;
            NLatticeParametersSet++;
          }

        }
        else if (attName == "endX")
        {
          endX = (size_t)value.asInt();
          if (!endX_set)
          {
            endX_set = true;
            NLatticeParametersSet++;
          }

        }
        else if (attName == "gamma")
        {
          gamma = x;
          if (!gamma_set)
          {
            gamma_set = true;
            NLatticeParametersSet++;
          }
        }else
          throw std::invalid_argument("Not a valid attribute namea "+attName);


        if (NLatticeParametersSet >= (int)nAttributes())
        {

          Geometry::UnitCell lat(a, b, c, alpha, beta, gamma);
          B0 = lat.getB();
        }
      }

  }
}
