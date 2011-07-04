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
    @preferred_nStd : number of standard deviations to use when extracting upper and lower signal values.
    @skipN : of n-cells, how many to skip before using the next as part of the anaysis (for speed-up).
    */
    GaussianThresholdRange::GaussianThresholdRange(Mantid::API::IMDWorkspace_sptr workspace, signal_t preferred_nStd, unsigned int skipN) : m_workspace(workspace), m_min(0), m_max(0), m_isCalculated(false), m_preferred_nStd(preferred_nStd), m_skipN(skipN)
    {
    }

    ///Destructor
    GaussianThresholdRange::~GaussianThresholdRange()
    {
    }

    /**
    Assumes a normal, completely symetrical distribution and calculates max and min values based on this.
    @raw_values : collection of cell/signal values to analyse.
    @size : Number of cells/signals being analysed.
    @max_signal : Maximum signal value.
    @min_signal : Minimum signal value
    @accumulated_signal : sum of all signal values.
    */
    void GaussianThresholdRange::calculateAsNormalDistrib(std::vector<signal_t>& raw_values, unsigned int size, signal_t max_signal, signal_t min_signal, signal_t accumulated_signal)
    {

      signal_t mean = accumulated_signal / size;
      signal_t sum_sq_diff = 0;
      for(int i = 0; i < size; i++)
      {
        sum_sq_diff += (raw_values[i] - mean) * (raw_values[i] - mean);
      }
      signal_t sdev = std::sqrt(sum_sq_diff/ size );
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
      Mantid::API::IMDIterator* it = m_workspace->createIterator();
      std::vector<signal_t> raw_values;
      signal_t signal = 0;
      signal_t accumulated_signal = 0;
      signal_t max_signal = m_workspace->getCell(0).getSignal();
      signal_t min_signal = m_workspace->getCell(0).getSignal();
      unsigned int size = 0;
      for(int i =0; ;i++)
      {
        if(it->next())
        {
          size_t pos = it->getPointer();
          signal = m_workspace->getCell(pos).getSignal();
          accumulated_signal += signal;
          raw_values.push_back(signal);
          max_signal = signal > max_signal  ? signal : max_signal;
          min_signal = signal < min_signal ? signal : min_signal;
          size++;
        }
        else
        {
          break;
        }
        for(unsigned int j = 0; j < m_skipN; j++)
        {
          if(j < m_skipN)
          {
            it->next();
          }
        }
      }
      calculateAsNormalDistrib(raw_values, size, max_signal, min_signal, accumulated_signal);
      
      m_isCalculated = true;
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
  }
}