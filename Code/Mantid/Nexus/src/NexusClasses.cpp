//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/NexusClasses.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{
namespace NeXus
{

    std::vector<std::string> NXAttributes::names()const
    {
        std::vector<std::string> out;
        std::map<std::string,std::string>::const_iterator it = m_values.begin();
        for(;it!=m_values.end();it++)
            out.push_back(it->first);
        return out;
    }

    std::vector<std::string> NXAttributes::values()const
    {
        std::vector<std::string> out;
        std::map<std::string,std::string>::const_iterator it = m_values.begin();
        for(;it!=m_values.end();it++)
            out.push_back(it->second);
        return out;
    }

    /**  Returns the value of an attribute
     *   @param name The name of the attribute
     *   @return The value of the attribute if it exists or an empty string otherwise
     */
    std::string NXAttributes::operator()(const std::string& name)const
    {
        std::map<std::string,std::string>::const_iterator it = m_values.find(name);
        if (it == m_values.end()) return "";
        return it->second;
    }

    /**  Sets the value of the attribute.
     *   @param name The name of the attribute
     *   @param value The new value of the attribute
     */
    void NXAttributes::set(const std::string &name, const std::string &value)
    {
        m_values[name] = value;
    }

    /**  Sets the value of the attribute as a double.
     *   @param name The name of the attribute
     *   @param value The new value of the attribute
     */
    void NXAttributes::set(const std::string &name, double value)
    {
        std::ostringstream ostr;
        ostr << value;
        m_values[name] = ostr.str();
    }


    //---------------------------------------------------------
    //          NXObject methods
    //---------------------------------------------------------

    NXObject::NXObject(const NXhandle fileID,const NXClass* parent,const std::string& name):m_fileID(fileID),m_open(false)
    {
        if (parent && !name.empty())
        {
            m_path = parent->path() + "/" + name;
        }
    }

    void NXObject::getData(void* data)
    {
        if (NXgetdata(m_fileID,data) != NX_OK)
            throw std::runtime_error("Cannot read data from NeXus file");
    }

    void NXObject::getSlab(void* data, int start[], int size[])
    {
        if (NXgetslab(m_fileID,data,start,size) != NX_OK)
            throw std::runtime_error("Cannot read data slab from NeXus file");
    }

    //---------------------------------------------------------
    //          NXClass methods
    //---------------------------------------------------------

    NXClass::NXClass(const NXClass& parent,const std::string& name):NXObject(parent.m_fileID,&parent,name)
    {
        clear();
    }

    NXClassInfo NXClass::getNextEntry()
    {
        NXClassInfo res;
        char nxname[NX_MAXNAMELEN],nxclass[NX_MAXNAMELEN];
        res.stat = NXgetnextentry(m_fileID,nxname,nxclass,&res.datatype);
        res.nxname = nxname;
        res.nxclass = nxclass;
        return res;
    }

    void NXClass::readAllInfo()
    {
        clear();
        NXClassInfo info;
        while(info = getNextEntry())
        {
            if (info.nxclass == "SDS")
            {
                NXInfo data_info;
                NXopendata(m_fileID,info.nxname.c_str());
                NXgetinfo(m_fileID, &data_info.rank, data_info.dims, &data_info.type);
                NXclosedata(m_fileID);
                data_info.nxname = info.nxname;
                m_datasets->push_back(data_info);
            }
            else if(info.nxclass.substr(0,2) == "NX")
            {
                m_groups->push_back(info);
            }
            //std::cerr<<'!'<<info.nxname<<'\n';
        }
        reset();
    }

    void NXClass::open()
    {
        if (NX_ERROR == NXopengrouppath(m_fileID,m_path.c_str())) 
        {
            throw std::runtime_error("Cannot open group "+m_path+" of class "+NX_class());
        }
        m_open = true;
        readAllInfo();
    }

    void NXClass::reset()
    {
        NXinitgroupdir(m_fileID);
    }

    void NXClass::clear()
    {
        m_groups.reset(new std::vector<NXClassInfo>);
        m_datasets.reset(new std::vector<NXInfo>);
    }

    //---------------------------------------------------------
    //          NXNote methods
    //---------------------------------------------------------

    std::string NXNote::author()
    {
        if (!m_author_ok)
        {
            NXChar aut = openNXChar("author");
            aut.load();
            m_author = std::string(aut(),aut.dim0());
            m_author_ok = true;
        }
        return m_author;
    }

    std::vector< std::string >& NXNote::data()
    {
        if (!m_data_ok)
        {
            NXChar str = openNXChar("data");
            str.load();
            std::istringstream istr(std::string(str(),str.dim0()));
            std::string line;
            size_t i = 0;
            while(getline(istr,line))
            {
                m_data.push_back(line);
                //std::cerr<<"data("<<i++<<"):"<<line<<'\n';
            }
            m_data_ok = true;
        }
        return m_data;
    }

    std::string NXNote::description()
    {
        if (!m_description_ok)
        {
            NXChar str = openNXChar("description");
            str.load();
            m_description = std::string(str(),str.dim0());
            m_description_ok = true;
        }
        return m_description;
    }

    //---------------------------------------------------------
    //          NXRoot methods
    //---------------------------------------------------------

    /**  Constructor. On creation opens the Nexus file for reading only.
     */
    NXRoot::NXRoot(const std::string& fname)
        :m_filename(fname)
    {
        // Open NeXus file
        NXstatus stat=NXopen(m_filename.c_str(), NXACC_READ, &m_fileID);
        if(stat==NX_ERROR)
        {
            throw Kernel::Exception::FileError("Unable to open File:" , m_filename);  
        }
        readAllInfo();
    }

    /**  Constructor.
     *   Creates a new Nexus file. The first root entry will be also created.
     *   @param fanme The file name to create
     *   @param entry The name of the first entry in the new file
     */
    NXRoot::NXRoot(const std::string& fname,const std::string& entry)
        :m_filename(fname)
    {
        // Open NeXus file
        NXstatus stat=NXopen(m_filename.c_str(), NXACC_CREATE5, &m_fileID);
        if(stat==NX_ERROR)
        {
            throw Kernel::Exception::FileError("Unable to open File:" , m_filename);  
        }
    }

    NXRoot::~NXRoot()
    {
        NXclose(&m_fileID);
    }

    bool NXRoot::isStandard()const
    {
        return true;
    }

    //---------------------------------------------------------
    //          NXDataSet methods
    //---------------------------------------------------------

    NXDataSet::NXDataSet(const NXClass& parent,const std::string& name)
        :NXObject(parent.m_fileID,&parent,name)
    {
        m_info.nxname = name;
    }
    void NXDataSet::open()
    {
        if (NX_ERROR == NXopenpath(m_fileID,m_path.c_str())) 
        {
            throw std::runtime_error("Cannot open dataset "+m_path);
        }
        NXgetinfo(m_fileID, &m_info.rank, m_info.dims, &m_info.type);
    }


} // namespace DataHandling
} // namespace Mantid
