#ifndef MANTID_LIVEDATA_ISISHISTODATALISTENER_H_
#define MANTID_LIVEDATA_ISISHISTODATALISTENER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ILiveListener.h"
#include "MantidKernel/cow_ptr.h"

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------

struct idc_info;
typedef struct idc_info* idc_handle_t;

namespace Mantid
{
  //----------------------------------------------------------------------
  // Forward declarations
  //----------------------------------------------------------------------
  namespace API
  {
    class MatrixWorkspace;
  }

  namespace LiveData
  {
    /** ILiveListener is the interface implemented by classes which connect directly to
        instrument data acquisition systems (DAS) for retrieval of 'live' data into Mantid.

        Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
     */
    class ISISHistoDataListener : public API::ILiveListener
    {
    public:
      ISISHistoDataListener();
      ~ISISHistoDataListener();

      std::string name() const { return "ISISHistoDataListener"; }
      bool supportsHistory() const { return false; }
      bool buffersEvents() const { return false; }

      bool connect(const Poco::Net::SocketAddress& address);
      void start(Kernel::DateAndTime startTime = Kernel::DateAndTime());
      boost::shared_ptr<API::Workspace> extractData();

      bool isConnected();
      ILiveListener::RunStatus runStatus();
      int runNumber() const;

    private:

      void setSpectra(const std::vector<specid_t>& specList);
      void setPeriods(const std::vector<specid_t>& periodList);
      int getInt(const std::string& par) const;
      std::string getString(const std::string& par) const;
      void getFloatArray(const std::string& par, std::vector<float>& arr, const size_t dim);
      void getIntArray(const std::string& par, std::vector<int>& arr, const size_t dim);
      void getData(int period, int index, int count, boost::shared_ptr<API::MatrixWorkspace> workspace, size_t workspaceIndex);
      void calculateIndicesForReading(std::vector<int>& index, std::vector<int>& count);
      void loadSpectraMap();
      void runLoadInstrument(boost::shared_ptr<API::MatrixWorkspace> localWorkspace, const std::string& iName);
      void loadTimeRegimes();
      int getTimeRegimeToLoad() const;
      bool isPeriodIgnored(int period) const;
      static double dblSqrt(double in);

      /// is initialized
      bool isInitilized;

      /// the DAE name
      std::string m_daeName;

      /// the DAE handle
      idc_handle_t m_daeHandle;

      /// number of periods
      int m_numberOfPeriods;

      /// number of spectra for each time regime
      std::vector<int> m_numberOfSpectra;

      /// total number of spectra
      int m_totalNumberOfSpectra;

      /// number of bins for each time regime
      std::vector<int> m_numberOfBins;

      /// list of spectra to read or empty to read all
      std::vector<specid_t> m_specList;

      /// list of periods to read or empty to read all
      std::vector<specid_t> m_periodList;

      /// Store the bin boundaries for each time regime
      std::vector<boost::shared_ptr<MantidVec>> m_bins;

      /// Detector IDs
      std::vector<int> m_detIDs;

      /// Spectra IDs
      std::vector<int> m_specIDs;

      /// Monitor spectra
      std::vector<int> m_monitorSpectra;

      /// Time regime to load
      int m_timeRegime;

      /// reporter function called when the IDC reading routines raise an error
      static void IDCReporter(int status, int code, const char* message);
    };

  } // namespace LiveData
} // namespace Mantid

#endif  /* MANTID_LIVEDATA_ISISHISTODATALISTENER_H_ */
