#include "MantidQtCustomInterfaces/SANSDiagnostics.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/UserStringParser.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidGeometry/IDetector.h"

using Mantid::specid_t;
using Mantid::detid_t;


namespace MantidQt
{
  namespace CustomInterfaces
  {
    using namespace Mantid::Kernel;
    using namespace Mantid::API;

    ///Constructor
    SANSDiagnostics::SANSDiagnostics(QWidget *parent, Ui::SANSRunWindow *ParWidgets):
    m_SANSForm(ParWidgets), parForm(parent),m_totalPeriods(0),m_Period(0),m_rectDetectors(),
      g_log(Mantid::Kernel::Logger::get("SANSDiagnostics"))
    {
      initLayout();
      //connect to SANSRunWindow to apply mask
      connect(this,SIGNAL(applyMask(const QString&,bool)),parent,SLOT(applyMask(const QString&,bool)));
    }
    ///Destructor
    SANSDiagnostics::~SANSDiagnostics()
    {
      saveSettings();
    }

    ///initialise the diagonstics tab
    void SANSDiagnostics::initLayout()
    {
      //loads the last saved settings
      loadSettings();
      setToolTips();
      //disable the rectanglar detctors initially
      disableDetectorGroupBoxes(true);
      //daisable periods controls
      changePeriodsControls(true);
    
      //disable time region contorls
      m_SANSForm->region_det1->setDisabled(true);
      m_SANSForm->region_det2->setDisabled(true);
      m_SANSForm->tirange_edit1->setDisabled(true);
      m_SANSForm->tirange_edit2->setDisabled(true);

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
      connect(m_SANSForm->ti_Btn2,SIGNAL(clicked()),this,SLOT(secondDetectorTimeIntegralClicked()));
      /// if period is entered display rectangual detector banks for that period
      connect(m_SANSForm->period_edit,SIGNAL(editingFinished()),this,SLOT(displayDetectorBanksofMemberWorkspace()));
      
      
    }
    /// set tool tips
    void SANSDiagnostics::setToolTips()
    {
      m_SANSForm->label_period->setToolTip("Period number of the member workspace to process  if the loaded file contains multi period data");
      m_SANSForm->hrange_det1->setToolTip("H/V_Min and H/V_Max values for SumRowColumn algorithm");
      m_SANSForm->vrange_det1->setToolTip("H/V_Min and H/V_Max values for SumRowColumn algorithm");

      m_SANSForm->hrange_det2->setToolTip("H/V_Min and H/V_Max values for SumRowColumn algorithm");
      m_SANSForm->vrange_det2->setToolTip("H/V_Min and H/V_Max values for SumRowColumn algorithm");

      m_SANSForm->hi_Btn1->setToolTip("Executes SANS specific SumRowColumn algorithm and displays the H Plot for the first detectro bank ");
      m_SANSForm->vi_Btn1->setToolTip("Executes SANS specific SumRowColumn algorithm and displays the V Plot for the first detectro bank");
      m_SANSForm->hi_Btn2->setToolTip("Executes SANS specific SumRowColumn algorithm and displays the H Plot for the second detectro bank ");
      m_SANSForm->vi_Btn2->setToolTip("Executes SANS specific SumRowColumn algorithm and displays the V Plot for the second detectro bank ");

      m_SANSForm->ti_Btn1->setToolTip("Executes SumSpectra algorithm and displays the Plot for the first detectro bank ");
      m_SANSForm->ti_Btn2->setToolTip("Executes SumSpectra algorithm and displays the Plot for the second detectro bank");
      m_SANSForm->total_perioids->setToolTip("Total number of periods");

    }

  /** This method loads the first spectrum
    * and displays periods and rectangular detectors if any.
    */ 
    void SANSDiagnostics::loadFirstSpectrum()
    {
      
      //get the file names using the filefinder plugin
      QString filename =getFileName();
      if(filename.isEmpty())
      {
        return;
      }
      //get first string from the filename  list.
      std::string fileName=filename.toStdString();
      std::replace(fileName.begin(),fileName.end(),'\\','/');
      //if the file name same as the last entered file name don't run the load algorithm again 
      if(!m_fileName.compare(QString::fromStdString(fileName))) return;
      m_fileName=QString::fromStdString(fileName);
      //load the first spectrum
      if(!runLoadAlgorithm(m_fileName,"1","1"))
      {
        return;
      }
      //total periods
      m_totalPeriods=getTotalNumberofPeriods();
      if(m_totalPeriods<=1)
      {
       // disble Periods Controls;
        changePeriodsControls(true);
      }
      if(m_totalPeriods==1)
      {        
        displayRectangularDetectors(m_outws_load);
      }
      else if(m_totalPeriods>1)
      {
        // enable Periods Controls;
        changePeriodsControls(false);
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
      std::string key=style.toStdString();
      m_SANSForm->total_perioids->setText(QApplication::translate("SANSRunWindow",key.c_str(),0, QApplication::UnicodeUTF8));

    }
    /// This method disables the total periods controls
    void SANSDiagnostics::changePeriodsControls(bool bEnable)
    {
       //daisable periods controls
      m_SANSForm->period_edit->setDisabled(bEnable);
      m_SANSForm->total_perioids->setDisabled(bEnable);
      m_SANSForm->label_period->setDisabled(bEnable);
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
    *  @param period - period number
    *  @returns name of the member workspace.
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
        g_log.error()<<"Error when accessing the Workspace "+m_outws_load.toStdString()<<std::endl;
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
          g_log.error("The period number entered is wrong.");
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
        g_log.error("Error:Period number entered is is wrong.Enter a value between 1 and total number of periods "+ sPeriods );
        return;
      }
      //this check is doing bcoz the editfinished signal seems to be emitted more than once .
      if(periodNum==m_Period)
      {
      return;
      }
      m_Period=periodNum;
      if(m_totalPeriods>1)
      {        
        m_memberwsName= getMemberWorkspace(periodNum);
      }

      displayRectangularDetectors(m_memberwsName);
    }
    /// This method displays the rectangular detectors
    void SANSDiagnostics::displayRectangularDetectors(const QString& wsName)
    { 

      Mantid::API::Workspace_sptr ws_sptr;
      try
      {
        ws_sptr = Mantid::API::AnalysisDataService::Instance().retrieve(wsName.toStdString());

      }
      catch(Exception::NotFoundError&)
      {
        g_log.error()<<"Error when accessing the Workspace "+wsName.toStdString()<<std::endl;
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
        g_log.error()<<"The instrument associated to the file "+m_fileName.toStdString()<<" does not have any RectangularDetectors "<<std::endl;
        disableDetectorGroupBoxes(true);
        return;
      }
      //get the name of detectors
      QString det1Name,det2Name;
      //first detector name
      det1Name=getDetectorName(0);
      if(!det1Name.isEmpty())
      {
        //enable the detector display controls
        m_SANSForm->groupBox_Detector1->setDisabled(false);
        //set anme
        m_SANSForm->groupBox_Detector1->setTitle(det1Name);
        m_SANSForm->groupBox_Detector1->show();
      }
      else
      {
        m_SANSForm->groupBox_Detector1->setDisabled(true);
      }
     
      //2nd detector
      det2Name=getDetectorName(1);
      if(!det2Name.isEmpty())
      { 
        m_SANSForm->groupBox_Detector2->setDisabled(false);
        m_SANSForm->groupBox_Detector2->setTitle(det2Name);
        m_SANSForm->groupBox_Detector2->show();
      }
      else
      {
        m_SANSForm->groupBox_Detector2->setDisabled(true);
        
      }

    }

  /** This method returns the detector name from list of detctors for a given index
    * @param index of the rectangular detector
    * @return detector name
    */
    const QString SANSDiagnostics::getDetectorName(int index)
    {
      boost::shared_ptr<RectDetectorDetails> rectDet;
      try
      {
        rectDet = m_rectDetectors.at(index);
      } 
      catch(std::out_of_range&)
      {     
      } 
      if(rectDet)
      {  
        return rectDet->getDetcetorName();
      }
      return "";
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

  /** This method returns spectrum list for the selected detector
    * @param mws_sptr shared pointer to workspace
    * @param detNum number used to identify detector
    * @param specList  -list of spectrum
    */
    void SANSDiagnostics::getSpectraList(const Mantid::API::MatrixWorkspace_sptr& mws_sptr,const detid_t detNum,std::vector<specid_t>&specList)
    {
      boost::shared_ptr<RectDetectorDetails> rectDet;
      try
      {
        rectDet= m_rectDetectors.at(detNum);
      }
      catch(std::out_of_range& )
      {
        if(detNum==0)
        {
          g_log.error()<<"Error : The instrument does not have any RectangularDetectors "<<std::endl;
        }
        else if(detNum==1)
        {
          g_log.error()<<"Error : The instrument  have only one  RectangularDetector"<<std::endl;
        }
        return;
      }
      if(!rectDet)
      {
        g_log.error()<<"Error when accessing the details of rectangular detector"<<std::endl;
        return;
      }
      std::vector<detid_t> detIdList;
      detIdList.push_back(rectDet->getMinimumDetcetorId());
      detIdList.push_back(rectDet->getMaximumDetcetorId());
      specList= mws_sptr->spectraMap().getSpectra(detIdList);
     
    }
  /** This method returns the minimum and maximum spectrum ids
    * @param specList - list of spectra.
    * @param minSpec - minimum spectrum number
    * @param maxSpec - maximum spectrum number
    */
    void SANSDiagnostics::minandMaxSpectrumIds(const std::vector<specid_t>& specList,QString& minSpec, QString& maxSpec)
    {      
      specid_t spec_min =*std::min_element(specList.begin(),specList.end());
      specid_t spec_max=*std::max_element(specList.begin(),specList.end());
    
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

    /** This method returns workspace Indexes for the given spectrum indexes
      * @param mws_sptr - shared pointer to workspace
      * @param specList - list of spectra
      * @param startWSIndex - start index of workspace
      * @param endWSIndex  - end index of the workspace
    */

    void SANSDiagnostics::getWorkspaceIndexes(const Mantid::API::MatrixWorkspace_sptr& mws_sptr,
                                              const std::vector<specid_t>& specList,
                                              QString& startWSIndex,QString& endWSIndex)
    {      
            
      std::vector<size_t> wsindexList;
      mws_sptr->getIndicesFromSpectra(specList,wsindexList);
      std::string wsStart;
      std::string wsEnd;
      try
      {        
        wsStart= boost::lexical_cast<std::string>(wsindexList.at(0));
        wsEnd = boost::lexical_cast<std::string>(wsindexList.at(1));
      }
      catch(boost::bad_lexical_cast&)
      {
        g_log.error()<<"Error: Invalid start / end workspace index"<<std::endl;
      }
      catch(std::out_of_range&)
      {
        g_log.error()<<"Error: Invalid start / end workspace index"<<std::endl;
      }
     
      startWSIndex = QString::fromStdString(wsStart);
      endWSIndex =  QString::fromStdString(wsEnd);
          
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

  /**This method returns workspace name from the  file name 
    * @param fileName name of the file
    * @returns workspace name
    */
    QString SANSDiagnostics::getWorkspaceNameFileName(const QString& fileName)
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
    * @param specMin - minimum spectrum number
    * @param specMax - maximum spectrum number
    * @returns true if the spectra is in valid range.
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
        g_log.error()<<"Inavlid spectrum number found in  the selected detector bank "<<std::endl;
        return false;
      }
      if(spec_min<1 )
      {
        g_log.error()<<"Inavlid spectrum minimum "+specMin.toStdString()+ " found in  the selected detector bank "<<std::endl;
      }
      if(spec_max>Mantid::EMPTY_INT() )
      {
        g_log.error()<<"Inavlid spectrum maximum "+specMax.toStdString()+ " found in  the selected detector bank  "<<std::endl;
                                                                          
      }
      return ((spec_min>=1 && spec_max<=Mantid::EMPTY_INT())?true:false);
    }


    /// Handler for first detector horizontal integral button
    void SANSDiagnostics::firstDetectorHorizontalIntegralClicked()
    {           
      QString orientation("D_H");
      QString minSpec;
      QString maxSpec;
      int detNum=0;//first detector

      QString ipwsName= getWorkspaceToProcess();
      Mantid::API::Workspace_sptr ws_sptr;
      try
      {
        ws_sptr = Mantid::API::AnalysisDataService::Instance().retrieve(ipwsName.toStdString());

      }
      catch(Exception::NotFoundError&)
      {
        g_log.error()<<"Error when accessing the Workspace "+ipwsName.toStdString()<<std::endl;
        return;

      }
      Mantid::API::MatrixWorkspace_sptr mws_sptr=boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws_sptr);
      if(!mws_sptr)
      {
        return;
      }
      std::vector<specid_t> specList;
      getSpectraList(mws_sptr,detNum,specList);
      minandMaxSpectrumIds(specList,minSpec,maxSpec);
      if(!isValidSpectra(minSpec,maxSpec))
      {
        m_SANSForm->groupBox_Detector1->setDisabled(true);
        return;
      } 
      
      QString detName= getDetectorName(0);
      //give the detectorname_H for workspace
      detName+="_H";
      const QString opws(detName);
      ///horizontal integral range string
      QString hiRange=m_SANSForm->hirange_edit1->text();
     
      IntegralClicked(hiRange,orientation,minSpec,maxSpec,opws,m_SANSForm->tcmask1->isChecked(),true);
         
    }

    /// Handler for first detector vertical integral button
    void SANSDiagnostics::firstDetectorVerticalIntegralClicked()
    {
      QString orientation("D_V");
      QString minSpec;
      QString maxSpec;
      int64_t detNum=0;//first detector
     
      QString ipwsName= getWorkspaceToProcess();
      Mantid::API::Workspace_sptr ws_sptr;
      try
      {
        ws_sptr = Mantid::API::AnalysisDataService::Instance().retrieve(ipwsName.toStdString());

      }
      catch(Exception::NotFoundError&)
      {
        g_log.error()<<"Error when accessing the Workspace "+ipwsName.toStdString()<<std::endl;
        return;

      }
      Mantid::API::MatrixWorkspace_sptr mws_sptr=boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws_sptr);
      if(!mws_sptr)
      {
        return;
      }

      std::vector<specid_t> specList;
      getSpectraList(mws_sptr,detNum,specList);
      minandMaxSpectrumIds(specList,minSpec,maxSpec);

      if(!isValidSpectra(minSpec,maxSpec))
      {
        m_SANSForm->groupBox_Detector1->setDisabled(true);
        return;
      }
      QString detName= getDetectorName(0);
      //give the detectorname_V for workspace
      detName+="_V";
      QString opws(detName);
      ///horizontal integral range string
      QString viRange=m_SANSForm->virange_edit1->text();
      
      IntegralClicked(viRange,orientation,minSpec,maxSpec,opws,m_SANSForm->tcmask2->isChecked(),true);

    }


    /// Handler for first detector time integral button
    void SANSDiagnostics::firstDetectorTimeIntegralClicked()
    {
      int64_t detNum=0;//first detector
      QString minSpec;
      QString maxSpec;
     
      QString ipwsName= getWorkspaceToProcess();
      Mantid::API::Workspace_sptr ws_sptr;
      try
      {
        ws_sptr = Mantid::API::AnalysisDataService::Instance().retrieve(ipwsName.toStdString());

      }
      catch(Exception::NotFoundError&)
      {
        g_log.error()<<"Error when accessing the Workspace "+ipwsName.toStdString()<<std::endl;
        return;

      }
      Mantid::API::MatrixWorkspace_sptr mws_sptr=boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws_sptr);
      if(!mws_sptr)
      {
        return;
      }

      std::vector<specid_t> specList;
      getSpectraList(mws_sptr,detNum,specList);
      minandMaxSpectrumIds(specList,minSpec,maxSpec);
      QString wsStartIndex, wsEndIndex;
      
      if(!isValidSpectra(minSpec,maxSpec))
      {
        m_SANSForm->groupBox_Detector1->setDisabled(true);
        return;
      }
      /// now run the load algorithm with the spec_min and spec_max
      if(!runLoadAlgorithm(m_fileName,minSpec,maxSpec))
      {
        return;
      }

      QString loadedws=getWorkspaceToProcess(); 
      Mantid::API::Workspace_sptr loadedws_sptr;
      try
      {
        loadedws_sptr = Mantid::API::AnalysisDataService::Instance().retrieve(loadedws.toStdString());

      }
      catch(Exception::NotFoundError&)
      {
        g_log.error()<<"Error when accessing the Workspace "+loadedws.toStdString()<<std::endl;
        return;

      }
      Mantid::API::MatrixWorkspace_sptr mws_sptr1=boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(loadedws_sptr);
      if(!mws_sptr1)
      {
        return;
      }
      //get the workspace index from spectra list
      getWorkspaceIndexes(mws_sptr1,specList,wsStartIndex,wsEndIndex);
      
      //apply mask
      maskDetector(loadedws,m_SANSForm->pmask1->isChecked(),false);
      
      QString detName= getDetectorName(0);
      //give the detectorname_V for workspace
      detName+="_T";
      QString opws(detName);
           
      //execute SumSpectra 
      if(!runsumSpectra(loadedws,opws,wsStartIndex,wsEndIndex))
      {
        return;
      }
     
      QString plotws = "\"" + opws +"\"";
      plotSpectrum(plotws,0);
    }



   /**This method gets called from the handler of Vertical/Horizontal Integral button click.
    * executes LoadRaw and SumRowColumn algorithm.
    * @param range string entered by user
    * @param orientation orientation
    * @param specMin- minimum spectrum index
    * @param specMax - maximum spectrum index
    * @param opws - output workspace.
    * @param bMask boolean used for masking
    * @param time_pixel true if time masking,false if pixel mask
    */
    void SANSDiagnostics::IntegralClicked(const QString& range,const QString& orientation,
      const QString& specMin,const QString& specMax,const QString& opws,bool bMask,bool time_pixel)
    {
      /// now run the load algorithm with the spec_min and spec_max
      if(!runLoadAlgorithm(m_fileName,specMin,specMax))
      {
        return;
      } 
      //get the workspace name
      QString loadedws = getWorkspaceToProcess(); 
            
      //aplly mask
      if(bMask)
      {
        maskDetector(loadedws,bMask,time_pixel);
      }
    
      if(range.isEmpty())
      {        
        QString HVMin,HVMax;
        HVMinHVMaxStringValues(Mantid::EMPTY_INT(),Mantid::EMPTY_INT(),HVMin,HVMax);
        if(!runsumRowColumn(loadedws,opws,orientation,HVMin,HVMax))
        {
          return ;
        }
        QString plotws = "\"" + opws +"\"";
        plotSpectrum(plotws,0);
        return;
      }
      //parse the range string
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
           
      QString wsPlotString;
      //loop through each element of the parsed value vector
      std::vector<std::vector<unsigned int> >::const_iterator parsedValcitr;
      for(parsedValcitr=parsedVals.begin();parsedValcitr!=parsedVals.end();++parsedValcitr)
      {
        if((*parsedValcitr).empty())
        {
          return;
        }
        //check the vector contains sequential vales.
        if(!isSequentialValues((*parsedValcitr)))
        {
          g_log.error("Values between H/V_Min and H/V_Max in the Range string  are not sequential ");
          return ;
        }
        //first value in the vector  is the HVmin
        int min=(*parsedValcitr).at(0);
        int max;
        ///last value is HVMax
        if((*parsedValcitr).size()>1)
        {
          max=(*parsedValcitr).at((*parsedValcitr).size()-1);
        }
        else
        {
          //if the vector contains only one value HVMax=HVMin
          max=min;
        }

        QString HVMin,HVMax;
        HVMinHVMaxStringValues(min,max,HVMin,HVMax);
        
        ++count;
        std::stringstream num;
        num<<count;
        QString outputwsname=opws+QString::fromStdString(num.str());

        //now execute sumrowcolumn with hvmin and havmax from the first and last vales from the vector
        if(!runsumRowColumn(loadedws,outputwsname,orientation,HVMin,HVMax))
        {
          return ;
        }
               
        wsPlotString+="\"";
        wsPlotString+=outputwsname;
        wsPlotString+="\"";
        wsPlotString+=",";
      }

      //remove the last comma
      int index=wsPlotString.lastIndexOf(",");
      wsPlotString.remove(index,1);
  
      //plot the zeroth spectrum for all the workspaces created.
      int specNum=0;
      plotSpectrum(wsPlotString,specNum);
    
    }

    /** Get values of HVMin and HvMax values for sumrowcolumn algorithm
      * @param minVal spectrum min value
      * @param maxVal spectrum maximum value
      * @param hvMin  spectrum min string
      * @param hvMax  spectrum max string
    */
    void SANSDiagnostics::HVMinHVMaxStringValues(const int minVal,const int maxVal,QString& hvMin,QString& hvMax)
    {
      try
      {
        //first value in the vector 
        hvMin=QString::fromStdString(boost::lexical_cast<std::string>(minVal));
        ///last value is HVMax
        hvMax=QString::fromStdString(boost::lexical_cast<std::string>(maxVal));
      }
      catch(boost::bad_lexical_cast& )
      {
        g_log.error("Error when getting the H/V_Min and H/V_Max value from the Range string ");
      }
      
    }

   /** This method applys time channel masks to the detector bank selected.
      * @param wsName - name of the workspace.
      * @param bMask - boolean flag to indicate the mask check box selected
      */
    void SANSDiagnostics::maskDetector(const QString& wsName,bool bMask,bool time_pixel)
    {
      //if  mask control selected
      if(bMask)
      {
        emit applyMask(wsName,time_pixel);
      }
    }

  /**This method plots spectrum for the given workspace
    * @param wsName - name of the workspace
    * @param specNum - spectrum number
    */ 
    void SANSDiagnostics::plotSpectrum(const QString& wsName,int specNum)
    {      
      QString plotspec="plotSpectrum([";
      plotspec+=wsName;
      plotspec+="],";
      plotspec+=QString::number(specNum);
      plotspec+=")";
      runPythonCode(plotspec);
    }

    /// Handler for second detector horizontal integral button
    void SANSDiagnostics::secondDetectorHorizontalIntegralClicked()
    {
      QString orientation("D_H");
      QString minSpec;
      QString maxSpec;
      int64_t detNum=1;//second detector
      
      QString ipwsName= getWorkspaceToProcess();
      Mantid::API::Workspace_sptr ws_sptr;
      try
      {
        ws_sptr = Mantid::API::AnalysisDataService::Instance().retrieve(ipwsName.toStdString());

      }
      catch(Exception::NotFoundError&)
      {
        g_log.error()<<"Error when accessing the Workspace "+ipwsName.toStdString()<<std::endl;
        return;

      }
      Mantid::API::MatrixWorkspace_sptr mws_sptr=boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws_sptr);
      if(!mws_sptr)
      {
        return;
      }
      
      std::vector<specid_t> specList;
      getSpectraList(mws_sptr,detNum,specList);
      minandMaxSpectrumIds(specList,minSpec,maxSpec);
       
      if(!isValidSpectra(minSpec,maxSpec))
      {
        m_SANSForm->groupBox_Detector2->setDisabled(true);
        return;
      }
      QString detName= getDetectorName(detNum);
      //give the detectorname_H for workspace
      detName+="_H";
      QString opws(detName);
      ///horizontal integral range string
      QString hiRange=m_SANSForm->hirange_edit2->text();
      IntegralClicked(hiRange,orientation,minSpec,maxSpec,opws,m_SANSForm->tcmask3->isChecked(),true);

    }
    /// Handler for second detector horizontal integral button
    void SANSDiagnostics::secondDetectorVerticalIntegralClicked()
    {      
      QString orientation("D_V");
      QString minSpec;
      QString maxSpec;
      int64_t detNum=1;//first detector

      QString ipwsName= getWorkspaceToProcess();
      Mantid::API::Workspace_sptr ws_sptr;
      try
      {
        ws_sptr = Mantid::API::AnalysisDataService::Instance().retrieve(ipwsName.toStdString());

      }
      catch(Exception::NotFoundError&)
      {
        g_log.error()<<"Error when accessing the Workspace "+ipwsName.toStdString()<<std::endl;
        return;

      }
      Mantid::API::MatrixWorkspace_sptr mws_sptr=boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws_sptr);
      if(!mws_sptr)
      {
        return;
      }

      std::vector<specid_t> specList;
      getSpectraList(mws_sptr,detNum,specList);
      minandMaxSpectrumIds(specList,minSpec,maxSpec);
      if(!isValidSpectra(minSpec,maxSpec))
      {
        m_SANSForm->groupBox_Detector2->setDisabled(true);
        return;
      }
      QString detName= getDetectorName(detNum);
      //give the detectorname_H for workspace
      detName+="_V";
      QString opws(detName);

      ///horizontal integral range string
      QString viRange=m_SANSForm->virange_edit2->text();
      IntegralClicked(viRange,orientation,minSpec,maxSpec,opws,m_SANSForm->tcmask4->isChecked(),true);
    }
    /// Handler for second detector horizontal integral button
    void SANSDiagnostics::secondDetectorTimeIntegralClicked()
    {
      //second detector
      int64_t detNum=1;
      QString minSpec;
      QString maxSpec;

      //Get the workspace created by load algorithm initially with specmin=1 and specmax=1
      QString initialwsName= getWorkspaceToProcess();
      Mantid::API::Workspace_sptr ws_sptr;
      try
      {
        ws_sptr = Mantid::API::AnalysisDataService::Instance().retrieve(initialwsName.toStdString());
      }
      catch(Exception::NotFoundError&)
      {
        g_log.error()<<"Error when accessing the Workspace "+initialwsName.toStdString()<<std::endl;
        return;
      }
      Mantid::API::MatrixWorkspace_sptr mws_sptr=boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws_sptr);
      if(!mws_sptr)
      {
        return;
      }
            
      std::vector<specid_t> specList;
      //get spectrum list from detector ids
      getSpectraList(mws_sptr,detNum,specList);
      // get maximum and minimum spectrum ids
      minandMaxSpectrumIds(specList,minSpec,maxSpec);
      QString wsStartIndex, wsEndIndex;
      //   
      if(!isValidSpectra(minSpec,maxSpec))
      {
        m_SANSForm->groupBox_Detector2->setDisabled(true);
        return;
      }
      /// now run the load algorithm with the spec_min and spec_max
      if(!runLoadAlgorithm(m_fileName,minSpec,maxSpec))
      {
        return;
      } 
      //Get the workspace created by load algorithm  with spec_min and specmax for the detectors
      QString loadedws= getWorkspaceToProcess();
      //apply mask
      maskDetector(loadedws,m_SANSForm->pmask2->isChecked(),false);
    
      Mantid::API::Workspace_sptr loadedws_sptr;
      try
      {
        loadedws_sptr = Mantid::API::AnalysisDataService::Instance().retrieve(loadedws.toStdString());

      }
      catch(Exception::NotFoundError&)
      {
        g_log.error()<<"Error when accessing the Workspace "+loadedws.toStdString()<<std::endl;
        return;

      }
      Mantid::API::MatrixWorkspace_sptr mloadedws_sptr=boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(loadedws_sptr);
      if(!loadedws_sptr)
      {
        return;
      }
      //get workspace indexes
      getWorkspaceIndexes(mloadedws_sptr,specList,wsStartIndex,wsEndIndex);
      
      QString detName= getDetectorName(1);
      //give the detectorname_V for workspace
      detName+="_T";
      QString opws(detName);
      //execute SumSpectra 
     if(!runsumSpectra(loadedws,opws,wsStartIndex,wsEndIndex))
     {
       return;
     }

     QString plotws = "\"" + opws +"\"";
     plotSpectrum(plotws,0);

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
        g_log.error()<<"Error: when accessing the workspace "<<m_outws_load.toStdString()<<std::endl;
        return 0;
      }
      if(Mantid::API::WorkspaceGroup_sptr wsgrp_sptr=boost::dynamic_pointer_cast<WorkspaceGroup>(ws_sptr))
      {
        return wsgrp_sptr->getNumberOfEntries();
      }
      return 1;
    }
    /// This method loads last saved settings values from registry
    void SANSDiagnostics::loadSettings()
    {
     
      QSettings settings;
      m_settingsGroup = "CustomInterfaces/SANSRunWindow/SANSDiagnostics";
      settings.beginGroup(m_settingsGroup);
      m_SANSForm->file_run_edit->readSettings(settings.group());
      settings.endGroup();
    }

    ///save settings
    void SANSDiagnostics::saveSettings()
    {
      m_dataDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("datasearch.directories"));
      m_dataDir = m_dataDir.split(";", QString::SkipEmptyParts)[0];
      QSettings settings;
      m_settingsGroup="CustomInterfaces/SANSRunWindow/SANSDiagnostics";
      settings.beginGroup(m_settingsGroup);
      settings.setValue("last_directory", m_dataDir);
      settings.setValue("File",getFileName());
      settings.endGroup();
      m_SANSForm->file_run_edit->saveSettings(settings.group());
      
    }

    /**Execute sumrowcolumn algorithm 
    * @param values a vector containing the values for hvmin,hvmax
    * @param periodNum - number of the  period to load
    * @param ipws - name of the i/p workspace
    * @param opws - name of the o/p workspace
    * @param orientation - orientation of the detector
    */
    bool SANSDiagnostics::executeSumRowColumn(const std::vector<unsigned int>& values,
      const QString ipws,const QString& opws,const QString& orientation)
    {      
      if(values.empty())
      {
        return false ;
      }

      QString HVMin;
      QString HVMax; 
      //check the vector contains sequential vales.
      if(!isSequentialValues(values))
      {
        g_log.error("Values between HVMin and HVMax in the Range string  are not sequential ");
        return false;
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
      if(!runsumRowColumn(ipws,opws,orientation,HVMin,HVMax))
      {
        return false;
      }
      return true;
    }

  /**Execute sumrowcolumn algorithm with the vales
    * @param ipwsName name of the input workspace
    * @param opwsName name of the output workspace
    * @param orientation indicates row or columns to sum
    * @param hvMin minimum value of
    * @param hvMax
    */
    bool SANSDiagnostics::runsumRowColumn(const QString ipwsName,const QString& opwsName,
      const QString& orientation,const QString& hvMin,const QString& hvMax)
    {

       Mantid::API::Workspace_sptr ws_sptr;
      try
      {
        ws_sptr = Mantid::API::AnalysisDataService::Instance().retrieve(ipwsName.toStdString());

      }
      catch(Exception::NotFoundError&)
      {
        g_log.error()<<"Error when  trying to access the workspace "<<ipwsName.toStdString()<<" which is not loaded"<<std::endl;
        return false;
      }
      if(opwsName.isEmpty())
      {
         g_log.error()<<"Output workspace name is empty , can't create workspace with empty name"<<std::endl;
         return false;
      }
      if(hvMin.isEmpty())
      {
         g_log.error()<<"Error when executing SumRowColumn algorithm :Empty H/V_Min String value "<<std::endl;
         return false;
      }
      if(hvMax.isEmpty())
      {
         g_log.error()<<"Error when executing SumRowColumn algorithm :Empty H/V_Max String value "<<std::endl;
         return false;
      }

      QString code=
        "try:\n"
        "  SumRowColumn('";
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
      code+=")\n";
      code+="except:\n";
      code+="  print 'Failure'";

      QString ret= runPythonCode(code.trimmed());
      if(!ret.isEmpty())
      {
        g_log.error()<<"Error when executing the SumRowColumn algorithm "<<ret.toStdString()<<std::endl;
        return false;
      }
      return true;
    }

  /** This method creates script string for sumspectra algorithm
    * @param ipwsName - name of the input workspace
    * @param opwsName - name of the ooutput workspace
    * @param wsStartIndex - start workspace index
    * @param wsEndIndex - end workspace Index
    * @returns - sumspectra script string
    */
    bool  SANSDiagnostics::runsumSpectra(const QString& ipwsName,const QString& opwsName,const QString& wsStartIndex,const QString& wsEndIndex)
    {
      
      if(opwsName.isEmpty())
      {
         g_log.error()<<"Output workspace name is empty , can't create workspace with empty name"<<std::endl;
         return false;
      }

      if(wsStartIndex.isEmpty())
      {
        g_log.error()<<"Error: Invalid StartWorkspaceIndex"<<std::endl;
        return false;
      }
      if(wsEndIndex.isEmpty())
      {
        g_log.error()<<"Error Invalid EndWorkspaceIndex"<<std::endl;
        return false;
      }
           
      QString code=
        "try:\n"
        "  SumSpectra(\"";
      code+=ipwsName;
      code+="\",\"";
      code+=opwsName;
      code+="\",";
      code+="StartWorkspaceIndex=";
      code+=wsStartIndex;
      code+=",";
      code+="EndWorkspaceIndex=";
      code+=wsEndIndex;
      code+=")\n";
      code+="except:\n";
      code+="  print 'Failure'"; 
     
      QString ret= runPythonCode(code.trimmed());
      if(!ret.isEmpty())
      {
        g_log.error()<<"Error when executing the SumSpectra algorithm "<<ret.toStdString()<<std::endl;
        return false;
      }
      return true;
    }


  /**This method executes load algorithm with filename,specmin,specmax
    * @param fileName - name of the file
    * @param specMin  - spectrum minimum
    * @param specMax  - spectrum maximum
    */
    bool SANSDiagnostics::runLoadAlgorithm(const QString& fileName,const QString& specMin,const QString& specMax)
    {

      if(fileName.isEmpty()) return false;
      //get the output workspace for load algorithm from file name
      m_outws_load=getWorkspaceNameFileName(fileName);
      if(m_outws_load.isEmpty())
      {
         g_log.error()<<"Output workspace name is empty , can't create workspace with empty name"<<std::endl;
         return false;
      }
        
      QString load=
        "try:\n"
        "  Load('";
      load+=fileName;
      load+="','";
      load+=m_outws_load;
      load+="',";
      load+="SpectrumMin=";
      load+=specMin;
      load+=",";
      load+="SpectrumMax=";
      load+=specMax;
      load+=")\n";
      load+="except:\n";
      load+="  print 'Failure'"; 
      QString ret= runPythonCode(load.trimmed());
      if(!ret.isEmpty())
      {
        g_log.error()<<"Error when executing the Load algorithm "<<std::endl;
        return false;
      }
      return true;
    }
    /**Checks the values in the given vector is sequential
    * @param values  vector containing numbers
    * @returns true if the vector is sequential
    */
    bool SANSDiagnostics::isSequentialValues(const std::vector<unsigned int>& values)
    {
      try
      {
        unsigned int startVal=values.at(0);
        std::vector<unsigned int>::const_iterator citr;
        for(size_t i=1;i<values.size();++i)
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
        g_log.error("Error when executing SumRowColumn Algorithm" );
        return false;
      }
      return true;
    }
    
  /** This method enables the mask controls in teh diagnostics UI
    */
    void  SANSDiagnostics::enableMaskFileControls()
    {
      //enable time and pixel check masks
      m_SANSForm->tcmask1->setEnabled(true);
      m_SANSForm->tcmask2->setEnabled(true);
      m_SANSForm->pmask1->setEnabled(true);
      m_SANSForm->tcmask3->setEnabled(true);
      m_SANSForm->tcmask4->setEnabled(true);
      m_SANSForm->pmask2->setEnabled(true);
    }

  }
}
