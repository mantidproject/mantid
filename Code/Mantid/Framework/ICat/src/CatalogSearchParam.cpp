#include "MantidICat/CatalogSearchParam.h"
#include <boost/lexical_cast.hpp> 

namespace Mantid
{
  namespace ICat
  {
    /// constructor
    CatalogSearchParam::CatalogSearchParam() : m_startRun(0),m_endRun(0),
        m_startDate(0),m_endDate(0), m_myData(false) {}

    /// Destructor
    CatalogSearchParam::~CatalogSearchParam() {}

    /**
     * This method sets start date
     * @param startRun :: start run number
     */
    void CatalogSearchParam::setRunStart(const double& startRun)
    {
      m_startRun = startRun;
    }

    /**
     * This method sets end date
     * @param endRun :: end run number
     */
    void CatalogSearchParam::setRunEnd(const double& endRun)
    {
      m_endRun = endRun;
    }

    /**
     * This method sets instrument name
     * @param instrName :: name of the instrument
     */
    void CatalogSearchParam::setInstrument(const std::string& instrName)
    {
      m_instrName = instrName;
    }

    /**
     * This method sets the start date
     * @param startDate :: start date for search
     */
    void CatalogSearchParam::setStartDate(const time_t& startDate)
    {
      m_startDate = startDate;
    }

    /**
     * This method sets the end date
     * @param endDate :: end date for search
     */
    void CatalogSearchParam::setEndDate(const time_t& endDate)
    {
      m_endDate = endDate;
    }

    /**
     * This method sets the keywords to search for
     * @param keywords :: keywords used for search
     */
    void CatalogSearchParam::setKeywords(const std::string& keywords)
    {
      m_keywords = keywords;
    }

    /**
     * This method sets investigationName used for searching
     * @param instName :: name of the investigation
     */
    void CatalogSearchParam::setInvestigationName(const std::string& instName)
    {
      m_investigationName = instName;
    }

    /**
     * This method sets data file name used for searching
     * @param datafileName :: data file name to search for
     */
    void CatalogSearchParam::setDatafileName(const std::string& datafileName)
    {
      m_datafileName = datafileName;
    }

    /**
     * This method sets sample used for searching
     * @param sampleName :: name of the sample
     */
    void CatalogSearchParam::setSampleName(const std::string& sampleName)
    {
      m_sampleName = sampleName;
    }

    /**
     * This method sets Investigator name
     * @param investigatorName :: name of the investigator
     */
    void CatalogSearchParam::setInvestigatorSurName(const std::string& investigatorName)
    {
      m_investigatorSurname = investigatorName;
    }

    /**
     * This method sets Investigation Type
     * @param invstType :: type of investigation
     */
    void CatalogSearchParam::setInvestigationType(const std::string& invstType)
    {
      m_investigationType = invstType;
    }

    /**
     * Sets the "My data only" checkbox.
     * @param flag :: Flag to search in "My data" only.
     */
    void CatalogSearchParam::setMyData(bool flag)
    {
      m_myData = flag;
    }

    /**
     * This method returns the start run number
     * @return Run start number
     */
    const double& CatalogSearchParam::getRunStart() const
    {
      return m_startRun;
    }

    /**
     * Gets the input from the investigation name field.
     * @return Run end number
     */
    const double& CatalogSearchParam::getRunEnd() const
    {
      return m_endRun;
    }

    /**
     * Gets the input from the instrument name field.
     * @return Instrument name
     */
    const std::string& CatalogSearchParam::getInstrument() const
    {
      return m_instrName;
    }

    /**
     * Gets the input from the start date name field.
     * @return Start date
     */
    const time_t& CatalogSearchParam::getStartDate() const
    {
      return m_startDate;
    }

    /**
     * Gets the input from the end date name field.
     * @return End date for investigations search
     */
    const time_t& CatalogSearchParam::getEndDate() const
    {
      return m_endDate;
    }

    /**
     * Gets the input from the keywords name field.
     * @return Investigation include
     */
    const std::string& CatalogSearchParam::getKeywords() const
    {
      return m_keywords;
    }

    /**
     * Gets the input from the investigation name field.
     * @return Investigation name
     */
    const std::string& CatalogSearchParam::getInvestigationName() const
    {
      return m_investigationName;
    }

    /**
     * Gets the input from the datafile name field.
     * @return Datafile name
     */
    const std::string& CatalogSearchParam::getDatafileName() const
    {
      return m_datafileName;
    }

    /**
     * Gets the input from the sample name field.
     * @return Samplename
     */
    const std::string& CatalogSearchParam::getSampleName() const
    {
      return m_sampleName;
    }

    /**
     * Gets the input from the investigator name field.
     * @return Surname of the investigator
     */
    const std::string& CatalogSearchParam::getInvestigatorSurName() const
    {
      return m_investigatorSurname;
    }

    /**
     * Gets the input from the investigation type field.
     * @return Type of the investigation
     */
    const std::string& CatalogSearchParam::getInvestigationType() const
    {
      return m_investigationType;
    }

    /**
     * Is "My data only" selected?
     * @returns true if my data checkbox is selected.
     */
    bool CatalogSearchParam::getMyData() const
    {
      return m_myData;
    }

    /**
     * This method saves the date components to C library struct tm
     * @param sDate :: string containing the date
     * @return time_t value of date
     */
    time_t CatalogSearchParam::getTimevalue(const std::string& sDate)
    {

      if(!sDate.compare(""))
      {
        return 0;
      }
      struct tm  timeinfo;
      std::basic_string <char>::size_type index,off=0;
      int day,month,year;

      //look for the first '/' to extract day part from the date string
      index=sDate.find('/',off);
      if(index == std::string::npos)
      {
        throw std::runtime_error("Invalid Date:date format must be DD/MM/YYYY");
      }
      //get day part of the date
      try
      {
        day=boost::lexical_cast<int>(sDate.substr(off,index-off).c_str());
      }
      catch(boost::bad_lexical_cast&)
      {
        throw std::runtime_error("Invalid Date");
      }
      timeinfo.tm_mday=day;

      //change the offset to the next position after "/"
      off=index+1;
      //look for 2nd '/' to get month part from  the date string
      index=sDate.find('/',off);
      if(index == std::string::npos)
      {
        throw std::runtime_error("Invalid Date:date format must be DD/MM/YYYY");
      }
      //now get the month part
      try
      {
        month=boost::lexical_cast<int>(sDate.substr(off,index-off).c_str());
      }
      catch(boost::bad_lexical_cast&)
      {
        throw std::runtime_error("Invalid Date");
      }
      timeinfo.tm_mon=month-1;

      //change the offset to the position after "/"
      off=index+1;
      //now get the year part from the date string
      try
      {
        year=boost::lexical_cast<int>(sDate.substr(off,4).c_str());

      }
      catch(boost::bad_lexical_cast&)
      {
        throw std::runtime_error("Invalid Date");
      }

      timeinfo.tm_year=year-1900;
      timeinfo.tm_min=0;
      timeinfo.tm_sec=0;
      timeinfo.tm_hour=0;
      return std::mktime (&timeinfo );
    }

  }
}
