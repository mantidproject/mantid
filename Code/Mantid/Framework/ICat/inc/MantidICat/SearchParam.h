#ifndef MANTID_ICAT_SAERCHPARAM_H_
#define MANTID_ICAT_SAERCHPARAM_H_

#include <string>
#include <stdexcept>

namespace Mantid
{
	namespace ICat
	{
    
  /** This class is used in ICat Search service to set all the inputs for search.
      This file is part of Mantid.

      Mantid is free software; you can redistribute it and/or modify
      it under the terms of the GNU General Public License as published by
      the Free Software Foundation; either version 3 of the License, or
      (at your option) any later version.

      Mantid is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
      GNU General Public License for more details.

      You should have received a copy of the GNU General Public License
      along with this program.  If not, see <http://www.gnu.org/licenses/>.

      File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
      Code Documentation is available at: <http://doxygen.mantidproject.org>
    
	*/	
    class CSearchParam
		{
		public:
			/// constructor
			CSearchParam();
			/// Destructor
			~CSearchParam();
			/**This method  sets start date
        *@param startRun :: start run number
			 */
			void setRunStart(const double& startRun);
			/**This method  sets end date
        *@param endRun :: end run number
			 */
			void setRunEnd(const double& endRun);
			/**This method  sets isntrument name
        *@param instrName :: name of the instrument
			 */
			void setInstrument(const std::string& instrName);
			/**This method  sets the start date
        *@param startDate :: start date for search
			 */
			void setStartDate(const time_t& startDate);
			/**This method  sets the end date
        *@param endDate :: end date for search
			 */
			void setEndDate(const time_t& endDate);
			/**This method  sets the CaseSensitive
        *@param bCase :: flag to do case sensitive  search
			 */
			void setCaseSensitive(bool bCase);
			
			/**This method  sets the keywords
        *@param keywords :: the key words used for searching investigations 
			 */
			void setKeywords(const std::string& keywords);

			/**This method  sets investigationName used for searching
        *@param instName ::  investigation name
			 */
			void  setInvestigationName(const std::string& instName);

			/**This method  sets investigationAbstract used for searching
        *@param invstabstract :: investigation abstract
			 */
			 void setInvestigationAbstract(const std::string& invstabstract);

			/**This method  sets sample used for searching
        *@param sampleName :: name of the sample 
			 */
			void setSampleName(const std::string& sampleName);

			/**This method  sets Investigator surname
        *@param investigatorName :: surname of the investigator
			 */
			void  setInvestigatorSurName(const std::string& investigatorName);

			/** This method  sets Rb Number
        *@param RbNumber :: Rb number used for search
			 */
			 void setRbNumber(const std::string& RbNumber);

			/**This method  sets Investigation Type
        *@param invstType :: investigation type used for search
			 */
			 void setInvestigationType(const std::string& invstType);

			/**This method  sets datafileName
        *@param datafileName :: data file name used for search
			 */
			void setDatafileName(const std::string& datafileName );

			/**This method  returns the start run number
        *@returns  run start number
			 */
			const double& getRunStart()const  ;
			/**This method  returns the end run number
        *@returns  run end number
			 */
			const double& getRunEnd() const;
			/**This method  returns the instrument name
        *@returns  instrument name
			 */
			const std::string& getInstrument() const;
			/**This method  returns the start date
        *@returns  start date
			 */
       const time_t& getStartDate() const;

			/**This method  returns the end date
        *@returns end date for investigations serch
			 */
			 const time_t& getEndDate() const;
			/**This method  returns case sensitive flag
        *@returns  case sensitive flag
			 */
			bool getCaseSensitive() const;
			
			/**This method  returns keywords used for searching
        *@returns keywords
			 */
			const std::string& getKeywords() const;

			/**This method  returns investigationName used for searching
        *@returns investigation name
			 */
			const std::string& getInvestigationName() const;

			/**This method  returns investigationAbstract used for searching
        *@returns investigation abstract
			 */
			const std::string& getInvestigationAbstract() const;

			/**This method  returns sample used for searching
        *@returns samplename
			 */
			const std::string& getSampleName() const;

			/**This method  returns Investigator surname
        *@returns surname of the investigator
			 */
			const std::string& getInvestigatorSurName() const;

			/**This method  returns Rb Number
        *@returns Rb number
			 */
			const std::string& getRbNumber() const;

			/**This method  returns Investigation Type
        *@returns Rb number
			 */
			 const std::string& getInvestigationType() const;

			/**This method  returns datafileName
        *@returns m_datafileName
			 */
			 const std::string& getDatafileName() const;

			/**This method returns the time_t value for a Date which is in "DD/MM/YYYY" format
        *@param sDate :: input date string
      */
			time_t getTimevalue(const std::string& sDate);

		private:
			double m_startRun;
			double m_endRun;
			std::string m_instrName;
			std::string m_keywords;
			bool m_caseSensitive;
			time_t m_startDate;
			time_t m_endDate;
			
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
