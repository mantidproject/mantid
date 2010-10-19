#ifndef MANTID_ICAT_SAERCHPARAM_H_
#define MANTID_ICAT_SAERCHPARAM_H_

#include <string>

namespace Mantid
{
	namespace ICat
	{
        class CSearchParam
		{
		public:
			/// constructor
			CSearchParam();
			/// Destructor
			~CSearchParam();
			/** This method  sets start date
             *  @param startRun start run number
			 */
			void setRunStart(const double& startRun);
			/** This method  sets end date
           	 *  @param endRun end run number
			 */
			void setRunEnd(const double& endRun);
			/** This method  sets isntrument name
           	 *  @param instrName name of the instrument
			 */
			void setInstrument(const std::string& instrName);
			/** This method  sets the start date
           	 *  @param startDate start date for search
			 */
			void setStartDate(const time_t& startDate);
			/** This method  sets the end date
           	 *  @param endDate end date for search
			 */
			void setEndDate(const time_t& endDate);
			/** This method  sets the CaseSensitive
           	 *  @param setCaseSensitive flag to do case sensitive  search
			 */
			void setCaseSensitive(bool bCase);
			
			/** This method  sets the InvestigationInclude
           	 *  @param include enum for selecting data from the icat db
			 */
			void setKeywords(const std::string& keywords);

			/** This method  sets investigationName used for searching
           	 *  @param investigation name
			 */
			void  setInvestigationName(const std::string& instName);

			/** This method  sets investigationAbstract used for searching
           	 *  @param investigation abstract
			 */
			 void setInvestigationAbstract(const std::string& invstabstract);

			/** This method  sets sample used for searching
           	 *  @param samplename
			 */
			void setSampleName(const std::string& sampleName);

			/** This method  sets Investigator surname
           	 *@param surname of the investigator
			 */
			void  setInvestigatorSurName(const std::string& investigatorName);

			/** This method  sets Rb Number
           	 *@param Rb number
			 */
			 void setRbNumber(const std::string& RbNumber);

			/** This method  sets Investigation Type
           	 *@param Rb number
			 */
			 void setInvestigationType(const std::string& invstType);

			/** This method  sets datafileName
           	 *@param m_datafileName
			 */
			void setDatafileName(const std::string& datafileName );

			/** This method  returns the start run number
           	 *  @returns  run start number
			 */
			const double& getRunStart()const  ;
			/** This method  returns the end run number
           	 *  @returns  run end number
			 */
			const double& getRunEnd() const;
			/** This method  returns the instrument name
           	 *  @returns  instrument name
			 */
			const std::string& getInstrument() const;
			/**This method  returns the start date
           	 * @returns  start date
			 */
           
			 const time_t& getStartDate() const;
			/** This method  returns the end date
           	 *  @returns end date for investigations serch
			 */
			 const time_t& getEndDate() const;
			/** This method  returns case sensitive flag
           	 *  @returns  case sensitive flag
			 */
			bool getCaseSensitive() const;
			
			/** This method  returns keywords used for searching
           	 *  @returns keywords
			 */
			const std::string& getKeywords() const;

			/** This method  returns investigationName used for searching
           	 *  @ returns investigation name
			 */
			const std::string& getInvestigationName() const;

			/** This method  returns investigationAbstract used for searching
           	 *  @returns investigation abstract
			 */
			const std::string& getInvestigationAbstract() const;

			/** This method  returns sample used for searching
           	 *  @returns samplename
			 */
			const std::string& getSampleName() const;

			/** This method  returns Investigator surname
           	 *@returns surname of the investigator
			 */
			const std::string& getInvestigatorSurName() const;

			/** This method  returns Rb Number
           	 *@returns Rb number
			 */
			const std::string& getRbNumber() const;

			/** This method  returns Investigation Type
           	 *@returns Rb number
			 */
			 const std::string& getInvestigationType() const;

			/** This method  returns datafileName
           	 *@returns m_datafileName
			 */
			 const std::string& getDatafileName() const;

			/// This method returns the time_t value for a Date which is in "DD/MM/YYYY" format
			time_t getTimevalue(const std::string& sDate);

		private:
			double m_startRun;
			double m_endRun;
			std::string m_instrName;
			std::string m_keywords;
			bool m_caseSensitive;
			time_t m_startDate;
			time_t m_endDate;
			//ns1__investigationInclude m_include;

			std::string m_investigationName;
			std::string m_investigationAbstract;
			std::string m_sampleName;
			std::string m_investigatorSurname;
			std::string m_RbNumber;
			std::string m_investigationType;
			std::string m_datafileName;

		};
		
}
		
}
		
#endif