#include "MantidQtCustomInterfaces/SANSDiagnostics.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/UserStringParser.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidAPI/SpectraDetectorMap.h"


namespace MantidQt
{
  namespace CustomInterfaces
  {
    using namespace Mantid::Kernel;
    using namespace Mantid::API;

    ///Constructor
    SANSDiagnostics::SANSDiagnostics(QWidget *parent, Ui::SANSRunWindow *ParWidgets):
    m_SANSForm(ParWidgets), parForm(parent),m_totalPeriods(0),m_rectDetectors(),m_Period(0),
      g_log(Mantid::Kernel::Logger::get("SANSDiagnostics"))
    {
      initLayout();
    }
    ///Destructor
    SANSDiagnostics::~SANSDiagnostics()
    {
    }

    ///initialise the diagonstics tab
    void SANSDiagnostics::initLayout()
    {
      //loads the last saved settings
      loadSettings();

      //disable the rectanglar detctors initially
      disableDetectorGroupBoxes(true);
      //daisable periods controls
      m_SANSForm->period_edit->setDisabled(true);
      m_SANSForm->total_perioids->setDisabled(true);
      m_SANSForm->label_period->setDisabled(true);

      //disable the check boxes for Time channel and Pixel masks
      m_SANSForm->tcmask1->setDisabled(true);
      m_SANSForm->tcmask2->setDisabled(true);
      m_SANSForm->pmask1->setDisabled(true);
      m_SANSForm->tcmask3->setDisabled(true);
      m_SANSForm->tcmask4->setDisabled(true);
      m_SANSForm->pmask2->setDisabled(true);

      connect(this, SIGNAL(runAsPythonScript(const QString&)),
        parForm, SIGNAL(runAsPythonScript(const QString&)));

      ///connect file finder plugin signal to loadfirstspectrum slot of this class
      connect(m_SANSForm->file_run_edit,SIGNAL(fileEditingFinished()),this,SLOT(loadFirstSpectrum()));
      connect(m_SANSForm->hi_Btn1,SIGNAL(clicked()),this,SLOT(firstDetectorHorizontalIntegralClicked()));
      connect(m_SANSForm->vi_Btn1,SIGNAL(clicked()),this,SLOT(firstDetectorVerticalIntegralClicked()));
      connect(m_SANSForm->ti_Btn1,SIGNAL(clicked()),this,SLOT(firstDetectorTimeIntegralClicked()));
      //2nd detector
      connect(m_SANSForm->hi_Btn2,SIGNAL(clicked()),this,SLOT(secondDetectorHorizontalIntegralClicked()));
      connect(m_SANSForm->vi_Btn2,SIGNAL(clicked()),this,SLOT(secondDetectorVerticalIntegralClicked()));
      connect(m_SANSForm->ti_Btn2,SIGNAL(clicked()),this,SLOT(secondDetectorVerticalIntegralClicked()));
      /// if period is entered display rectangual detector banks for that period
      connect(m_SANSForm->period_edit,SIGNAL(editingFinished()),this,SLOT(displayDetectorBanksofMemberWorkspace()));

    }
    /// set tool tips
    void SANSDiagnostics::setToolTips()
    {

    }

    /* This method loads the first spectrum
    * and displays periods and rectangular detectors if any.
    */ 
    void SANSDiagnostics::loadFirstSpectrum()
    {
      disableDetectorGroupBoxes(true);
      //get the file names using the filefinder plugin
      QString filename =getFileName();
      if(filename.isEmpty())
      {
        return;
      }
      //get first string from the filename  list.
      std::string fileName=filename.toStdString();
      std::replace(fileName.begin(),fileName.end(),'\\','/');
      //if teh file name same as the last entered file name don't load the file agin
      //if(!m_fileName.compare(QString::fromStdString(fileName))) return;
      m_fileName=QString::fromStdString(fileName);
      //load the first spectrum
      runLoadAlgorithm(m_fileName,"1","1");
      //total periods
      m_totalPeriods=getTotalNumberofPeriods();
      if(m_totalPeriods==1)
      {       
        displayRectangularDetectors(m_outws_load);
      }
      else
      {
        m_SANSForm->period_edit->setDisabled(false);
        m_SANSForm->total_perioids->setDisabled(false);
        m_SANSForm->label_period->setDisabled(false);
        displayTotalPeriods();
      }

    }

    ///Display total periods
    void SANSDiagnostics::displayTotalPeriods()
    {

      std::string speriods("/");
      std::string period;
      try
      {
        period=boost::lexical_cast<std::string>(m_totalPeriods);
      }
      catch(boost::bad_lexical_cast&)
      {
        g_log.error("Error when displaying the total number of periods");
      }
      speriods+=period;

      QString style="<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
        "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
        "p, li { white-space: pre-wrap; }\n"
        "</style></head><body style=\" font-family:'MS Shell Dlg 2'; font-size:8.25pt; font-weight:400; font-style:normal;\">\n"
        "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-size:12pt;\">";
      style+=QString::fromStdString(speriods);
      style+="</span></p></body></html>";
      m_SANSForm->total_perioids->setText(QApplication::translate("SANSRunWindow",style,0, QApplication::UnicodeUTF8));

    }

    /// get the period number entered in the Periods box
    int SANSDiagnostics::getPeriodNumber()
    {
      QString period=m_SANSForm->period_edit->text();
      int periodNum=0;
      try
      {
        periodNum=boost::lexical_cast<int>(period.toStdString());
      }
      catch(boost::bad_lexical_cast&)
      {
        g_log.error("Error when reading the user entered  period number");
        return 0;
      }
      return periodNum;
    }
    /**This method returns the member workspace name for the period
    *@param period - period number
    *@returns name of the member workspace.
    */
    QString SANSDiagnostics::getMemberWorkspace(int period)
    {   
      Mantid::API::Workspace_sptr ws_sptr;
      try
      {
        ws_sptr = Mantid::API::AnalysisDataService::Instance().retrieve(m_outws_load.toStdString());

      }
      catch(Exception::NotFoundError&)
      {
        return "";
      }
      if(Mantid::API::WorkspaceGroup_sptr wsgrp_sptr=boost::dynamic_pointer_cast<WorkspaceGroup>(ws_sptr))
      {
        std::vector<std::string> members= wsgrp_sptr->getNames();
        try
        {
          return QString::fromStdString(members.at(period-1));//return the member workspace.
        }
        catch(std::out_of_range&)
        {
          g_log.error("Error:The period number entered is wrong,No member workspace\
                      exists in the group workspace for the corresponding period entered.");
        }
      }
      return "";
    }
    ///returns true if the workspace contains multi period data i.e;multi period data
    bool SANSDiagnostics::isMultiPeriod()
    {
      Mantid::API::Workspace_sptr ws_sptr;
      try
      {
        ws_sptr = Mantid::API::AnalysisDataService::Instance().retrieve(m_outws_load.toStdString());

      }
      catch(Exception::NotFoundError&)
      {
        return false;
      }
      Mantid::API::WorkspaceGroup_sptr wsgrp_sptr=boost::dynamic_pointer_cast<WorkspaceGroup>(ws_sptr);
      return (wsgrp_sptr?true:false);
    }
    /// Deisplays rectangular detecctors of the selected member workspace.
    void SANSDiagnostics::displayDetectorBanksofMemberWorkspace()
    { 

      //if multi period get the user selected workspace
      int periodNum=getPeriodNumber();
      /*if(periodNum==m_Period)
      {
      return;
      }*/
      m_Period=periodNum;
      std::string sPeriods;
      try
      {
        sPeriods= boost::lexical_cast<std::string>(m_totalPeriods);
      }
      catch(boost::bad_lexical_cast&)
      {

      }
      if(periodNum>m_totalPeriods || periodNum<1)
      {
        g_log.error("Error:Period number entered is is wrong.Enter a value between 1\
                    and total number of periods "+ sPeriods );
        return;
      }

      if(m_totalPeriods>1)
      {        
        m_memberwsName= getMemberWorkspace(periodNum);
      }

      displayRectangularDetectors(m_memberwsName);
    }
    /// This method displays the rectangualr detectors
    void SANSDiagnostics::displayRectangularDetectors(const QString& wsName)
    { 

      Mantid::API::Workspace_sptr ws_sptr;
      try
      {
        ws_sptr = Mantid::API::AnalysisDataService::Instance().retrieve(wsName.toStdString());

      }
      catch(Exception::NotFoundError&)
      {
        g_log.error()<<"Workspace "+wsName.toStdString()+" not loaded"<<std::endl;
      }
      if(!ws_sptr)
      {
        return;
      }

      //get rectangular detector details.
      std::vector<boost::shared_ptr<RectDetectorDetails> > rectDetectors=rectangularDetectorDetails(ws_sptr);
      m_rectDetectors.assign(rectDetectors.begin(),rectDetectors.end());
      if(rectDetectors.empty())
      {
        g_log.error()<<"No rectangular detectors found in the instrunment associated to the file "+m_fileName.toStdString()<<std::endl;
        disableDetectorGroupBoxes(true);
        return;
      }
      //get the name of detectors
      std::string det1Name,det2Name;
      //first detector name
      det1Name=getDetectorName(0);
      if(!det1Name.empty())
      {
        //enable the detector display controls
        m_SANSForm->groupBox_Detector1->setDisabled(false);
        //set anme
        m_SANSForm->groupBox_Detector1->setTitle(QString::fromStdString(det1Name));
        m_SANSForm->groupBox_Detector1->show();
      }
      else
      {
        m_SANSForm->groupBox_Detector1->hide();
      }
      if(rectDetectors.size()<2)
      {
        m_SANSForm->groupBox_Detector2->hide();
        return;
      }
      //2nd detector
      det2Name=getDetectorName(1);
      if(!det2Name.empty())
      { 
        m_SANSForm->groupBox_Detector2->setDisabled(false);
        m_SANSForm->groupBox_Detector2->setTitle(QString::fromStdString(det2Name));
        m_SANSForm->groupBox_Detector2->show();
      }
      else
      {
        m_SANSForm->groupBox_Detector2->hide();
      }

    }

    /**This method returns the detector name from list of detctors for a given index
    *@param index of the rectangualar detector
    *@return detector name
    */
    QString SANSDiagnostics::getDetectorName(int index)
    {
      boost::shared_ptr<RectDetectorDetails> rectDet;
      try
      {
        rectDet = m_rectDetectors.at(index);
      } 
      catch(std::out_of_range&)
      {       
        g_log.error()<<"Rectangular detector not found"<<std::endl;
      } 
      if(!rectDet)
      {  
        return "";
      }
      return rectDet->getDetcetorName();
    }

    /** This method returns a vector of rectanglar detector's name, min & max detector id.
    * @param ws_sptr shared pointer to workspace
    * @returns vector of rectangular detectors details
    */
    std::vector<boost::shared_ptr<RectDetectorDetails> > SANSDiagnostics::rectangularDetectorDetails(Mantid::API::Workspace_sptr& ws_sptr)
    {      

      Mantid::API::MatrixWorkspace_sptr mws_sptr=boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws_sptr);
      if(!mws_sptr)
      {
        return std::vector<boost::shared_ptr<RectDetectorDetails> > ();
      }
      Mantid::Geometry::IInstrument_sptr inst=mws_sptr->getInstrument();
      if(!inst)
      {       
        return std::vector<boost::shared_ptr<RectDetectorDetails> > ();
      }
      std::vector<boost::shared_ptr<RectDetectorDetails> > rectDetectors;
      for (int i=0; i < inst->nelements(); i++)
      {                
        Mantid::Geometry::IComponent_sptr comp=(*inst)[i];
        boost::shared_ptr<Mantid::Geometry::RectangularDetector> det = 
          boost::dynamic_pointer_cast<Mantid::Geometry::RectangularDetector>( comp );
        if (det)
        { 
          boost::shared_ptr<RectDetectorDetails> rect(new RectDetectorDetails);
          rect->setDetcetorName(QString::fromStdString(det->getName()));
          rect->setMinimumDetcetorId(det->minDetectorID());
          rect->setMaximumDetcetorId(det->maxDetectorID());
          rectDetectors.push_back(rect);

        }
        else
        {

          boost::shared_ptr<Mantid::Geometry::ICompAssembly> assem = 
            boost::dynamic_pointer_cast<Mantid::Geometry::ICompAssembly>(comp);
          if (assem)
          {           
            for (int j=0; j < assem->nelements(); j++)
            {
              det = boost::dynamic_pointer_cast<Mantid::Geometry::RectangularDetector>( (*assem)[j] );
              if (det) 
              { 

                boost::shared_ptr<RectDetectorDetails> rect(new RectDetectorDetails);
                rect->setDetcetorName(QString::fromStdString(det->getName()));
                rect->setMinimumDetcetorId(det->minDetectorID());
                rect->setMaximumDetcetorId(det->maxDetectorID());
                rectDetectors.push_back(rect);
              }

            }
          }
        }
      }
      return rectDetectors;
    }

    /**This method returns the minimum and maximum spectrum ids
    *@param detNum - a number used to identify the detector.
    *@param minSpec - minimum spectrum number
    *@param maxSpec - maximum spectrum number
    */
    void SANSDiagnostics::minandMaxSpectrumIds(const int detNum,QString& minSpec, QString& maxSpec)
    {
      boost::shared_ptr<RectDetectorDetails> rectDet;
      try
      {
        rectDet= m_rectDetectors.at(detNum);
      }
      catch(std::out_of_range& )
      {
        g_log.error()<<"Error : No Rectangualar detector found"<<std::endl;
      }
      if(!rectDet)
      {
        return;

      }
      std::vector<int> detIdList;
      detIdList.push_back(rectDet->getMinimumDetcetorId());
      detIdList.push_back(rectDet->getMaximumDetcetorId());
      QString wsName;
      //get the spec min and max using the detector ids
      //if the loaded workpace is multi period use the workspace for user selected period
      if(isMultiPeriod())
      {
        wsName=m_memberwsName;
      }
      else
      {wsName=m_outws_load;
      }

      Mantid::API::Workspace_sptr ws_sptr;
      try
      {
        ws_sptr = Mantid::API::AnalysisDataService::Instance().retrieve(wsName.toStdString());

      }
      catch(Exception::NotFoundError&)
      {
        g_log.error()<<"Workspace "+wsName.toStdString()+" not loaded"<<std::endl;
      }
      Mantid::API::MatrixWorkspace_sptr mws_sptr=boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws_sptr);
      if(!mws_sptr)
      {
        return;
      }
      //get spectrum list
      std::vector<int>specList= mws_sptr->spectraMap().getSpectra(detIdList);
      if(specList.empty())
      {
        return;
      }
      int spec_min =*std::min_element(specList.begin(),specList.end());
      int spec_max=*std::max_element(specList.begin(),specList.end());

      std::string s_min,s_max;
      try
      {
        s_min = boost::lexical_cast<std::string >(spec_min);

      }
      catch(boost::bad_lexical_cast& )
      {
        g_log.error("Invalid Spectrum Minimum Number ");
        return;
      }
      try
      {
        s_max  = boost::lexical_cast<std::string >(spec_max);
      }
      catch(boost::bad_lexical_cast&)
      {
        g_log.error("Invalid Spectrum Maximum Number ");
        return;
      }

      minSpec=QString::fromStdString(s_min);
      maxSpec=QString::fromStdString(s_max);


    }
    //This method disables the rectangular detectors group boxes
    void SANSDiagnostics::disableDetectorGroupBoxes(bool bStatus)
    {
      //disable the rectanglar detctors initially
      m_SANSForm->groupBox_Detector1->setDisabled(bStatus);
      m_SANSForm->groupBox_Detector2->setDisabled(bStatus);
    }

    /// This method returns the list of file names entered 
    /// opened using browse in the file finder widget
    QString SANSDiagnostics::getFileName()
    {
      //get the file name using the filefinder plugin
      QString filename;
      if(m_SANSForm->file_run_edit->isValid())
      {
        filename=m_SANSForm->file_run_edit->getFirstFilename();
      }
      if(filename.isEmpty())
      {
        return "";
      }
      return filename;
    }

    /**This method executes load algorithm with filename,specmin,specmax
    *@param fileName - name of the file
    *@param specMin  - spectrum minimum
    *@param specMax  - spectrum maximum
    */
    void SANSDiagnostics::runLoadAlgorithm(const QString& fileName,const QString& specMin,const QString& specMax)
    {

      if(fileName.isEmpty()) return;
      //output workspace for load algorithm 
      m_outws_load=getWorkspaceName(fileName);
      QString load="Load('";
      load+=fileName;
      load+="','";
      load+=m_outws_load;
      load+="',";
      load+=specMin;
      load+=",";
      load+=specMax;
      load+=")";
      runPythonCode(load.trimmed());
    }

    /**This method returns workspace name from the load file name 
    *@param fileName name of the file
    *@returns workspace name
    */
    QString SANSDiagnostics::getWorkspaceName(const QString& fileName)
    {
      //construct workspace name from the file name.
      int index1=fileName.lastIndexOf(".");
      if(index1==-1)
      {        
        return "";
      }
      int index2=fileName.lastIndexOf("/");
      if(index2==-1)
      {        
        return "";
      }
      return fileName.mid(index2+1,index1-index2-1);
    }

    ///This method returns name of the   workspace which is to be
    /// used as the i/p  for sumrowcolumn or sumspectra algorithm 
    QString SANSDiagnostics::getWorkspaceToProcess()
    {
      QString wsName;
      //if the load algorithm created workspace is group workspace
      // return the workspace corresponding to  user selected workspace.
      if(isMultiPeriod())
      {
        wsName=m_memberwsName;
      }
      else
      {
        wsName=m_outws_load;
      }
      return wsName;
    }
    /**This method checks the spec min and are in valid range
    *@param specMin - minimum spectrum number
    *@param specMax - maximum spectrum number
    *@returns true if the spectra is in valid range.
    */
    bool SANSDiagnostics::isValidSpectra(const QString& specMin,const QString& specMax)
    {
      int spec_min=0;
      int spec_max=0;
      try
      {
        spec_min=boost::lexical_cast<int>(specMin.toStdString());
        spec_max=boost::lexical_cast<int>(specMax.toStdString());
      }
      catch(boost::bad_lexical_cast&)
      {

      }
      QString wsName= getWorkspaceToProcess();
      if(spec_min<1 )
      {
        g_log.error()<<"Inavlid spectrum minimum "+specMin.toStdString()+ " found\
                                                                          in the workspace  "+wsName.toStdString()<<std::endl;
      }
      if(spec_max>=Mantid::EMPTY_INT() )
      {
        g_log.error()<<"Inavlid spectrum maximum "+specMax.toStdString()+ " found\
                                                                          in the workspace  "+wsName.toStdString()<<std::endl;
      }
      return ((spec_min>=1 ||spec_max>=Mantid::EMPTY_INT())?true:false);
    }
    /// Handler for first detector horizontal integral button
    void SANSDiagnostics::firstDetectorHorizontalIntegralClicked()
    {           
      QString orientation("D_H");
      QString minSpec;
      QString maxSpec;
      int detNum=0;//first detector
      minandMaxSpectrumIds(detNum,minSpec,maxSpec);
      if(!isValidSpectra(minSpec,maxSpec))
      {
        return;
      }
      QString detName= getDetectorName(0);
      //give the detectorname_H for workspace
      detName+="_H";
      QString opws(detName);
      ///horizontal integral range string
      QString hiRange=m_SANSForm->hirange_edit1->text();
      if(hiRange.isEmpty())
      {      
        return;
      }
      IntegralClicked(hiRange,orientation,minSpec,maxSpec,opws);

    }

    /// Handler for first detector vertical integral button
    void SANSDiagnostics::firstDetectorVerticalIntegralClicked()
    {
      QString orientation("D_V");
      QString minSpec;
      QString maxSpec;
      int detNum=0;//first detector
      minandMaxSpectrumIds(detNum,minSpec,maxSpec);
      if(!isValidSpectra(minSpec,maxSpec))
      {
        return;
      }
      QString detName= getDetectorName(0);
      //give the detectorname_V for workspace
      detName+="_V";
      QString opws(detName);
      ///horizontal integral range string
      QString viRange=m_SANSForm->virange_edit1->text();
      if(viRange.isEmpty())
      {      
        return;
      }
      IntegralClicked(viRange,orientation,minSpec,maxSpec,opws);

    }
    /// Handler for first detector time integral button
    void SANSDiagnostics::firstDetectorTimeIntegralClicked()
    {
      int detNum=0;//first detector
      QString minSpec;
      QString maxSpec;
      minandMaxSpectrumIds(detNum,minSpec,maxSpec);
      if(!isValidSpectra(minSpec,maxSpec))
      {
        return;
      }
      /// now run the load algorithm with the spec_min and spec_max
      runLoadAlgorithm(m_fileName,minSpec,maxSpec);

      QString wsName= getWorkspaceToProcess();
      //execute SumSpectra 
      sumSpectraScript(wsName);
      plotSpectrum(wsName,0);
    }

    /**This method gets called from the handler of Vertical/Horizontal Integral button click.
    *executes LoadRaw and SumRowColumn algorithm.
    *@param range string entered by user
    *@param orientation orientation
    *@param specMin- minimum spectrum index
    *@param specMax - maximum spectrum index
    *@param opws - output workspace.
    */
    void SANSDiagnostics::IntegralClicked(const QString& range,const QString& orientation,
      const QString& specMin,const QString& specMax,const QString& opws)
    {
      /// now run the load algorithm with the spec_min and spec_max
      runLoadAlgorithm(m_fileName,specMin,specMax);
      int count=0;
      UserStringParser parser;
      std::vector<std::vector<unsigned int> > parsedVals;
      try
      {
        parsedVals=parser.parse(range.toStdString());
      }
      catch(std::runtime_error& e)
      {
        g_log.error(e.what());
        return;
      }
      catch(std::out_of_range& e)
      {
        g_log.error(e.what());
        return;
      }
      //now execute sumrowcolumn algorithm
      QString ipwsName;
      int periodno=0;
      if(isMultiPeriod())
      {
        ipwsName=m_memberwsName; 
      }
      else
      {
        ipwsName=m_outws_load;
      }

      QString wsPlotString="[";
      //loop through each element of the parsed value vector
      std::vector<std::vector<unsigned int> >::const_iterator parsedValcitr;
      for(parsedValcitr=parsedVals.begin();parsedValcitr!=parsedVals.end();++parsedValcitr)
      {
        ++count;
        std::stringstream num;
        num<<count;
        QString outputwsname=opws+QString::fromStdString(num.str());
        executeSumRowColumn(*parsedValcitr,ipwsName,outputwsname,orientation);

        wsPlotString+="\"";
        wsPlotString+=outputwsname;
        wsPlotString+="\"";
        wsPlotString+=",";
      }

      //remove the last comma
      int index=wsPlotString.lastIndexOf(",");
      wsPlotString.remove(index,1);
      wsPlotString+="]";

      //plot the zeroth spectrum for all the workspaces created.
      int specNum=0;
      QString plotspec="plotSpectrum(";
      plotspec+=wsPlotString;
      plotspec+=",";
      plotspec+=QString::number(specNum);
      plotspec+=")";
      runPythonCode(plotspec);

    }
    /**This method plots spectrum for the given workspace
    *@param wsName - name of the workspace
    *specNum - spectrum number
    */ 
    void SANSDiagnostics::plotSpectrum(const QString& wsName,int specNum)
    {      
      QString plotspec="plotSpectrum(\"";
      plotspec+=wsName;
      plotspec+="\",";
      plotspec+=QString::number(specNum);
      plotspec+=")";
      //g_log.error()<<"plot spectrum for time integral is "<<plotspec.toStdString()<<std::endl;
      runPythonCode(plotspec);

    }

    /// Handler for second detector horizontal integral button
    void SANSDiagnostics::secondDetectorHorizontalIntegralClicked()
    {
      QString orientation("D_H");
      QString minSpec;
      QString maxSpec;
      int detNum=1;//second detector
      minandMaxSpectrumIds(detNum,minSpec,maxSpec);
      if(!isValidSpectra(minSpec,maxSpec))
      {
        return;
      }
      QString detName= getDetectorName(detNum);
      //give the detectorname_H for workspace
      detName+="_H";
      QString opws(detName);
      ///horizontal integral range string
      QString hiRange=m_SANSForm->hirange_edit2->text();
      if(hiRange.isEmpty())
      {      
        return;
      }
      IntegralClicked(hiRange,orientation,minSpec,maxSpec,opws);

    }
    /// Handler for second detector horizontal integral button
    void SANSDiagnostics::secondDetectorVerticalIntegralClicked()
    {      
      QString orientation("D_V");
      QString minSpec;
      QString maxSpec;
      int detNum=1;//first detector
      minandMaxSpectrumIds(detNum,minSpec,maxSpec);
      if(!isValidSpectra(minSpec,maxSpec))
      {
        return;
      }
      QString detName= getDetectorName(detNum);
      //give the detectorname_H for workspace
      detName+="_H";
      QString opws(detName);

      ///horizontal integral range string
      QString viRange=m_SANSForm->virange_edit2->text();
      if(viRange.isEmpty())
      {      
        return;
      }
      IntegralClicked(viRange,orientation,minSpec,maxSpec,opws);
    }
    /// Handler for second detector horizontal integral button
    void SANSDiagnostics::secondDetectorTimeIntegralClicked()
    {
      //second detector
      int detNum=1;
      QString minSpec;
      QString maxSpec;
      minandMaxSpectrumIds(detNum,minSpec,maxSpec);
      if(!isValidSpectra(minSpec,maxSpec))
      {
        return;
      }
      /// now run the load algorithm with the spec_min and spec_max
      runLoadAlgorithm(m_fileName,minSpec,maxSpec);
      QString wsName= getWorkspaceToProcess();
      //execute SumSpectra 
      sumSpectraScript(wsName);
      plotSpectrum(wsName,0);
    }

    /// This method loads values from registry
    void SANSDiagnostics::loadSettings()
    {
      m_settingsGroup = "CustomInterfaces/SANSDiagnostics";
      QSettings settings;
      // Load settings for MWRunFile widgets
      settings.beginGroup(m_settingsGroup + "DataFiles");
      settings.value("last_directory", m_dataDir);
      m_SANSForm->file_run_edit->readSettings(settings.group());
      settings.endGroup();
    }

    /// get the total number of periods in the loaded raw/nexus file
    int SANSDiagnostics::getTotalNumberofPeriods()
    {
      Mantid::API::Workspace_sptr ws_sptr;
      try
      {
        ws_sptr = Mantid::API::AnalysisDataService::Instance().retrieve(m_outws_load.toStdString());

      }
      catch(Exception::NotFoundError&)
      {
        return 0;
      }
      if(Mantid::API::WorkspaceGroup_sptr wsgrp_sptr=boost::dynamic_pointer_cast<WorkspaceGroup>(ws_sptr))
      {
        return wsgrp_sptr->getNumberOfEntries();
      }
      return 1;
    }
    ///save settings
    void SANSDiagnostics::saveSettings()
    {
      m_dataDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("datasearch.directories"));
      m_dataDir = m_dataDir.split(";", QString::SkipEmptyParts)[0];
      QSettings settings;
      // Load settings for MWRunFile widgets
      settings.beginGroup(m_settingsGroup + "DataFiles");
      settings.setValue("last_directory",m_dataDir);

      m_SANSForm->file_run_edit->saveSettings(settings.group());
      settings.endGroup();

    }

    /**Execute sumrowcolumn algorithm 
    *@param values a vector containing the values for hvmin,hvmax
    *@param periodNum - number of the  period to load
    *@param ipws - name of the i/p workspace
    *@param opws - name of the o/p workspace
    *@param orientation - orientation of the detector
    */
    void SANSDiagnostics::executeSumRowColumn(const std::vector<unsigned int>& values,
      const QString ipws,const QString& opws,const QString& orientation)
    {      
      if(values.empty())
      {
        return ;
      }

      QString HVMin;
      QString HVMax; 
      //check the vector contains sequential vales.
      if(!isSequentialValues(values))
      {
        g_log.error("Values between HVMin and HVMax in the Range string  are not sequential ");
        return;
      }
      try
      {
        //first value in the vector 
        HVMin=QString::fromStdString(boost::lexical_cast<std::string>(values.at(0)));
        ///last value is HVMax
        if(values.size()>1)
        {
          HVMax=QString::fromStdString(boost::lexical_cast<std::string>(values.at(values.size()-1)));
        }
        else
        {
          //if the vector contains only one value HVMax=HVMin
          HVMax=HVMin;
        }
      }
      catch(boost::bad_lexical_cast& )
      {
      }
      catch(std::out_of_range& )
      {
      }
      //now execute sumrowcolumn with hvmin and havmax from the first and last vales from the vector
      QString code=sumRowColumnScript(ipws,opws,orientation,HVMin,HVMax);
      runPythonCode(code.trimmed());
    }

    /**Execute sumrowcolumn algorithm with the vales
    *@param ipwsName name of the input workspace
    *@param opwsName name of the output workspace
    *@param orientation indicates row or columns to sum
    *@param hvMin 
    *@param hvMax
    */
    QString SANSDiagnostics::sumRowColumnScript(const QString ipwsName,const QString& opwsName,
      const QString& orientation,const QString& hvMin,const QString& hvMax)
    {
      QString code="SumRowColumn('";
      code+=ipwsName;
      code+="', '";
      code+=opwsName;
      code+="', '";
      code+=orientation;
      code+="', ";
      code+="HVMin=";
      code+=hvMin;
      code+=",";
      code+="HVMax=";
      code+=hvMax;
      code+=")";
      return code;
    }
    /**This method creates script string for sumspectra algorithm
    *@param opwsName - name of the ooutput workspace
    *@returns - sumspectra script string
    */
    QString SANSDiagnostics::sumSpectraScript(const QString& opwsName)
    {
      QString ipwsName= getWorkspaceToProcess();

      QString code="SumSpectra(";
      code+=ipwsName;
      code+="\",\"";
      code+=ipwsName;
      code+="\"";
      return code;
    }

    /**Checks the values in the given vector is sequential
    *@param values  vector containing numbers
    *@returns true if the vector is sequential
    */
    bool SANSDiagnostics::isSequentialValues(const std::vector<unsigned int>& values)
    {
      try
      {
        unsigned int startVal=values.at(0);
        std::vector<unsigned int>::const_iterator citr;
        for(int i=1;i<values.size();++i)
        {
          if(values.at(i)==startVal+1)
          {
            startVal=values.at(i);
          }
          else
          {
            return false;
          }

        }

      }
      catch(std::out_of_range&)
      {
      }
      return true;
    }

  }
}