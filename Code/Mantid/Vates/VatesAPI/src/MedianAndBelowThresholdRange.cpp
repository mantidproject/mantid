#include "MantidVatesAPI/MedianAndBelowThresholdRange.h"
#include "MantidAPI/IMDIterator.h"
#include <cmath>


namespace Mantid
{
  namespace VATES
  {

    /**
    Constructor
    */
    MedianAndBelowThresholdRange::MedianAndBelowThresholdRange() : m_min(0.00), m_max(0), m_isCalculated(false)
    {
    }

    /**
    Constructor
    */
    MedianAndBelowThresholdRange::MedianAndBelowThresholdRange(signal_t min, signal_t max, bool isCalculated, Mantid::API::IMDWorkspace_sptr workspace) : m_min(min), m_max(max),  m_isCalculated(isCalculated), m_workspace(workspace)
    {
    }

    ///Destructor
    MedianAndBelowThresholdRange::~MedianAndBelowThresholdRange()
    {
    }


    /**
    Overriden calculate method. 
    */
    void MedianAndBelowThresholdRange::calculate()
    {
      if(NULL == m_workspace.get())
      {
        throw std::logic_error("The workspace has not been set.");
      }
      
      signal_t signal = 0;
      size_t pos = 0;
      signal_t accumulated_signal = 0;

      try
      {
      Mantid::API::IMDIterator* it = m_workspace->createIterator();
      while(it->next())
      {
        pos = it->getPointer();
        signal = m_workspace->getCell(pos).getSignal();
        accumulated_signal += signal;
        m_min = signal < m_min ? signal : m_min;
      }
      m_max = accumulated_signal / static_cast<signal_t>(it->getDataSize());
      }
      catch(Kernel::Exception::NotImplementedError&) // Really don't want to do this, but some IMDWorkspace don't implement createIterator!
      {
        uint64_t size = m_workspace->getNPoints();
        for(uint64_t i = 0; i < size; i++)
        {
          signal = m_workspace->getSignalAt(i);
          accumulated_signal += signal;
          m_min = signal < m_min ? signal : m_min;
        }
        m_max = accumulated_signal / static_cast<signal_t>(size);
      }
      m_isCalculated = true;
    }

    /**
    Indicates wheter execution has occured or not.
    @return : true if ::calculate() has been called previously, otherwise false.
    */
    bool MedianAndBelowThresholdRange::hasCalculated() const
    {
      return m_isCalculated;
    }

    /**
    Getter for the calculated minimum value.
    */
    signal_t MedianAndBelowThresholdRange::getMinimum() const
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
    signal_t MedianAndBelowThresholdRange::getMaximum() const
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
    MedianAndBelowThresholdRange* MedianAndBelowThresholdRange::clone() const
    {
      return new MedianAndBelowThresholdRange(m_min, m_max, m_isCalculated, this->m_workspace);
    }

    /**
    Setter for IMDWorkspace.
    @parameter: workspace : The workspace to extract ranges from.
    */
    void MedianAndBelowThresholdRange::setWorkspace(Mantid::API::IMDWorkspace_sptr workspace)
    {
      m_isCalculated = false;
      m_workspace = workspace;
    }

    /**
    Determine whether the signal is withing range.
    @parameter signal value
    @return true if the signal is in the range defined by this object.
    */
    bool MedianAndBelowThresholdRange::inRange(const signal_t& signal)
    {
      return signal != 0 && signal < m_max;
    }
  }
}