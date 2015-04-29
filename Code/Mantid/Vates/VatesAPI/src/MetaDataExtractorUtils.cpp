#include "MantidVatesAPI/MetaDataExtractorUtils.h"
#include <qwt_double_interval.h>
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/Logger.h"
#include "boost/pointer_cast.hpp"
#include <cfloat>

namespace Mantid
{
  namespace VATES
  {
      namespace
    {
        /// Static logger
        Kernel::Logger g_log("MetaDataExtractorUtils");
    }

    MetaDataExtractorUtils::MetaDataExtractorUtils():defaultMin(0.0), defaultMax(1.0)
    {
    }

    MetaDataExtractorUtils::~MetaDataExtractorUtils()
    {
    }

    /**
     * Extract the instrument information from the workspace. If there 
     * is more than one instrument involved, then extract the first instrument
     * from the list.
     * @param workspace Shared pointer to the workspace.
     * @returns The instrument name or an empty string. 
     */
    std::string MetaDataExtractorUtils::extractInstrument(Mantid::API::IMDWorkspace_sptr workspace)
    {
      Mantid::API::IMDEventWorkspace_sptr eventWorkspace = boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(workspace);
      Mantid::API::IMDHistoWorkspace_sptr histoWorkspace = boost::dynamic_pointer_cast<Mantid::API::IMDHistoWorkspace>(workspace);
    
      std::string instrument = "";

      // Check which workspace is currently used and that it contains at least one instrument.
      if (eventWorkspace)
      {
        if (eventWorkspace->getNumExperimentInfo() > 0)
        {
          instrument = eventWorkspace->getExperimentInfo(0)->getInstrument()->getName();
        } else 
        {
          g_log.notice() << "The event workspace does not have any instruments. \n";

          instrument = "";
        }
      } else if (histoWorkspace)
      {
        if (histoWorkspace->getNumExperimentInfo() > 0)
        {
          instrument = histoWorkspace->getExperimentInfo(0)->getInstrument()->getName();
        } else
        {
          g_log.notice() << "The histo workspace does not have any instruments. \n";

          instrument = "";
        }
      } else
      {
        g_log.warning() << "The workspace does not seem to be either event or histo. \n";
        instrument = "";
      }

      return instrument;
    }

    /**
     * Set the minimum and maximum of the workspace data. Code essentially copied from SignalRange.cpp
     * @param workspace Rreference to an IMD workspace
     * @returns The minimum and maximum value of the workspace dataset.
     */
    QwtDoubleInterval MetaDataExtractorUtils::getMinAndMax(Mantid::API::IMDWorkspace_sptr workspace)
    {
      if (!workspace)
        throw std::invalid_argument("The workspace is empty.");

      auto iterators = workspace->createIterators(PARALLEL_GET_MAX_THREADS, 0);

      std::vector<QwtDoubleInterval> intervals(iterators.size());
      // cppcheck-suppress syntaxError
      PRAGMA_OMP( parallel for schedule(dynamic, 1))
      for (int i=0; i < int(iterators.size()); i++)
      {
        Mantid::API::IMDIterator * it = iterators[i];

        QwtDoubleInterval range = this->getRange(it);
        intervals[i] = range;
        // don't delete iterator in parallel. MSVC doesn't like it
        // when the iterator points to a mock object.
      }

      // Combine the overall min/max
      double minSignal = DBL_MAX;
      double maxSignal = -DBL_MAX;

      auto inf = std::numeric_limits<double>::infinity();
      for (size_t i=0; i < iterators.size(); i++)
      {
        delete iterators[i];
        
        double signal;
        signal = intervals[i].minValue();
        if (signal != inf && signal < minSignal) minSignal = signal;

        signal = intervals[i].maxValue();
        if (signal != inf && signal > maxSignal) maxSignal = signal;
      }

      // Set the lowest element to the smallest non-zero element.
      if (minSignal == DBL_MAX)
      {
        minSignal = defaultMin;
        maxSignal = defaultMax;
      } 

      QwtDoubleInterval minMaxContainer;

      if (minSignal < maxSignal)
        minMaxContainer = QwtDoubleInterval(minSignal, maxSignal);
      else
      {
        if (minSignal != 0)
          // Possibly only one value in range
          minMaxContainer = QwtDoubleInterval(minSignal*0.5, minSignal*1.5);
        else
          // Other default value
          minMaxContainer = QwtDoubleInterval(defaultMin, defaultMax);
      }

      return minMaxContainer;
    }

    /**
     * Get the range of a workspace dataset for a single iterator. Code the same as in SignalRange.cpp
     * @param it :: IMDIterator of what to find
     * @returns the min/max range, or INFINITY if not found
     */
    QwtDoubleInterval MetaDataExtractorUtils::getRange(Mantid::API::IMDIterator * it)
    {
      if (!it)
        return QwtDoubleInterval(defaultMin, defaultMax);
      if (!it->valid())
        return QwtDoubleInterval(defaultMin, defaultMax);

      // Use no normalization
      it->setNormalization(Mantid::API::VolumeNormalization);
     
      double minSignal = DBL_MAX;
      double minSignalZeroCheck = DBL_MAX;
      double maxSignal = -DBL_MAX;
      auto inf = std::numeric_limits<double>::infinity();

      do
      {
        double signal = it->getNormalizedSignal();
      
        // Skip any 'infs' as it screws up the color scale
        if (signal != inf)
        {
          if (signal == 0.0) minSignalZeroCheck = signal;
          if (signal < minSignal && signal >0.0) minSignal = signal;
          if (signal > maxSignal) maxSignal = signal;
        }
      } while (it->next());

      if (minSignal == DBL_MAX)
      {
        if (minSignalZeroCheck != DBL_MAX)
        {
          minSignal = defaultMin;
          maxSignal = defaultMax;
        }
        else
        {
          minSignal = inf;
          maxSignal = inf;
        }
      }
      return QwtDoubleInterval(minSignal, maxSignal);
    }
  }
}
