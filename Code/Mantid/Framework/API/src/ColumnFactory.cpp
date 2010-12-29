#include <iostream>
#include <sstream>
#include "MantidAPI/ColumnFactory.h"
#include "MantidAPI/Column.h"
#include "MantidKernel/Logger.h"
namespace Mantid
{
namespace API
{

    ColumnFactoryImpl::ColumnFactoryImpl() : Kernel::DynamicFactory<API::Column>(), g_log(Kernel::Logger::get("ColumnFactory"))
	{
	}

	ColumnFactoryImpl::~ColumnFactoryImpl()
	{
	}

    boost::shared_ptr<Column> ColumnFactoryImpl::create(const std::string& type) const
    {
        boost::shared_ptr<Column> c = Kernel::DynamicFactory<Column>::create(type);
        c->m_type = type;
        return c;
    }


} // namespace DataObjects
} // namespace Mantid
