#ifndef NEXUS_STREAM_HPP
#define NEXUS_STREAM_HPP
//
//  NeXus - Neutron & X-ray Common Data Format
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
//

/////////////////// Subversion Repository Details ////////////////////////
// Repository Location     $HeadURL$
// Revision of last commit $LastChangedRevision$
// Date of last commit     $LastChangedDate$
// Last changed by         $LastChangedBy$
//////////////////////////////////////////////////////////////////////////

/**
 * \file NeXusStream.hpp
 * Header for IOStream like interface to NeXus files
 * \author Freddie Akeroyd, STFC ISIS Facility, GB
 * \version $LastChangedRevision$
 * \date    $LastChangedDate$
 * \defgroup cpp_stream IOstream like interface
 * \ingroup cpp_main
 */

#include "MantidNexusCpp/NeXusFile.hpp"
#include <list>
#include <vector>

namespace NeXus {
namespace Stream {
/**
 * interface implemented by all serialisable NeXus components
 */
class MANTID_NEXUSCPP_DLL ISerialisable {
public:
  virtual void readFromFile(File &nf) const = 0;
  virtual void writeToFile(File &nf) const = 0;
};

/// \ingroup cpp_stream
enum StreamModifier { Close = 0 };

/**
 * Base class for serialisable named and typed parameter
 */
class MANTID_NEXUSCPP_DLL HolderBase : public ISerialisable {
protected:
  std::string m_name;

public:
  HolderBase() : m_name("") {}
  HolderBase(const std::string &name);
  void setName(const std::string &name);
  std::string getName() { return m_name; }
  virtual NXnumtype getType() = 0;
  virtual HolderBase *clone() = 0;
  virtual ~HolderBase() {}
};

/**
 * Serialisable NeXus attribute
 */
template <typename NumT> class MANTID_NEXUSCPP_DLL AttrHolder : public HolderBase {
protected:
  const NumT *m_c_value;
  NumT *m_value;
  AttrHolder() : HolderBase(), m_c_value(NULL), m_value(NULL) {}
  AttrHolder(const std::string &name, const NumT *cv, NumT *v) : HolderBase(name), m_c_value(cv), m_value(v) {}

public:
  AttrHolder(const std::string &name, NumT &value);
  AttrHolder(const std::string &name, const NumT &value);
  AttrHolder(NumT &value);
  AttrHolder(const NumT &value);
  NXnumtype getType();
  virtual void readFromFile(File &nf) const;
  virtual void writeToFile(File &nf) const;
  AttrHolder *clone() { return new AttrHolder(m_name, m_c_value, m_value); }
  virtual ~AttrHolder() {
    m_value = NULL;
    m_c_value = NULL;
  }
};

/**
 * Serialisable attribute
 * \ingroup cpp_stream
 */
class MANTID_NEXUSCPP_DLL Attr : public ISerialisable {
protected:
  HolderBase *m_holder;

public:
  Attr() : m_holder(NULL) {}
  template <typename NumT> Attr(NumT &d) { m_holder = new AttrHolder<NumT>(d); }
  template <typename NumT> Attr(const NumT &d) { m_holder = new AttrHolder<NumT>(d); }
  template <typename NumT> Attr(const std::string &name, NumT &d) { m_holder = new AttrHolder<NumT>(name, d); }
  template <typename NumT> Attr(const std::string &name, const NumT &d) { m_holder = new AttrHolder<NumT>(name, d); }
  Attr(const std::string &name, Attr &d) {
    m_holder = d.m_holder->clone();
    setName(name);
  }
  Attr(const std::string &name, const Attr &d) {
    m_holder = d.m_holder->clone();
    setName(name);
  }
  Attr(const Attr &a) : m_holder(NULL) { m_holder = a.m_holder->clone(); }
  Attr &operator=(const Attr &a) {
    if (this != &a) {
      delete m_holder;
      m_holder = a.m_holder->clone();
    }
    return *this;
  }
  void setName(const std::string &name) { m_holder->setName(name); }
  virtual void readFromFile(File &nf) const override { m_holder->readFromFile(nf); }
  virtual void writeToFile(File &nf) const override { m_holder->writeToFile(nf); }
  virtual ~Attr() {
    delete m_holder;
    m_holder = NULL;
  }
};

/**
 * Serialisable NeXus class with associated attributes
 */
class MANTID_NEXUSCPP_DLL ObjectWithAttr : public ISerialisable {
protected:
  std::list<Attr> m_attr;

  void processAttr(const std::string &attr1_name, const Attr &attr1_value, const std::string &attr2_name,
                   const Attr &attr2_value) {
    if (attr1_name.size() > 0) {
      m_attr.push_back(Attr(attr1_name, attr1_value));
    }
    if (attr2_name.size() > 0) {
      m_attr.push_back(Attr(attr2_name, attr2_value));
    }
  }

public:
  ObjectWithAttr(const std::string &attr1_name = "", const Attr &attr1_value = Attr(),
                 const std::string &attr2_name = "", const Attr &attr2_value = Attr()) {
    processAttr(attr1_name, attr1_value, attr2_name, attr2_value);
  }

  virtual void readFromFile(File &nf) const override {
    for (std::list<Attr>::const_iterator it = m_attr.begin(); it != m_attr.end(); ++it) {
      it->readFromFile(nf);
    }
  }

  virtual void writeToFile(File &nf) const override {
    for (std::list<Attr>::const_iterator it = m_attr.begin(); it != m_attr.end(); ++it) {
      it->writeToFile(nf);
    }
  }

  virtual ~ObjectWithAttr() {}
};

/**
 * Serialisable NeXus group object
 * \ingroup cpp_stream
 */
class MANTID_NEXUSCPP_DLL Group : public ObjectWithAttr {
protected:
  std::string m_name;
  std::string m_class;

public:
  Group(const std::string &name, const std::string &nxclass, const std::string &attr1_name = "",
        const Attr &attr1_value = Attr(), const std::string &attr2_name = "", const Attr &attr2_value = Attr())
      : ObjectWithAttr(attr1_name, attr1_value, attr2_name, attr2_value), m_name(name), m_class(nxclass) {}

  virtual void readFromFile(File &nf) const override {
    nf.openGroup(m_name, m_class);
    ObjectWithAttr::readFromFile(nf);
  }

  virtual void writeToFile(File &nf) const override {
    nf.makeGroup(m_name, m_class, true);
    ObjectWithAttr::writeToFile(nf);
  }

  virtual ~Group() {}
};

/**
 * Serialisable NeXus data
 */
template <typename NumT> class MANTID_NEXUSCPP_DLL DataHolder : public HolderBase {
protected:
  const std::vector<NumT> *m_c_value;
  std::vector<NumT> *m_value;
  DataHolder() : HolderBase(), m_c_value(NULL), m_value(NULL) {}
  DataHolder(const std::string &name, const std::vector<NumT> *cv, std::vector<NumT> *v)
      : HolderBase(name), m_c_value(cv), m_value(v) {}

public:
  DataHolder(const std::string &name);
  DataHolder(const std::string &name, std::vector<NumT> &value);
  DataHolder(const std::string &name, const std::vector<NumT> &value);
  DataHolder(std::vector<NumT> &value);
  DataHolder(const std::vector<NumT> &value);
  NXnumtype getType() { return NeXus::getType<NumT>(); }
  virtual void readFromFile(File &nf) const override;
  virtual void writeToFile(File &nf) const override;
  DataHolder *clone() override { return new DataHolder(m_name, m_c_value, m_value); }
  virtual ~DataHolder() override {}
};

/**
 * Serialisable data object that contains attributes
 * \ingroup cpp_stream
 */
class MANTID_NEXUSCPP_DLL Data : public ObjectWithAttr {
  HolderBase *m_holder;

public:
  Data() : ObjectWithAttr(), m_holder(NULL) {}
  Data(const std::string &name) : ObjectWithAttr() {
    m_holder = new DataHolder<int>(name); // TODO: move name out of holder and use  m_holder = NULL
  }
  template <typename NumT>
  Data(const std::string &name, std::vector<NumT> &data, const std::string &attr1_name = "",
       const Attr &attr1_value = Attr(), const std::string &attr2_name = "", const Attr &attr2_value = Attr())
      : ObjectWithAttr(attr1_name, attr1_value, attr2_name, attr2_value) {
    m_holder = new DataHolder<NumT>(name, data);
  }
  template <typename NumT>
  Data(const std::string &name, const std::vector<NumT> &data, const std::string &attr1_name = "",
       const Attr &attr1_value = Attr(), const std::string &attr2_name = "", const Attr &attr2_value = Attr())
      : ObjectWithAttr(attr1_name, attr1_value, attr2_name, attr2_value) {
    m_holder = new DataHolder<NumT>(name, data);
  }
  Data(const Data &d) : ObjectWithAttr(d), m_holder(NULL) { m_holder = d.m_holder->clone(); }
  Data &operator=(const Data &d) {
    if (this != &d) {
      delete m_holder;
      m_holder = d.m_holder->clone();
    }
    return *this;
  }
  virtual void readFromFile(File &nf) const override;
  virtual void writeToFile(File &nf) const override;
  virtual ~Data() override { delete m_holder; }
};

/// \ingroup cpp_stream
MANTID_NEXUSCPP_DLL File &operator<<(File &nf, const ISerialisable &obj);

/// \ingroup cpp_stream
MANTID_NEXUSCPP_DLL File &operator>>(File &nf, const ISerialisable &obj);

/// \ingroup cpp_stream
MANTID_NEXUSCPP_DLL File &operator<<(File &nf, const StreamModifier sm);

/// \ingroup cpp_stream
MANTID_NEXUSCPP_DLL File &operator>>(File &nf, const StreamModifier sm);

} // namespace Stream
} // namespace NeXus

#endif /* NEXUS_STREAM_HPP */
