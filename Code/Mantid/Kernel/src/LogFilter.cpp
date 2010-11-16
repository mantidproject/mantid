#include "MantidKernel/LogFilter.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <stdexcept>

namespace Mantid
{
namespace Kernel
{

//-------------------------------------------------------------------------------------------------
/** Constructor
    @param tsp Pointer to property to be filtered. Its actual type must be TimeSeriesProperty<double>
 */
LogFilter::LogFilter(const Property* tsp)
{
    const TimeSeriesProperty<double>* ind = dynamic_cast<const TimeSeriesProperty<double>*>(tsp);
    if (ind)
    {
        m_prop.reset(static_cast<TimeSeriesProperty<double>*>(ind->clone()));
        return;
    }

    const TimeSeriesProperty<int>* ini = dynamic_cast<const TimeSeriesProperty<int>*>(tsp);
    if (ini)
    {
        TimeSeriesProperty<double>* p = new TimeSeriesProperty<double>("tmp");
        std::map<DateAndTime, int> pmap = ini->valueAsMap();
        for(std::map<DateAndTime, int>::iterator it = pmap.begin();it!=pmap.end();it++)
            p->addValue(it->first,double(it->second));
        m_prop.reset(p);
        return;
    }

    const TimeSeriesProperty<bool>* inb = dynamic_cast<const TimeSeriesProperty<bool>*>(tsp);
    if (inb)
    {
        TimeSeriesProperty<double>* p = new TimeSeriesProperty<double>("tmp");
        std::map<DateAndTime, bool> pmap = inb->valueAsMap();
        for(std::map<DateAndTime, bool>::iterator it = pmap.begin();it!=pmap.end();it++)
            p->addValue(it->first,double(it->second));
        m_prop.reset(p);
        return;
    }

    throw std::runtime_error("Cannot cast to TimeSeriesProperty<double>");
}


//-------------------------------------------------------------------------------------------------
/**  Filter using a TimeSeriesProperty<bool>. True values mark the allowed time intervals.
     @param filter Filtering mask
 */
void LogFilter::addFilter(const TimeSeriesProperty<bool>* filter)
{
    if (filter->size() == 0) return;
    if (!m_filter || m_filter->size() == 0) m_filter.reset(filter->clone());
    else
    {
        TimeSeriesProperty<bool>* f = new TimeSeriesProperty<bool>("tmp");

        TimeSeriesProperty<bool>* f1 = m_filter.get();
        TimeSeriesProperty<bool>* f2 = filter->clone();

        TimeInterval t1 = f1->nthInterval(f1->size()-1);
        TimeInterval t2 = f2->nthInterval(f2->size()-1);

        if (t1.begin() < t2.begin())
        {
            f1->addValue(t2.begin(),true);// should be f1->lastValue, but it doesnt matter for boolean AND
        }
        else if  (t2.begin() < t1.begin())
        {
            f2->addValue(t1.begin(),true);
        }

        int i = 0;
        int j = 0;

        t1 = f1->nthInterval(i);
        t2 = f2->nthInterval(j);

        // Make the two filters start at the same time. An entry is added at the beginning
        // of the filter that starts later to equalise their staring times. The new interval will have
        // value opposite to the one it started with originally.
        if (t1.begin() > t2.begin())
        {
            f1->addValue(t2.begin(),!f1->nthValue(i));
            t1 = f1->nthInterval(i);
        }
        else if  (t2.begin() > t1.begin())
        {
            f2->addValue(t1.begin(),!f2->nthValue(j));
            t2 = f2->nthInterval(j);
        }

        for(;;)
        {
            TimeInterval t;
            t = t1.intersection(t2);
            if (t.isValid())
            {
                f->addValue(t.begin(),  (f1->nthValue(i) && f2->nthValue(j)) );
            }

            if (t1.end() < t2.end())
            {
                i++;
            }
            else if (t2.end() < t1.end())
            {
                j++;
            }
            else
            {
                i++;
                j++;
            }

            if (i == f1->size() || j == f2->size()) break;
            t1 = f1->nthInterval(i);
            t2 = f2->nthInterval(j);
        }

        delete f2;
        f->clearFilter();
        m_filter.reset(f);
    }
    m_prop->clearFilter();
    m_prop->filterWith(m_filter.get());
}


//-------------------------------------------------------------------------------------------------
/// Clears filters
void LogFilter::clear()
{
    m_prop->clearFilter();
    m_filter.reset();
}

} // namespace Kernel
} // namespace Mantid

