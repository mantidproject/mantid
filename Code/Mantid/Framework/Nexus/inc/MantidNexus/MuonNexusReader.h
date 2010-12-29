#ifndef MUONNEXUSREADER_H
#define MUONNEXUSREADER_H

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include <napi.h>
#include "boost/date_time/posix_time/posix_time.hpp"
#include <limits.h>


// class MuonNexusReader - based on ISISRAW this class implements a simple
// reader for Nexus Muon data files.
class MuonNexusReader
{
    /** @class MuonNexusReader MuonNexusReader.h

    MuunNexusReader opens a Nexus file and reads certain fields expected for a ISIS Muon
	data file (old format). These values are stored for access via LoadMuonNexus.

    Required Properties:
    <UL>
    <LI> Filename - The name of and path to the input Nexus file </LI>
    </UL>

    @author Ronald Fowler, based on ISISRAW.
    @date 14/08/2008

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
	private:
		std::string nexus_instrument_name; ///< name read from nexus file
		std::string nexus_samplename; ///< sample name read from Nexus
        int nexusLogCount;  ///< number of NXlog sections read from file
		std::vector<bool> logType;  ///< true if i'th log is numeric
		std::vector<std::string> logNames;  ///< stores name read from file
		int readMuonLogData(NXhandle fileID);  ///< method to read the fields of open NXlog section
		std::vector< std::vector<float> > logValues, ///< array of values for i'th NXlog section
			                              logTimes;  ///< arrys of times for i'th NXlog section
		std::string startTime; ///< string startTime which must be read from Nexus file to base all NXlog times on
		std::time_t startTime_time_t; ///< startTime in time_t format
		std::time_t to_time_t(boost::posix_time::ptime t) ///< convert posix time to time_t
        {
			/*!
			Take the input Posix time, subtract the unix epoch, and return the seconds
			as a std::time_t value.
			@param t :: time of interest as ptime
			@return :: time_t value of t
			*/
			if( t == boost::posix_time::neg_infin )
             return 0;
			else if( t == boost::posix_time::pos_infin )
             return LONG_MAX;
			boost::posix_time::ptime start(boost::gregorian::date(1970,1,1));
            return (t-start).total_seconds();
        } 

	public:
		/// Default constructor
		MuonNexusReader();
		/// Destructor
		~MuonNexusReader();

		int readFromFile(const std::string& filename); ///< read histogram data
		int readLogData(const std::string& filename);  ///< read log data
		int getTimeChannels(float* timechannels, const int& len) const; ///< get time bin boundaries
        /// return sample name
		std::string getSampleName() const {return nexus_samplename; };
		int numberOfLogs() const;  ///< Number of NXlog sections read from file
		int getLogLength(const int i) const;  ///< Lenght of i'th log
		std::string getLogName(const int i) const;  ///< Name of i'th log
        void getLogValues(const int& logNumber, const int& logSequence,
						  std::time_t& logTime, double& value);  ///< get logSequence pair of logNumber log
		bool logTypeNumeric(const int i) const;  ///< true if i'th log is of numeric type
		// following ISISRAW.h
		int t_nsp1;			///< number of spectra in time regime 1
		int t_ntc1;			///< number of time channels in time regime 1
		int t_nper;          ///< number of periods in file (=1 at present)
		// for nexus histogram data
		float* corrected_times; ///< temp store for corrected times
		int* counts;         ///< temp store of histogram data
		int* detectorGroupings; ///< detector grouping info
		int numDetectors; ///< detector count
		std::string getInstrumentName(); ///< return instrument name
};


#endif /* MUONNEXUSREADER_H */













