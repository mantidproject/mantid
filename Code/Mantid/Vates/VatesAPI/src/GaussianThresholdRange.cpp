#include "MantidVatesAPI/GaussianThresholdRange.h"
#include "MantidAPI/IMDIterator.h"
#include <cmath>


namespace Mantid
{
  namespace VATES
  {

    /**
    Constructor
    @parameter workspace : Input workspace to analyse
    @parameter preferred_nStd : number of standard deviations to use when extracting upper and lower signal values.
    @parameter sampleSize : How many cells to consider in the analysis.
    */
    GaussianThresholdRange::GaussianThresholdRange(Mantid::API::IMDWorkspace_sptr workspace, signal_t preferred_nStd, size_t sampleSize) : m_workspace(workspace), m_min(0), m_max(0), m_isCalculated(false), m_preferred_nStd(preferred_nStd), m_sampleSize(sampleSize)
    {
    }

    /**
    Constructor
    @parameter preferred_nStd : number of standard deviations to use when extracting upper and lower signal values.
    @parameter sampleSize : How many cells to consider in the analysis.
    */
    GaussianThresholdRange::GaussianThresholdRange(signal_t preferred_nStd, size_t sampleSize) :  m_min(0), m_max(0), m_isCalculated(false), m_preferred_nStd(preferred_nStd), m_sampleSize(sampleSize)
    {
    }

    ///Destructor
    GaussianThresholdRange::~GaussianThresholdRange()
    {
    }

    /**
    Assumes a normal, completely symetrical distribution and calculates max and min values based on this.
    @parameter raw_values : collection of cell/signal values to analyse.
    @parameter size : Number of cells/signals being analysed.
    @parameter max_signal : Maximum signal value.
    @parameter min_signal : Minimum signal value
    @parameter accumulated_signal : sum of all signal values.
    */
    void GaussianThresholdRange::calculateAsNormalDistrib(std::vector<signal_t>& raw_values, size_t size, signal_t max_signal, signal_t min_signal, signal_t accumulated_signal)
    {

      signal_t mean = accumulated_signal / static_cast<signal_t>( size );
      signal_t sum_sq_diff = 0;
      for(size_t i = 0; i < size; i++)
      {
        sum_sq_diff += (raw_values[i] - mean) * (raw_values[i] - mean);
      }
      signal_t sdev = std::sqrt(sum_sq_diff/ static_cast<signal_t>( size ));
      signal_t distribution_center = (max_signal - min_signal) / 2;
      signal_t proposed_max = distribution_center + min_signal + (sdev * m_preferred_nStd);
      signal_t proposed_min = distribution_center + min_signal - (sdev * m_preferred_nStd);

      //If there is a tight distribution, and n * sigma gives a greater range than just using min_max, we need to correct.
      m_max = proposed_max > max_signal ? max_signal : proposed_max;
      m_min = proposed_min < min_signal ? min_signal : proposed_min;
      
    }

    /**
    Overriden calculate method. Directs calculation of max and min values based on a normal distribution.
    */
    void GaussianThresholdRange::calculate()
    {
      if(NULL == m_workspace.get())
      {
        throw std::logic_error("The workspace has not been set.");
      }
      Mantid::API::IMDIterator* it = m_workspace->createIterator();
      std::vector<signal_t> raw_values;
      signal_t signal = 0;
      signal_t accumulated_signal = 0;
      signal_t max_signal = it->getNormalizedSignal();
      signal_t min_signal = max_signal;
      size_t size = 0;
      size_t nSkips = 1;
      if(m_sampleSize > 0)
      {
        size_t interval = it->getDataSize()/m_sampleSize; //Integer division
        nSkips = interval > 0 ? interval : 1; //nSkips = 1 minimum
      }
      do
      {
        signal = it->getNormalizedSignal();
        if(signal != 0) //Cells with zero signal values are not considered in the analysis.
        {
          accumulated_signal += signal;
          raw_values.push_back(signal);
          max_signal = signal > max_signal  ? signal : max_signal;
          min_signal = signal < min_signal ? signal : min_signal;
          size++;
        }
      } while (it->next(nSkips));

      calculateAsNormalDistrib(raw_values, size, max_signal, min_signal, accumulated_signal);
      m_isCalculated = true;
      delete it;
    }

    /**
    Indicates wheter execution has occured or not.
    @return : true if ::calculate() has been called previously, otherwise false.
    */
    bool GaussianThresholdRange::hasCalculated() const
    {
      return m_isCalculated;
    }

    /**
    Getter for the calculated minimum value.
    */
    signal_t GaussianThresholdRange::getMinimum() const
    {
      if(!m_isCalculated)
      {
        throw std::runtime_error("Cannot call ::getMinimum() without first calling ::calculate()");
      }
      return m_min;
    }

    /**
    Getter for the calculated maximum value.
    */
    signal_t GaussianThresholdRange::getMaximum() const
    {
      if(!m_isCalculated)
      {
        throw std::runtime_error("Cannot call ::getMaximum() without first calling ::calculate()");
      }
      return m_max;
    }

    /**
    Virtual constructor clone method.
    @return clone as GaussianThresholdRange*.
    */
    GaussianThresholdRange* GaussianThresholdRange::clone() const
    {
      return new GaussianThresholdRange(this->m_workspace, m_preferred_nStd, m_sampleSize);
    }

    /**
    Setter for IMDWorkspace.
    @parameter: workspace : The workspace to extract ranges from.
    */
    void GaussianThresholdRange::setWorkspace(Mantid::API::Workspace_sptr workspace)
    {
      m_isCalculated = false;
      m_workspace = boost::shared_dynamic_cast<Mantid::API::IMDWorkspace>(workspace);
      if(!workspace)
      {
        throw std::logic_error("GaussianThresholdRange only works for IMDWorkspaces");
      }
    }

    /**
    Determine whether the signal is withing range.
    @parameter signal value
    @return true if the signal is in the range defined by this object.
    */
    bool GaussianThresholdRange::inRange(const signal_t& signal)
    {
      return signal >= m_min && signal <= m_max;
    }
  }
}
