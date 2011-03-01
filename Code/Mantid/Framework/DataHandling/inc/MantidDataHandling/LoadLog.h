#ifndef MANTID_DATAHANDLING_LOADLOG_H_
#define MANTID_DATAHANDLING_LOADLOG_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
        
  namespace DataHandling
  {
    /** @class LoadLog LoadLog.h DataHandling/LoadLog.h

	Load ISIS log file(s). Assumes that a log file originates from a 
	PC (not VMS) environment, i.e. the log files to be loaded are assumed
	to have the extension .txt. Its filename is assumed to starts with the raw data 
	file identifier followed by the character '_', and a log file is assumed to have a 
	format of two columns, where the first column consists of data-time strings of the 
	ISO 8601 form and the second column consists of either numbers or strings that may 
	contain spaces.

	The algoritm requires an input filename. If this filename is the name of a
	raw datafile the algorithm will attempt to read in all the log files associated
	with that log file. Otherwise it will assume the filename specified is the 
	filename of a specific log file. 

	LoadLog is an algorithm and as such inherits from the Algorithm class, 
	via DataHandlingCommand, and overrides the init() & exec() methods.
	LoadLog is intended to be used as a child algorithm of 
	other Loadxxx algorithms, rather than being used directly.

	Required Properties:
	<UL>
	<LI> Filename - The filename (including its full or relative path) of either an ISIS log file 
	or an ISIS raw file. If a raw file is specified all log files associated with that raw file 
	are loaded into the specified workspace. The file extension must either be .raw or .s when 
	specifying a raw file, and at least 10 characters long. </LI>
	<LI> Workspace - The workspace to which the log data is appended </LI>
	</UL>

	@author Anders Markvardsen, ISIS, RAL
	@date 26/09/2007

	Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory


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
    class DLLExport LoadLog : public API::Algorithm
    {
    public:
      /// Default constructor
      LoadLog();

      /// Destructor
      ~LoadLog() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadLog";};
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;};
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling\\Logs";}

      /// this method returns data periods property and useful for loading log data for multi period files
      const boost::shared_ptr<Kernel::Property> getPeriodsProperty() const {return m_periods;}

    private:

      /// Overwrites Algorithm method.
      void init();

      /// SNS text
      bool LoadSNSText();

      /// Overwrites Algorithm method
      void exec();

      /// The name and path of an input file. This may be the filename of a raw datafile or the name of a specific log file.
      std::string m_filename;

      /// type returned by classify
      enum kind { empty, string, number };

      /// Takes as input a string and try to determine what type it is
      kind classify(const std::string& s) const;

      /// convert string to lower case
      std::string stringToLower(std::string strToConvert);

      /// Checks if the file is an ASCII file
      bool isAscii(const std::string& filenamePart);

      /// check if first 19 characters of a string is data-time string according to yyyy-mm-ddThh:mm:ss
      bool isDateTimeString(const std::string& str) const;
      /// Return the name of the three column log file for associated with the specified file. Empty string if one doesn't exist
      std::string getThreeColumnName() const;
      /// Check for SNS-style text file
      bool SNSTextFormatColumns(const std::string& str, std::vector<double> & out) const;

      /// create timeseries property from .log file and adds taht to sample object
      std::set<std::string> createthreecolumnFileLogProperty(const std::string& logfile, API::Run& run);
         
      /// if a file with the second column(block column) name in .log file exists
      /// in the raw file directory
      bool blockcolumnFileExists(const std::string& fileName);
      /// if  alternate data stream named checksum exists for the raw file
      bool adsExists();
      /// returns the list of log files from ADS checksum
      std::set<std::string> getLogfilenamesfromADS();

      /// TimeSeriesProperty<int> containing data periods. Created by LogParser
      boost::shared_ptr<Kernel::Property> m_periods;
    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADLOG_H_*/
