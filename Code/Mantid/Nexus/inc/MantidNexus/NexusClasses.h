#ifndef MANTID_NEXUS_NEXUSCLASSES_H_
#define MANTID_NEXUS_NEXUSCLASSES_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Sample.h"
#include <napi.h>

#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <map>
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------

namespace Mantid
{
    namespace NeXus
    {

        /** C++ implementation of NeXus classes.

        @author Roman Tolchenov, Tessella plc
        @date 28/05/2009

        Copyright &copy; 2007-9 STFC Rutherford Appleton Laboratory

        This file is part of Mantid.

        Mantid is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 3 of the License, or
        (at your option) any later version.

        Mantid is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program.  If not, see <http://www.gnu.org/licenses/>.

        File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>. 
        Code Documentation is available at: <http://doxygen.mantidproject.org>
        */

        /** Structure for keeping information about a Nexus data set,
         *  such as the dimensions and the type
         */
        struct NXInfo
        {
            std::string nxname;  // name of the object
            int rank;    // number of dimensions of the data
            int dims[4]; // sizes along each dimension
            int type;    // type of the data, e.g. NX_CHAR, NX_FLOAT32, see napi.h
        };

        struct NXClassInfo
        {
            std::string nxname;  // name of the object
            std::string nxclass; // NX class of the object or "SDS" if a dataset
            int datatype;        // NX data type if a dataset
            NXstatus stat;
            operator bool(){return stat == NX_OK;}
        };

        /**  Nexus attributes. The type of each attribute is NX_CHAR
         */
        class NXAttributes
        {
        public:
            int n()const{return int(m_values.size());} ///< number of attributes
            std::vector<std::string> names()const;  ///< Returns the list of attribute names
            std::vector<std::string> values()const;  ///< Returns the list of attribute values
            std::string operator()(const std::string& name)const; // returns the value of attribute with name name
            void set(const std::string& name,const std::string& value); // set the attribute's value
            void set(const std::string& name,double value); // set the attribute's value as a double
        private:
            std::map<std::string,std::string> m_values;  ///< the list of attributes
        };

        class NXClass;

        /**  The base abstract class for NeXus classes and data sets. 
         *    NX classes and data sets are defined at www.nexusformat.org
         */
        class DLLExport NXObject
        {
            friend class NXDataSet;
            friend class NXClass;
            friend class NXRoot;
        public:
            NXObject(const NXhandle fileID,const NXClass* parent,const std::string& name);
            virtual ~NXObject(){};
            /// Return the NX class for a class (HDF group) or "SDS" for a data set;
            virtual std::string NX_class()const = 0;
            /// True if complies with our understanding of the www.nexusformat.org definition.
            //virtual bool isStandard()const = 0;
            std::string path()const{return m_path;}
        protected:
            void getData(void* data);
            void getSlab(void* data, int start[], int size[]);
            NXhandle m_fileID;
            std::string m_path;
            //boost::shared_ptr<NXClass> m_parent;
            bool m_open;
        private:
            NXObject():m_fileID(){}
        };

        /** Abstract base class for a data set.
         */
        class DLLExport NXDataSet: public NXObject
        {
        public:
            NXDataSet(const NXClass& parent,const std::string& name);
            std::string NX_class()const{return "SDS";}
            void open();
            int rank()const{return m_info.rank;}
            int dims(int i)const{return i<4?m_info.dims[i]:0;}
            int dim0()const{return m_info.dims[0];}
            int dim1()const{return m_info.dims[1];}
            int dim2()const{return m_info.dims[2];}
            int dim3()const{return m_info.dims[3];}
            std::string name()const{return m_info.nxname;}
            int type()const{return m_info.type;}
            virtual void load(int i=-1,int j=-1,int k=-1,int l=-1) = 0;
        private:
            NXInfo m_info;
        };

        template<class T>
        class NXDataSetTyped: public NXDataSet
        {
        public:
            NXDataSetTyped(const NXClass& parent,const std::string& name):NXDataSet(parent,name){}
            T* operator()()const
            {
                if (!m_data) throw std::runtime_error("Attempt to read uninitialized data");
                return m_data.get();
            }
            T operator[](int i)const
            {
                if (!m_data) throw std::runtime_error("Attempt to read uninitialized data");
                if (i < 0 || i >= m_n) rangeError();
                return m_data[i];
            }
            int size()const{return m_n;}
            void load(int i=-1,int j=-1,int k=-1,int l=-1)
            {
                if (rank()>4)
                {
                    throw std::runtime_error("Cannot load dataset of rank greater than 4");
                }
                m_n = 0;
                int start[4];
                if (rank() == 4)
                {
                    if (i < 0) // load all data
                    {
                        m_n = dim0()*dim1()*dim2()*dim3();
                        alloc(m_n);
                        getData(m_data.get());
                        return;
                    }
                    else if (j < 0)
                    {
                        if (i >= dim0()) rangeError();
                        m_n = dim1()*dim2()*dim3();
                        start[0] = i; m_size[0] = 1;
                        start[1] = 0; m_size[1] = dim1();
                        start[2] = 0; m_size[2] = dim2();
                        start[3] = 0; m_size[3] = dim2();
                    }
                    else if (k < 0)
                    {
                        if (i >= dim0() || j >= dim1()) rangeError();
                        m_n = dim2()*dim3();
                        start[0] = i; m_size[0] = 1;
                        start[1] = j; m_size[1] = 1;
                        start[2] = 0; m_size[2] = dim2();
                        start[3] = 0; m_size[3] = dim2();
                    }
                    else if (l < 0)
                    {
                        if (i >= dim0() || j >= dim1() || k >= dim2()) rangeError();
                        m_n = dim3();
                        start[0] = i; m_size[0] = 1;
                        start[1] = j; m_size[1] = 1;
                        start[2] = k; m_size[2] = 1;
                        start[3] = 0; m_size[3] = dim2();
                    }
                    else
                    {
                        if (i >= dim0() || j >= dim1() || k >= dim2() || l >= dim3()) rangeError();
                        m_n = dim3();
                        start[0] = i; m_size[0] = 1;
                        start[1] = j; m_size[1] = 1;
                        start[2] = k; m_size[2] = 1;
                        start[3] = l; m_size[3] = 1;
                    }
                }
                else if (rank() == 3)
                {
                    if (i < 0)
                    {
                        m_n = dim0()*dim1()*dim2();
                        alloc(m_n);
                        getData(m_data.get());
                        return;
                    }
                    else if (j < 0)
                    {
                        if (i >= dim0()) rangeError();
                        m_n = dim1()*dim2();
                        start[0] = i; m_size[0] = 1;
                        start[1] = 0; m_size[1] = dim1();
                        start[2] = 0; m_size[2] = dim2();
                    }
                    else if (k < 0)
                    {
                        if (i >= dim0() || j >= dim1()) rangeError();
                        m_n = dim2();
                        start[0] = i; m_size[0] = 1;
                        start[1] = j; m_size[1] = 1;
                        start[2] = 0; m_size[2] = dim2();
                    }
                    else
                    {
                        if (i >= dim0() || j >= dim1() || k >= dim2()) rangeError();
                        m_n = 1;
                        start[0] = i; m_size[0] = 1;
                        start[1] = j; m_size[1] = 1;
                        start[2] = k; m_size[2] = 1;
                    }
                }
                else if (rank() == 2)
                {
                    if (i < 0)
                    {
                        m_n = dim0()*dim1();
                        alloc(m_n);
                        getData(m_data.get());
                        return;
                    }
                    else if (j < 0)
                    {
                        if (i >= dim0()) rangeError();
                        m_n = dim1();
                        start[0] = i; m_size[0] = 1;
                        start[1] = 0; m_size[1] = dim1();
                    }
                    else
                    {
                        if (i >= dim0() || j >= dim1()) rangeError();
                        m_n = dim1();
                        start[0] = i; m_size[0] = 1;
                        start[1] = j; m_size[1] = 1;
                    }
                }
                else if (rank() == 1)
                {
                    if (i < 0)
                    {
                        m_n = dim0();
                        alloc(m_n);
                        getData(m_data.get());
                        return;
                    }
                    else
                    {
                        if (i >= dim0()) rangeError();
                        m_n = 1;
                        start[0] = i;
                        m_size[0] = 1;
                    }
                }
                alloc(m_n);
                getSlab(m_data.get(),start,m_size);
            }
        protected:
            void alloc(int n)
            {
                try
                {
                    m_data.reset(new T[n]);
                }
                catch(...)
                {
                    std::ostringstream ostr;
                    ostr << "Cannot allocate "<<n*sizeof(T)<<" bytes of memory to load the data";
                    throw std::runtime_error(ostr.str());
                }
            }
            void rangeError()const
            {
                throw std::range_error("Nexus dataset range error");
            }
            boost::shared_array<T> m_data;
            int m_size[4];
            int m_n;
        };

        typedef NXDataSetTyped<int> NXInt;
        typedef NXDataSetTyped<float> NXFloat;
        typedef NXDataSetTyped<double> NXDouble;
        typedef NXDataSetTyped<char> NXChar;
        
//#define NX_FLOAT32   5   float
//#define NX_FLOAT64   6   double
//#define NX_INT8     20  
//#define NX_UINT8    21
//#define NX_BOOLEAN NX_UINT
//#define NX_INT16    22  
//#define NX_UINT16   23
//#define NX_INT32    24   int
//#define NX_UINT32   25   unsigned int
//#define NX_INT64    26
//#define NX_UINT64   27
//#define NX_CHAR      4   char
//#define NX_BINARY   21

        //-------------------- classes --------------------------//

        class DLLExport NXClass: public NXObject
        {
            friend class NXRoot;
        public:
            NXClass(const NXClass& parent,const std::string& name);
            std::string NX_class()const{return "NXClass";}
            NXClassInfo getNextEntry();
            /// Creates a new object in the NeXus file at path path.
            //virtual void make(const std::string& path) = 0;
            void reset();
            template<class NX>
            NX openNXClass(const std::string& name)const
            {
                NX nxc(*this,name);
                nxc.open();
                return nxc;
            }

            NXClass openNXGroup(const std::string& name)const{return openNXClass<NXClass>(name);}

            template<class T>
            NXDataSetTyped<T> openNXData(const std::string& name)const
            {
                NXDataSetTyped<T> data(*this,name);
                data.open();
                return data;
            }

            NXInt openNXInt(const std::string& name)const{return openNXData<int>(name);}
            NXFloat openNXFloat(const std::string& name)const{return openNXData<float>(name);}
            NXDouble openNXDouble(const std::string& name)const{return openNXData<double>(name);}
            NXChar openNXChar(const std::string& name)const{return openNXData<char>(name);}

            boost::shared_ptr<std::vector<NXClassInfo> > m_groups;
            boost::shared_ptr<std::vector<NXInfo> > m_datasets;
        protected:
            void readAllInfo();
            void open();
            void clear(); // deletes content of m_groups and m_datasets
        private:
            NXClass():NXObject(){clear();}
        };

        //------------------- auxilary classes ----------------------------//

        
        class DLLExport NXLog:public NXClass
        {
        public:
            NXLog(const NXClass& parent,const std::string& name):NXClass(parent,name){}
            std::string NX_class()const{return "NXlog";}
        };

        
        //-------------------- main classes -------------------------------//

        class DLLExport NXMainClass:public NXClass
        {
        public:
            NXMainClass(const NXClass& parent,const std::string& name):NXClass(parent,name){}
            NXLog openNXLog(const std::string& name){return openNXClass<NXLog>(name);}
        };



        class DLLExport NXData:public NXMainClass
        {
        public:
            NXData(const NXClass& parent,const std::string& name):NXMainClass(parent,name){}
            std::string NX_class()const{return "NXdata";}
        };

        class DLLExport NXDetector:public NXMainClass
        {
        public:
            NXDetector(const NXClass& parent,const std::string& name):NXMainClass(parent,name){}
            std::string NX_class()const{return "NXdetector";}
        };

        class DLLExport NXInstrument:public NXMainClass
        {
        public:
            NXInstrument(const NXClass& parent,const std::string& name):NXMainClass(parent,name){}
            std::string NX_class()const{return "NXinstrument";}
        };

        class DLLExport NXEntry:public NXMainClass
        {
        public:
            NXEntry(const NXClass& parent,const std::string& name):NXMainClass(parent,name){}
            std::string NX_class()const{return "NXentry";}
            NXData openNXData(const std::string& name){return openNXClass<NXData>(name);}
            NXInstrument openNXInstrument(const std::string& name){return openNXClass<NXInstrument>(name);}
        };

        class DLLExport NXRoot: public NXClass
        {
        public:
            NXRoot(const std::string& fname);
            NXRoot(const std::string& fname,const std::string& entry);
            //~NXRoot();
            /// Return the NX class for a class (HDF group) or "SDS" for a data set;
            std::string NX_class()const{return "NXroot";}
            /// True if complies with our understanding of the www.nexusformat.org definition.
            bool isStandard()const;
            NXEntry openEntry(const std::string& name){return openNXClass<NXEntry>(name);}
        private:
            const std::string m_filename;
        };


    } // namespace NeXus
} // namespace Mantid

#endif /*MANTID_NEXUS_NEXUSCLASSES_H_*/
