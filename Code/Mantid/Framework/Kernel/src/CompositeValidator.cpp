#include "MantidKernel/CompositeValidator.h"

using namespace Mantid::Kernel;

namespace Mantid
{
  namespace Kernel
  {
    /// Default constructor
    CompositeValidator::CompositeValidator()
     : IValidator(), m_children()
    {
    }

    /// Destructor
    CompositeValidator::~CompositeValidator()
    {
      m_children.clear();
    }

    /**
     * The allowed values for the composite vaidator. This returns
     * the intersection of the allowedValues for the child validators
     * @return
     */
    std::vector<std::string> CompositeValidator::allowedValues() const
    {
      std::set<std::string>      elem_unique;
      std::multiset<std::string> elem_all;
      // how many validators return non-empty list of allowed values
      int n_combinations(0);
      std::list<IValidator_sptr>::const_iterator itrEnd = m_children.end();
      for( std::list<IValidator_sptr>::const_iterator itr = m_children.begin(); itr != itrEnd; ++itr)
      {
        std::vector<std::string> subs = (*itr)->allowedValues();
        if ( subs.empty())continue;
        elem_unique.insert(subs.begin(),subs.end());
        elem_all.insert(subs.begin(),subs.end());
        n_combinations++;
      }
      // empty or single set of allowed values
      if(n_combinations<2)return std::vector<std::string>( elem_unique.begin(), elem_unique.end() );
      // there is more then one combination and we have to identify its union;
      for(std::set<std::string>::const_iterator its=elem_unique.begin();its!=elem_unique.end();++its)
      {
        std::multiset<std::string>::iterator im = elem_all.find(*its);
        elem_all.erase(im);
      }
      std::set<std::string> rez;
      for(std::multiset<std::string>::const_iterator im=elem_all.begin();im!=elem_all.end();++im)
      {
        rez.insert(*im);
      }
      return std::vector<std::string>( rez.begin(), rez.end() );
    }

    /**
     * Clone the validator
     * @return A newly constructed validator object. Each child is also cloned
     */
    Kernel::IValidator_sptr CompositeValidator::clone() const
    {
      boost::shared_ptr<CompositeValidator> copy = boost::make_shared<CompositeValidator>();
      std::list<IValidator_sptr>::const_iterator itrEnd = m_children.end();
      for(   std::list<IValidator_sptr>::const_iterator itr = m_children.begin(); itr != itrEnd; ++itr)
      {
        copy->add( (*itr)->clone() );
      }
      return copy;
    }

    /** Adds a validator to the group of validators to check
     *  @param child :: A pointer to the validator to add
     */
    void CompositeValidator::add(Kernel::IValidator_sptr child)
    {
      m_children.push_back(child);
    }

    /** Checks the value of all child validators. Fails if any child fails.
     *  @param value :: The workspace to test
     *  @return A user level description of the first problem it finds otherwise ""
     */
    std::string CompositeValidator::check( const boost::any& value ) const
    {
      std::list<IValidator_sptr>::const_iterator itrEnd = m_children.end();
      for(std::list<IValidator_sptr>::const_iterator itr = m_children.begin(); itr != itrEnd; ++itr)
      {
        std::string error = (*itr)->check(value);
        //exit on the first error, to avoid passing doing more tests on invalid objects that could fail
        if (error != "") return error;
      }
      //there were no errors
      return "";
    }



  } // namespace Mantid
} // namespace Kernel

