/*WIKI*
Helper algorithm to calculate min-max input values for ConvertToMD algorithm, using ConvertToMD algorithm factory. 

Buils simplified matrix workspace, which contains min-max X values of the input workspace and the instrument, attached to the input workspace,
converts this workspace into MD using correspondent convertToMD plugin and returns the min-max values of the final transformation. 
If the min input workspace X values < 0 and max values > 0 algorithm also adds 0 X-values to the input workspace and verifies 
if min-max transformation results are achieved on at 0 values too.

For example, given input workspace in the units of energy transfer with some instrument and requesting |Q| inelastic transformation, the algorithm extracts looks through 
all spectra of the input workspace and identifies minimal, maximal and 0 energy transfer for the input spectras. Then it builds the workspace with the same detectors as the
input workspace but all spectras consisting of just 3 values namely min, 0 and max energy transfer, converts this workspace into |Q| dE, looks through all 
spectra of the transformed workspace to identify |Q|_min, |Q|_max and dE_min and dE_max and returns these values. 


*WIKI*/


#include "MantidMDAlgorithms/ConvertToMDHelper2.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidAPI/IMDNode.h"
#include "MantidMDEvents/MDWSTransform.h"
#include "MantidMDEvents/ConvToMDSelector.h"
#include "MantidMDEvents/UnitsConversionHelper.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/MultiThreaded.h"

#include <cfloat>


using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
  namespace MDAlgorithms
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(ConvertToMDHelper2)



    //----------------------------------------------------------------------------------------------
    /** Constructor
    */
    ConvertToMDHelper2::ConvertToMDHelper2()
    { }

    //----------------------------------------------------------------------------------------------
    /** Destructor
    */
    ConvertToMDHelper2::~ConvertToMDHelper2()
    {
    }


    //----------------------------------------------------------------------------------------------
    /// Algorithm's name for identification. @see Algorithm::name
    const std::string ConvertToMDHelper2::name() const { return "ConvertToMDHelper";}


    //----------------------------------------------------------------------------------------------
    /// Sets documentation strings for this algorithm

    void ConvertToMDHelper2::initDocs()
    {
      this->setWikiSummary("Calculate limits required for ConvertToMD");
      this->setOptionalMessage("Calculate limits required for ConvertToMD");
    }
    void ConvertToMDHelper2::init()
    {
      ConvertToMDParent::init();

      declareProperty(new Kernel::ArrayProperty<double>("MinValues",Direction::Output));
      declareProperty(new Kernel::ArrayProperty<double>("MaxValues",Direction::Output));

    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
    */

    void ConvertToMDHelper2::exec()
    {

      // initiate class which would deal with any dimension workspaces requested by algorithm parameters
      if(!m_HelperWSWrapper) m_HelperWSWrapper = boost::shared_ptr<MDEvents::MDEventWSWrapper>(new MDEvents::MDEventWSWrapper());

      // -------- get Input workspace
      Mantid::API::MatrixWorkspace_sptr InWS2D = getProperty("InputWorkspace");


      // Collect and Analyze the requests to the job, specified by the input parameters:
      //a) Q selector:
      std::string QModReq                    = getProperty("QDimensions");
      //b) the energy exchange mode
      std::string dEModReq                   = getProperty("dEAnalysisMode");
      //c) other dim property;
      std::vector<std::string> otherDimNames = getProperty("OtherDimensions");
      //d) The output dimensions in the Q3D mode, processed together with QConversionScales
      std::string QFrame                     = getProperty("Q3DFrames");
      //e) part of the procedure, specifying the target dimensions units. Currently only Q3D target units can be converted to different flavours of hkl
      std::string convertTo_                 = getProperty("QConversionScales");


      // Build the target ws description as function of the input & output ws and the parameters, supplied to the algorithm 
      MDEvents::MDWSDescription targWSDescr;

      // get raw pointer to Q-transformation (do not delete this pointer, it's held by MDTransfFactory!)
      MDEvents::MDTransfInterface* pQtransf =  MDEvents::MDTransfFactory::Instance().create(QModReq).get();
      // get number of dimensions this Q transformation generates from the workspace. 
      auto iEmode = Kernel::DeltaEMode().fromString(dEModReq);
      // get total numner of dimensions the workspace would have.
      unsigned int nMatrixDim = pQtransf->getNMatrixDimensions(iEmode,InWS2D);
      // total number of dimensions
      size_t nDim =nMatrixDim+otherDimNames.size();

      std::vector<double> MinValues,MaxValues;
      MinValues.resize(nDim,-FLT_MAX/10);
      MaxValues.resize(nDim,FLT_MAX/10);
      // verify that the number min/max values is equivalent to the number of dimensions defined by properties and min is less max
      targWSDescr.setMinMax(MinValues,MaxValues);   
      targWSDescr.buildFromMatrixWS(InWS2D,QModReq,dEModReq,otherDimNames);
      // add rinindex to the target workspace description for further usage as the identifier for the events, which come from this run. 
      targWSDescr.addProperty("RUN_INDEX",uint16_t(0),true);  


      // create new md workspace and set internal shared pointer of m_OutWSWrapper to this workspace
      API::IMDEventWorkspace_sptr spws = m_HelperWSWrapper->createEmptyMDWS(targWSDescr);
      if(!spws)
      {
        g_log.error()<<"can not create target event workspace with :"<<targWSDescr.nDimensions()<<" dimensions\n";
        throw(std::invalid_argument("can not create target workspace"));
      }
      // Build up the box controller
      Mantid::API::BoxController_sptr bc = m_HelperWSWrapper->pWorkspace()->getBoxController();
      // Build up the box controller, using the properties in BoxControllerSettingsAlgorithm
      //this->setBoxController(bc, m_MinMaxWS2D->getInstrument());
      //----> this all converts as follows:
      // let it be all in one box
      bc->setSplitThreshold(InWS2D->getNumberHistograms()*3+1);
      bc->setMaxDepth( 2 ); // just in case

      // Build MDGridBox
      bc->setSplitInto(2);
      bc->resetNumBoxes();

      // split boxes;
      spws->splitBox();
      spws->setMinRecursionDepth(1);  
      //<---- this all converts as follows: END


      // instanciate class, responsible for defining Mslice-type projection
      MDEvents::MDWSTransform MsliceProj;
      //identify if u,v are present among input parameters and use defaults if not
      std::vector<double> ut = getProperty("UProj");
      std::vector<double> vt = getProperty("VProj");
      std::vector<double> wt = getProperty("WProj");
      try
      {  
        // otherwise input uv are ignored -> later it can be modified to set ub matrix if no given, but this may overcomplicate things. 
        MsliceProj.setUVvectors(ut,vt,wt);   
      }
      catch(std::invalid_argument &)
      {    
        g_log.error() << "The projections are coplanar. Will use defaults [1,0,0],[0,1,0] and [0,0,1]" << std::endl;    
      }

      // set up target coordinate system and identify/set the (multi) dimension's names to use
      targWSDescr.m_RotMatrix = MsliceProj.getTransfMatrix(targWSDescr,QFrame,convertTo_);           

      //TODO: in a future it can be a choice -- use source workspace or the workspace with spherical instrument. Current settings 
      targWSDescr.m_PreprDetTable = this->preprocessDetectorsPositions(InWS2D,dEModReq,false,std::string(getProperty("PreprocDetectorsWS")));


      // build simplified workspace with min-max histogram values from input workspace and the same instument as the initial ws.true parameter 
      MinValues.assign(nDim,FLT_MAX);
      MaxValues.assign(nDim,-FLT_MAX);
      findMinMaxValues(targWSDescr,pQtransf,iEmode,true,MinValues,MaxValues);


      //DO THE JOB:
      // get pointer to appropriate  algorithm, (will throw if logic is wrong and ChildAlgorithm is not found among existing)
      MDEvents::ConvToMDSelector AlgoSelector;
      this->m_Convertor  = AlgoSelector.convSelector(m_MinMaxWS2D,this->m_Convertor);

      // initate conversion and estimate amout of job to do
      size_t n_steps = m_Convertor->initialize(targWSDescr,m_HelperWSWrapper,false);
      // progress reporter
      auto progress = std::auto_ptr<API::Progress>(new API::Progress(this,0.0,1.,n_steps)); 

      m_Convertor->runConversion(progress.get());

      /// may be  Get the minimum extents that hold the data will be sufficietn but I am not sure it works properly with convertToMD
      //virtual std::vector<Mantid::Geometry::MDDimensionExtents<coord_t> > m_OutWSWrapper->pWorkspace()->getMinimumExtents(size_t depth=2);
      size_t eventShift;
      std::string EventName = m_HelperWSWrapper->pWorkspace()->getEventTypeName();
      if(EventName=="MDEvent")
        eventShift = 4;
      else
        eventShift = 2;

      std::vector<API::IMDNode *> boxes;
      m_HelperWSWrapper->pWorkspace()->getBoxes(boxes,1000,false);
      MinValues.assign(nDim,FLT_MAX);
      MaxValues.assign(nDim,-FLT_MAX);

      std::vector<coord_t> events_table;
      for(size_t i=0;i<boxes.size();i++)
      {
        size_t nEventColumns;
        boxes[i]->getEventsData(events_table,nEventColumns);
        if (nEventColumns>0)
        {
          size_t nEvents=events_table.size()/nEventColumns;
          for(size_t evc=0;evc<nEvents;evc++)
          {
            for(size_t nd=0;nd<nDim;nd++)
            {
              coord_t ev_coord=events_table[evc*nEventColumns+eventShift+nd];
              if(ev_coord<MinValues[nd])MinValues[nd]=ev_coord;
              if(ev_coord>MaxValues[nd])MaxValues[nd]=ev_coord;
            }
          }
        }
      }


      setProperty("MinValues",MinValues);
      setProperty("MaxValues",MaxValues);

      m_HelperWSWrapper->releaseWorkspace();
      // remove service workspace from analysis data service to decrease rubbish
      API::AnalysisDataService::Instance().remove(m_MinMaxWS2D->getName());
    }

    Geometry::Object_sptr createCappedCylinder(double radius, double height, const V3D & baseCentre, const V3D & axis, const std::string & id)
    {
      std::ostringstream xml;
      xml << "<cylinder id=\"" << id << "\">" 
        << "<centre-of-bottom-base x=\"" << baseCentre.X() << "\" y=\"" << baseCentre.Y() << "\" z=\"" << baseCentre.Z() << "\"/>"
        << "<axis x=\"" << axis.X() << "\" y=\"" << axis.Y() << "\" z=\"" << axis.Z() << "\"/>"
        << "<radius val=\"" << radius << "\" />"
        << "<height val=\"" << height << "\" />"
        << "</cylinder>";

      Geometry::ShapeFactory shapeMaker;
      return shapeMaker.createShape(xml.str());
    }

    /**Create spherical instrument with specified number of detectors 
    @param nDetectors -- number of detectors to create in the instrument
    */
    Mantid::Geometry::Instrument_sptr ConvertToMDHelper2::createSphericalInstrument(size_t nDetectors)
    {
      // detectors parameters
      std::vector<double> L2(nDetectors,1);
      std::vector<double> polar(nDetectors,0);
      std::vector<double> azim(nDetectors,0);
      polar[1]=M_PI;
      for (size_t ic=2;ic<nDetectors;ic++)
      {
        polar[ic]=M_PI/2;
        azim[ic] =M_PI*double(ic-2)/2.;
      }

      return createCylInstrumentWithDetInGivenPosisions(-1,L2,polar,azim);

    }

    /**Create instrument with detectors in specified angular positions 
    @param L1    :: source-sample distance
    @param L2    :: vector of sample-detector distances
    @param ploar :: vector of polar(spherical) angles of detectors
    @param azim  :: vector of azim(spherical) angles of detectors

    @returns shared pointer to the instrument
    */
    Mantid::Geometry::Instrument_sptr 
      ConvertToMDHelper2::createCylInstrumentWithDetInGivenPosisions(const double &L1,const std::vector<double>& L2, const std::vector<double>& polar, const std::vector<double>& azim)
    {
      boost::shared_ptr<Geometry::Instrument> theInstument(new Geometry::Instrument("processed"));

      double cylRadius(0.004);
      double cylHeight(0.0002);
      // find characteristic sizes of the detectors;

      // One object
      Geometry::Object_sptr pixelShape = createCappedCylinder(cylRadius, cylHeight, V3D(0.0,-cylHeight/2.0,0.0), V3D(0.,1.0,0.), "pixel-shape");
      //Just increment pixel ID's
      int pixelID = 1;
      // one bank
      Geometry::CompAssembly *bank = new Geometry::CompAssembly("det_ass");

      for(size_t i=0;i<azim.size();i++){
        Geometry::Detector * physicalPixel = new Geometry::Detector("det"+boost::lexical_cast<std::string>(i), pixelID, pixelShape, bank);
        double zpos = L2[i]*cos(polar[i]);
        double xpos = L2[i]*sin(polar[i])*cos(azim[i]);
        double ypos = L2[i]*sin(polar[i])*sin(azim[i]);
        physicalPixel->setPos(xpos, ypos,zpos);
        pixelID++;
        bank->add(physicalPixel);
        theInstument->markAsDetector(physicalPixel);
      }
      theInstument->add(bank);
      bank->setPos(V3D(0.,0.,0.));

      //Define a source component
      Geometry::ObjComponent *source = new Geometry::ObjComponent("moderator", Geometry::Object_sptr(new Geometry::Object), theInstument.get());
      source->setPos(V3D(0.0, 0.0, L1));
      theInstument->add(source);
      theInstument->markAsSource(source);

      // Define a sample as a simple sphere
      Geometry::Object_sptr sampleCyl = createCappedCylinder(cylRadius, cylHeight, V3D(0.0,-cylHeight/2.0,0.0), V3D(0.,1.0,0.), "sample-shape");
      Geometry::ObjComponent *sample = new Geometry::ObjComponent("sample", sampleCyl, theInstument.get());
      theInstument->setPos(0.0, 0.0, 0.0);
      theInstument->add(sample);
      theInstument->markAsSamplePos(sample);

      return theInstument;
    }

    /**Build min-max workspace with the same detectors as inpitial workspace and specta which have only min-max values */
    void ConvertToMDHelper2::buildMinMaxWorkspaceWithMinInstrument(MDEvents::MDWSDescription &InOutWSDescription,
      MDEvents::MDTransfInterface const *const pQtransf,Kernel::DeltaEMode::Type iEMode,bool useWorkspace)
    {
      // for the time being. -- TODO: Enable
      UNUSED_ARG(iEMode);
      UNUSED_ARG(pQtransf);

      // Create workspace with min-max values
      double xMin,xMax;
      InOutWSDescription.getInWS()->getXMinMax(xMin,xMax);

      // we expect it also to copy all log files correspondent to ,const std::vector<std::string> &oterDimNames
      m_MinMaxWS2D=boost::dynamic_pointer_cast<DataObjects::Workspace2D>(WorkspaceFactory::Instance().create(InOutWSDescription.getInWS()));

      if(!m_MinMaxWS2D)
        throw(std::runtime_error(" Can not get Workspace 2D from the matrix workspace"));

      // This option depends on useWorkspace and is currently disabled.
      //size_t nHist = 2+4; // for spherical instrument number of histograms (detectors) in the min-max workspace -- more precise workspace would have the same number of detectors as the input one
      size_t nHist = InOutWSDescription.getInWS()->getNumberHistograms(); // number of histograms (detectors) in the min-max workspace -- more precise workspace would have the same number of detectors as the input one
      size_t nBins(3); // number of bins in min-max workspace


      MantidVecPtr X,Y,ERR;
      // source workspace units
      std::string wsUnitID    = InOutWSDescription.getInWS()->getAxis(0)->unit()->unitID();
      std::string targUnitID    = InOutWSDescription.getInWS()->getAxis(0)->unit()->unitID();


      std::vector<double> range;

      bool range_expanded=findConversionRange(InOutWSDescription,wsUnitID,xMin,xMax,range);
      if(range_expanded)
      {
        nBins = 3;
        m_MinMaxWS2D->initialize(nHist,nBins,nBins);
        X.access().resize(nBins);
        Y.access().resize(nBins,1);
        ERR.access().resize(nBins,1);

        X.access()[0]=xMin;
        X.access()[2]=xMax;
        for (size_t i=0; i< nHist; i++)
        {
          X.access()[1]=range[i];
          m_MinMaxWS2D->setX(i,X);
          m_MinMaxWS2D->setData(i,Y,ERR);
        }

      }
      else
      {
        nBins=2;
        m_MinMaxWS2D->initialize(nHist,nBins,nBins);
        X.access().resize(nBins);
        Y.access().resize(nBins,1);
        ERR.access().resize(nBins,1);
        X.access()[0]=range[0];
        X.access()[1]=range[1];

        for (size_t i=0; i< nHist; i++)
        {
          m_MinMaxWS2D->setX(i,X);
          m_MinMaxWS2D->setData(i,Y,ERR);
        }

      }


      if (!useWorkspace) 
      {
        // create spherical instrument has range of problems at the moment as detectors should be placed in RUB matrix egenvectors directions
        // this is for the future
        boost::shared_ptr<Geometry::Instrument> minMaxInstr = createSphericalInstrument(6);
        m_MinMaxWS2D->setInstrument(minMaxInstr);
      }



      // add workspace to analysis data servise for it to be availible for subalgorithms
      API::AnalysisDataService::Instance().addOrReplace("_"+InOutWSDescription.getInWS()->getName()+"_MinMaxServiceWS",m_MinMaxWS2D);
      // add this to ws description replacing initial workspace as conversion would work with this workspace pointer;
      InOutWSDescription.setWS(m_MinMaxWS2D);
    }

    /***/
    bool ConvertToMDHelper2::findConversionRange(const MDEvents::MDWSDescription &InWSDescription,const std::string & wsUnitID,
      const double &xMin,const double &xMax,std::vector<double> &range)const
    {

      MDEvents::UnitsConversionHelper unitsConverter;
      int Emode = static_cast<int>(InWSDescription.getEMode());
      long nHist =(long)InWSDescription.getInWS()->getNumberHistograms();

      bool convertTOF(false);
      if(wsUnitID!="TOF")convertTOF=true;

      bool rangeChanged(false);
      range.resize(nHist,DBL_MAX);
      unitsConverter.initialize("TOF",wsUnitID,InWSDescription.m_PreprDetTable,Emode);

      //PRAGMA_OMP(parallel for reduction(||:rangeChanged))
      for(long i=0;i<nHist;i++)
      {
        unitsConverter.updateConversion(i);
        double minTOFValue = unitsConverter.getMinTOF();
        if(convertTOF)
        {
          // different unit conversion occurs in the range from minTOFValue to max_time
          double ConvX1=unitsConverter.convertUnits(minTOFValue);
          double ConvX2=unitsConverter.convertUnits(DBL_MAX);

          if(ConvX1>xMin && ConvX1<xMax)
          {
            range[i]=ConvX1;
            rangeChanged=true;
          }
          if(ConvX2>xMin && ConvX2<xMax)
          {
            // we never expect it to happen and this can not happen for the units in use when this code was written. But if it does happen, let's warn the user as it mean rethinking the code
            if (rangeChanged)
            {
              g_log.warning()<<" conversion limits for both workspace ranges have changed. This is unexpected situation\n";
              g_log.warning()<<" Workspce limits: xMin: "<<xMin<<" xMax: "<<xMax<<"\n";
              g_log.warning()<<" Unit conversion limits limits: r1 "<<ConvX1<<" r2: "<<ConvX2<<"\n";
              range[i]=(ConvX2>range[i])?ConvX2:range[i];
              g_log.warning()<<" using conversion limi: "<<range[i]<<"\n";
            }
            else
            {
              range[i]=ConvX2;
              rangeChanged=true;
            }
          }

        }
        else
        {
          if(minTOFValue>xMin && minTOFValue<xMax)
          {
            range[i]=minTOFValue;
            rangeChanged=true;
          }
        }
      }


      return rangeChanged;
    }

  } // namespace MDAlgorithms
} // namespace Mantid
