#ifndef MANTID_API_ALGORITHMPROXY_H_
#define MANTID_API_ALGORITHMPROXY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IAlgorithm.h"
#include "MantidKernel/PropertyManagerOwner.h"
#include <boost/shared_ptr.hpp>
#include <Poco/ActiveMethod.h>

#ifdef _MSC_VER
  #pragma warning( disable: 4250 ) // Disable warning regarding inheritance via dominance, we have no way around it with the design
#endif

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "unknown"
#endif

namespace Poco
{
  class Void;
}

namespace Mantid
{
  namespace API
  {
    class Algorithm;
    typedef boost::shared_ptr<Algorithm> Algorithm_sptr;

    /**

    A proxy class that stands between the user and the actual algorithm. 
    AlgorithmDataService stores algoruithm proxies. Actual algorithms are 
    created by the proxy and destroyed after execution to free memory. 
    Algorithm and its proxy share all properties.

    @author Russell Taylor, Tessella Support Services plc
    @author Based on the Gaudi class of the same name (see http://proj-gaudi.web.cern.ch/proj-gaudi/)
    @date 12/09/2007
    @author Roman Tolchenov, Tessella plc
    @date 03/03/2009

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport AlgorithmProxy : public IAlgorithm, public Kernel::PropertyManagerOwner
    {
    public:
      AlgorithmProxy(Algorithm_sptr alg);
      virtual ~AlgorithmProxy();

      ///The name of the algorithm
      const std::string name() const {return m_name;}
      /// The version of the algorithm
      int version() const  {return m_version;}
      /// The category of the algorithm
      const std::string category() const {return m_category;}
      /// Aliases to the algorithm
      const std::string alias() const {return m_alias;}

      /// The algorithmID
      AlgorithmID getAlgorithmID() const {return AlgorithmID(this);}

      virtual const std::string getOptionalMessage() const { return m_OptionalMessage; }
      virtual const std::string getWikiSummary() const { return m_WikiSummary; }
      virtual const std::string getWikiDescription() const { return m_WikiDescription; }

      void initialize();
      bool execute();
      void executeAsSubAlg() { throw std::runtime_error("Not implemented."); }
      Poco::ActiveResult<bool> executeAsync();
      bool isInitialized() const;
      bool isExecuted() const;

      /// To query whether algorithm is a child. A proxy is always at top level, returns false
      bool isChild() const {return false;}
      void setChild(const bool) {} ///< Do nothing
      void setRethrows(const bool rethrow);

      /** @name PropertyManager methods */
      //@{
      /// Set the property value
      void setPropertyValue(const std::string& name, const std::string &value);
      //@}

      void cancel() const;
      bool isRunningAsync();
      bool isRunning();

      void addObserver(const Poco::AbstractObserver& observer)const;
      void removeObserver(const Poco::AbstractObserver& observer)const;

      ///Set logging on or off
      ///@param value :: true = logging enabled
      void setLogging(const bool value) { m_isLoggingEnabled=value; }
      /// Is the algorithm have logging enabled
      bool isLogging() const { return m_isLoggingEnabled; }

      ///setting the child start progress
      void setChildStartProgress(const double startProgress)const;
      /// setting the child end progress
      void setChildEndProgress(const double endProgress)const;

      /** @name String serialization */
      //@{
      /// Serialize an object to a string
      virtual std::string toString(bool withDefaultValues = false) const;
      //@}
      
    private:
      /// Private Copy constructor: NO COPY ALLOWED
      AlgorithmProxy(const AlgorithmProxy&);
      /// Private assignment operator: NO ASSIGNMENT ALLOWED
      AlgorithmProxy& operator=(const AlgorithmProxy&);

      void createConcreteAlg(bool initOnly = false);
      void stopped();
      void addObservers();

      /// Poco::ActiveMethod used to implement asynchronous execution.
      Poco::ActiveMethod<bool, Poco::Void, AlgorithmProxy> _executeAsync;
      /// Execute asynchronous implementation
      bool executeAsyncImpl(const Poco::Void & dummy);

      const std::string m_name;     ///< name of the real algorithm
      const std::string m_category; ///< category of the real algorithm
      const std::string m_alias;    ///< alias to the algorithm
      std::string m_OptionalMessage; ///<Message to display in GUI
      std::string m_WikiSummary; ///< A summary line for the wiki page.
      std::string m_WikiDescription; ///< Description in the wiki page.
      const int m_version;          ///< version of the real algorithm

      mutable boost::shared_ptr<Algorithm> m_alg;  ///< Shared pointer to a real algorithm. Created on demand
      bool m_isExecuted;     ///< Executed flag
      bool m_isLoggingEnabled;///< is the logging of the underlying algorithm enabled
      bool m_rethrow; ///< Whether or not to rethrow exceptions.

      /// Temporary holder of external observers wishing to subscribe
      mutable std::vector<const Poco::AbstractObserver*> m_externalObservers;

      /// Static refenence to the logger class
      static Kernel::Logger& g_log;
    };

  } // namespace API
} // namespace Mantid

#endif /*MANTID_API_ALGORITHMPROXY_H_*/
