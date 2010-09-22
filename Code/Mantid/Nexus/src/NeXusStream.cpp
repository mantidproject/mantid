//
//  NeXus - Neutron & X-ray Common Data Format
//  
//  $Id: Makefile.am 598 2005-08-19 16:19:15Z faa59 $
//  
//  IOStream like interface to NeXus C++ Bindings
//
//  Copyright (C) 2008 Freddie Akeroyd, STFC ISIS facility
//  
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
// 
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
// 
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free 
//  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
//  MA  02111-1307  USA
//             
//  For further information, see http://www.nexusformat.org/
//

/////////////////// Subversion Repository Details ////////////////////////
// Repository Location     $HeadURL: http://svn.nexusformat.org/code/trunk/bindings/cpp/NeXusStream.cpp $
// Revision of last commit $LastChangedRevision: 1259 $ 
// Date of last commit     $LastChangedDate: 2009-05-08 15:29:28 -0400 (Fri, 08 May 2009) $
// Last changed by         $LastChangedBy: Freddie Akeroyd $
//////////////////////////////////////////////////////////////////////////

/**
 * \file NeXusStream.cpp
 * Implementation of IOStream like interface to NeXus files
 * \author Freddie Akeroyd, STFC ISIS Facility, GB
 * \version $LastChangedRevision: 1259 $
 * \date    $LastChangedDate: 2009-05-08 15:29:28 -0400 (Fri, 08 May 2009) $
 */

#include <iostream>
//#include "MantidNexus/napiconfig.h"
#include "MantidNexus/NeXusStream.hpp"
#include "MantidNexus/NeXusException.hpp"
namespace NeXusAPI {
namespace Stream {

HolderBase::HolderBase(const std::string& name) : m_name(name)
{
}

void HolderBase::setName(const std::string& name)
{
    m_name = name;
}
    
template<typename NumT>
AttrHolder<NumT>::AttrHolder(const std::string& name, NumT& value) : HolderBase(name), m_c_value(NULL), m_value(&value)
{
}

template<typename NumT>
AttrHolder<NumT>::AttrHolder(const std::string& name, const NumT& value) : HolderBase(name), m_c_value(&value), m_value(NULL)
{
}

template<typename NumT>
AttrHolder<NumT>::AttrHolder(NumT& value) : HolderBase(""), m_c_value(NULL), m_value(&value)
{
}

template<typename NumT>
AttrHolder<NumT>::AttrHolder(const NumT& value) : HolderBase(""), m_c_value(&value), m_value(NULL)
{
}

template<typename NumT>
NXnumtype AttrHolder<NumT>::getType()
{
    return NeXusAPI::getType<NumT>();
}

template<>
NXnumtype AttrHolder<std::string>::getType()
{
    return NeXusAPI::getType<char>();
}

template<>
void AttrHolder<std::string>::readFromFile(File& nf) const
{
    if (m_value != NULL)
    {
        nf.getAttr(m_name, *m_value);
    }
    else
    {
	throw Exception("AttrHolder<NumT>::readFromFile - not able to read into a constant");
    }
}

template<typename NumT>
void AttrHolder<NumT>::readFromFile(File& nf) const
{
    if (m_value != NULL)
    {
        nf.getAttr(m_name, *m_value);
    }
    else
    {
	throw Exception("AttrHolder<NumT>::readFromFile - not able to read into a constant");
    }
}

template<typename NumT>
void AttrHolder<NumT>::writeToFile(File& nf) const
{
    if (m_value != NULL)
    {
        nf.putAttr(m_name, *m_value);
    }
    else if (m_c_value != NULL)
    {
        nf.putAttr(m_name, *m_c_value);
    }
    else
    {
	throw Exception("AttrHolder<NumT>::writeToFile - no value to write");
    }
}


template<typename NumT>
void DataHolder<NumT>::readFromFile(File& nf) const
{
    if (m_value != NULL)
    {
    	nf.openData(m_name);
	nf.getData(*m_value);
	nf.closeData();
    }
    else if (m_c_value != NULL)
    {
	throw Exception("DataHolder<NumT>::readFromFile - not able to read into a constant");
    }
    else
    {
    	nf.openData(m_name);
    }
}

template<typename NumT>
void DataHolder<NumT>::writeToFile(File& nf) const
{
    if (m_value != NULL)
    {
        nf.writeData(m_name, *m_value);
    }
    else if (m_c_value != NULL)
    {
        nf.writeData(m_name, *m_c_value);
    }
    else
    {
	throw Exception("DataHolder<NumT>::writeToFile - no value to write");
    }
}

template<typename NumT>
DataHolder<NumT>::DataHolder(const std::string& name, std::vector<NumT>& value) : HolderBase(name), m_c_value(NULL), m_value(&value)
{
}

template<typename NumT>
DataHolder<NumT>::DataHolder(const std::string& name) : HolderBase(name), m_c_value(NULL), m_value(NULL)
{
}

template<typename NumT>
DataHolder<NumT>::DataHolder(const std::string& name, const std::vector<NumT>& value) : HolderBase(name), m_c_value(&value), m_value(NULL)
{
}

template<typename NumT>
DataHolder<NumT>::DataHolder(std::vector<NumT>& value) : HolderBase(""), m_c_value(NULL), m_value(&value)
{
}

template<typename NumT>
DataHolder<NumT>::DataHolder(const std::vector<NumT>& value) : HolderBase(""), m_c_value(&value), m_value(NULL)
{
}

template class NXDLL_EXPORT AttrHolder<double>;
template class NXDLL_EXPORT AttrHolder<int>;
template class NXDLL_EXPORT AttrHolder<std::string>;

template class NXDLL_EXPORT DataHolder<double>;
template class NXDLL_EXPORT DataHolder<int>;
template class NXDLL_EXPORT DataHolder<char>;

void Data::readFromFile(File& nf) const
{
    m_holder->readFromFile(nf);
    if (m_attr.size() > 0)
    {
    	nf.openData(m_holder->getName());
	ObjectWithAttr::readFromFile(nf);
    	nf.closeData();
    }
}

void Data::writeToFile(File& nf) const
{
    m_holder->writeToFile(nf);
    if (m_attr.size() > 0)
    {
        nf.openData(m_holder->getName());
	ObjectWithAttr::writeToFile(nf);
        nf.closeData();
    }
}

File& operator<<(File& nf, const ISerialisable& obj)
{
    obj.writeToFile(nf);
    return nf;
}

File& operator>>(File& nf, const ISerialisable& obj)
{
    obj.readFromFile(nf);
    return nf;
}

File& operator<<(File& nf, const StreamModifier sm)
{
    switch(sm)
    {
	case Close:
	    if (nf.isDataSetOpen())
	    {
		nf.closeData();
	    }
	    else
 	    {	
		nf.closeGroup();
	    }
	    break;

	default:
	    break;
    }
    return nf;
}

File& operator>>(File& nf, const StreamModifier sm)
{
    switch(sm)
    {
	case Close:
	    if (nf.isDataSetOpen())
	    {
		nf.closeData();
	    }
	    else
 	    {	
		nf.closeGroup();
	    }
	    break;

	default:
	    break;
    }
    return nf;
}

} // Stream
} // NeXus
