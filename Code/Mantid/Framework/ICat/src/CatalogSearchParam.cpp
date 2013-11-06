#include "MantidICat/CatalogSearchParam.h"
#include "MantidKernel/DateAndTime.h"
#include <boost/algorithm/string/regex.hpp>

namespace Mantid
{
  namespace ICat
  {
    /// constructor
    CatalogSearchParam::CatalogSearchParam():m_startRun(0),m_endRun(0),m_caseSensitive(false),
        m_startDate(0),m_endDate(0)
    {}
    /// Destructor
    CatalogSearchParam::~CatalogSearchParam(){}
    /** This method  sets start date
     *  @param startRun start run number
     */
    void CatalogSearchParam::setRunStart(const double& startRun){m_startRun=startRun;}
    /** This method  sets end date
     *  @param endRun  end run number
     */
    void CatalogSearchParam::setRunEnd(const double& endRun){m_endRun=endRun;}
    /** This method  sets isntrument name
     *  @param instrName  name of the instrument
     */
    void CatalogSearchParam::setInstrument(const std::string& instrName){m_instrName=instrName;}
    /** This method  sets the start date
     *  @param startDate  start date for search
     */
    void CatalogSearchParam::setStartDate(const time_t& startDate){m_startDate=startDate;}
    /** This method  sets the end date
     *  @param endDate end date for search
     */
    void CatalogSearchParam::setEndDate(const time_t& endDate){m_endDate=endDate;}
    /** This method  sets the CaseSensitive
     *  @param bCase  flag to do case sensitive  search
     */
    void CatalogSearchParam::setCaseSensitive(bool bCase){m_caseSensitive=bCase;}

    /** This method  sets the InvestigationInclude
     *  @param keywords keywords used for search
     */
    void CatalogSearchParam::setKeywords(const std::string& keywords){m_keywords=keywords;}

    /** This method  sets investigationName used for searching
     *  @param instName  name of the investigation
     */
    void  CatalogSearchParam::setInvestigationName(const std::string& instName){ m_investigationName = instName;}

    /** This method  sets investigationAbstract used for searching
     *  @param invstabstract  abstract of the investigation
     */
    void CatalogSearchParam::setInvestigationAbstract(const std::string& invstabstract){ m_investigationAbstract=invstabstract;}

    /** This method  sets sample used for searching
     *  @param sampleName name of the sample
     */
    void CatalogSearchParam::setSampleName(const std::string& sampleName){ m_sampleName = sampleName;}

    /** This method  sets Investigator surname
     *@param investigatorName  surname of the investigator
     */
    void  CatalogSearchParam::setInvestigatorSurName(const std::string& investigatorName){m_investigatorSurname = investigatorName;}

    /** This method  sets Rb Number
     *@param RbNumber rutherford board number
     */
    void CatalogSearchParam::setRbNumber(const std::string& RbNumber){m_RbNumber = RbNumber;}

    /** This method  sets Investigation Type
     * @param invstType type of investigation
     */
    void CatalogSearchParam::setInvestigationType(const std::string& invstType){m_investigationType = invstType;}

    /** This method  sets datafileName
     *@param datafileName name of the file
     */
    void CatalogSearchParam::setDatafileName(const std::string& datafileName ){ m_datafileName =datafileName;}

    /**
     * Sets the "My data only" checkbox.
     * @param flag :: Flag to search in "My data" only.
     */
    void CatalogSearchParam::setMyData(bool flag) { m_myData = flag;}

    /** This method  returns the start run number
     *  @returns  run start number
     */
    const double& CatalogSearchParam::getRunStart()const {return m_startRun; }
    /** This method  returns the end run number
     *  @returns  run end number
     */
    const double& CatalogSearchParam::getRunEnd() const {return m_endRun;}
    /** This method  returns the instrument name
     *  @returns  instrument name
     */
    const std::string& CatalogSearchParam::getInstrument() const{return m_instrName;}
    /**This method  returns the start date
     * @returns  start date
     */

    const time_t& CatalogSearchParam::getStartDate()const{return m_startDate;}
    /** This method  returns the end date
     *  @returns end date for investigations serch
     */
    const time_t& CatalogSearchParam::getEndDate()const{return m_endDate;}

    /** This method  returns case sensitive flag
     *  @returns  case sensitive flag
     */
    bool CatalogSearchParam::getCaseSensitive()const{return m_caseSensitive;}
    /** This method  returns the enum for data search in icat db
     *  @returns  investigation include
     */

    const std::string& CatalogSearchParam::getKeywords()const{return m_keywords;}

    /** This method  returns investigationName used for searching
     *  @ returns investigation name
     */
    const std::string& CatalogSearchParam::getInvestigationName()const{return m_investigationName;}

    /** This method  returns investigationAbstract used for searching
     *  @returns investigation abstract
     */
    const std::string&  CatalogSearchParam::getInvestigationAbstract()const{return m_investigationAbstract;}

    /** This method  returns sample used for searching
     *  @returns samplename
     */
    const std::string& CatalogSearchParam::getSampleName()const{return m_sampleName;}

    /** This method  returns Investigator surname
     *@returns surname of the investigator
     */
    const std::string& CatalogSearchParam::getInvestigatorSurName()const{return m_investigatorSurname;}

    /** This method  returns Rb Number
     * @returns Rb number
     */
    const std::string& CatalogSearchParam::getRbNumber()const{return m_RbNumber;}

    /** This method  returns Investigation Type
        @ returns type of the investigation
     */
    const std::string& CatalogSearchParam::getInvestigationType()const{return m_investigationType;}

    /** This method  returns datafileName
     * @returns name of the data file
     */
    const std::string& CatalogSearchParam::getDatafileName()const{return m_datafileName;}

    /**
     * Creates a time_t value from an input date ("23/06/2003").
     * @param inputDate :: string containing the date.
     * @return time_t value of date
     */
    time_t CatalogSearchParam::getTimevalue(const std::string& inputDate)
    {
      // Prevent any possible errors.
      if(inputDate.empty()) return 0;
      // A container to hold the segments of the date.
      std::vector<std::string> dateSegments;
      // Split input by "/" prior to rearranging the date
      boost::algorithm::split_regex(dateSegments, inputDate, boost::regex("/"));
      // Reorganise the date to be ISO format.
      std::string isoDate = dateSegments.at(2) + "-" + dateSegments.at(1) + "-" + dateSegments.at(0) + " 0:00:00.000";
      // Return the date as time_t value.
      return Kernel::DateAndTime(isoDate).to_time_t();
    }

    /**
     * Is "My data only" selected?
     * @returns true if my data checkbox is selected.
     */
    bool CatalogSearchParam::getMyData() const { return (m_myData); }
  }
}
