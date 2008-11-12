#include <iostream>
#include "Benchmark.h"
#include "UserAlgorithmTest.h"
#include "MantidAPI/FrameworkManager.h"
//#include "MantidAPI/Workspace.h"
//#include "MantidDataObjects/Workspace1D.h" 
//#include "MantidDataObjects/Workspace2D.h" 

#include "MantidDataObjects/TableColumn.h" 
#include "MantidDataObjects/TablePointerColumn.h" 
#include "MantidDataObjects/TableWorkspace.h" 
#include "MantidDataObjects/ColumnFactory.h"
#include "MantidGeometry/V3D.h"

#include "boost/tuple/tuple.hpp"

using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;


class Base
{
public:
    int i;
    Base():i(0){}
private:
    Base(const Base&);
};

class Child:public Base
{
public:
    double d;
    Child():Base(),d(0){} 
    ~Child(){/*std::cerr<<"Child "<<d<<" deleted.\n";*/}
private:
    Child(const Child&);
};

//DECLARE_TABLEPOINTERCOLUMN(Base,Base);
DECLARE_TABLEPOINTERCOLUMN(Child,Child);
DECLARE_TABLECOLUMN(char,char);

int main()
{ 
//#define STR1(x) #x
//#define STR(x) STR1 ( x )
//#pragma message( STR( DLLExport ) )

    boost::tuples::tuple<int,std::string> t; 

    TableWorkspace tw(3);
    tw.createColumn("int","Number");
    tw.createColumn("str","Name");
    tw.createColumn("V3D","Position");
    tw.createColumn("Child","child");
    tw.createColumn("char","Char");
    //tw.setRowCount(3);
    std::cerr<<tw.columnCount()<<' '<<tw.rowCount()<<'\n';

    try
    {
        ColumnVector<int> cii = tw.getVector("Number");


        cii[0] = 100;
        std::cout<<"cii[0]="<<cii[0]<<'\n';
        ColumnVector<Mantid::Geometry::V3D> vec = tw.getVector("Position");
        vec[1] = Mantid::Geometry::V3D(10,20,30);
        std::cout<<vec[1]<<'\n';

        TablePointerColumn_ptr<Child> child = tw.getColumn("child");

        child->data(0).d = 1;
        child->data(1).d = 2;
        child->data(2).d = 3;
        std::cerr<<"child[0]= "<<child->data(0).d<<'\n';
        std::cerr<<"child[1]= "<<child->data(1).d<<'\n';
        std::cerr<<"child[2]= "<<child->data(2).d<<'\n';
        
        ColumnPointerVector<Child> ch = tw.getVector("child");
        std::cerr<<ch[0].d<<'\n';

        tw.insertRow(1); ch[1].d = 4;
        std::cerr<<"child.size="<<child->size()<<'\n';
        for(int i=0;i<ch.size();i++)
            std::cerr<<"child["<<i<<"]= "<<ch[i].d<<'\n';std::cerr<<'\n';
        int j;
        std::cerr<<"inserted "<<(j=tw.insertRow(10))<<'\n';ch[j].d = 10;
        for(int i=0;i<ch.size();i++)
            std::cerr<<"child["<<i<<"]= "<<ch[i].d<<'\n';std::cerr<<'\n';
        tw.removeRow(3);
        for(int i=0;i<ch.size();i++)
            std::cerr<<"child["<<i<<"]= "<<ch[i].d<<'\n';std::cerr<<'\n';
        tw.setRowCount(10);
        for(int i=0;i<ch.size();i++)
            std::cerr<<"child["<<i<<"]= "<<ch[i].d<<'\n';std::cerr<<'\n';
        //tw.removeColumn("child");
        tw.setRowCount(2);
        for(int i=0;i<ch.size();i++)
            std::cerr<<"child["<<i<<"]= "<<ch[i].d<<'\n';std::cerr<<'\n';

        tw.removeColumn("Name");
        std::cerr<<"tw.size="<<tw.columnCount()<<'\n';


        int i0 = tw.getRef<int>("Number",1);
        i0 = 99;
        boost::tuples::tuple<int&,Mantid::Geometry::V3D&,Child&> tup(tw.getRef<int>("Number",1),tw.getRef<Mantid::Geometry::V3D>("Position",1),tw.getRef<Child>("child",1));
        std::cerr<<"Tuple.child[1]="<<tup.get<2>().d<<'\n';


        std::vector<std::string> names;
        names.push_back( "Number");
        names.push_back(  "Position");
        names.push_back( "child");

        boost::tuples::tuple<int,Mantid::Geometry::V3D,Child> tup1;
        tw.set_Tuple(1,tup1,names);

        boost::tuples::tuple<int*,Mantid::Geometry::V3D*,Child*> tup2 =
            tw.make_TupleRef< boost::tuples::tuple<int*,Mantid::Geometry::V3D*,Child*> >(1,names);

        boost::tuples::tuple<int*,Mantid::Geometry::V3D*,Child*> tup3;
        tw.set_TupleRef(1,tup3,names);

        std::cout<<typeid(boost::tuples::tuple<int&,Mantid::Geometry::V3D&,Child&>::head_type).name()<<'\n';
        std::cerr<<"tup1.d="<<tup1.get<2>().d<<'\n';
        tup1.get<2>().d = 7.7;
        std::cerr<<"tup.d="<<tup.get<2>().d<<'\n';
        std::cerr<<"tup1.d="<<tup1.get<2>().d<<'\n';
        std::cerr<<"tup2.d="<<tup2.get<2>()->d<<'\n';
    }
    catch(std::exception& e)
    {
        std::cerr<<"Error: "<<e.what()<<'\n';
    }


/*    try
    {
        TablePointerColumn_ptr<Child> ci = ColumnFactory::Instance().create("Child");
        ci->resize(2);
        static_cast<Child&>(ci->data(0)).d = 10.98;
        static_cast<Child&>(ci->data(1)).d = 20.98;
        std::cerr<<"Child "<<static_cast<Child&>(ci->data(0)).d<<'\n';
        std::cerr<<"Child "<<static_cast<Child&>(ci->data(1)).d<<'\n';
        ci->resize(1);
        std::cerr<<"Size="<<ci->size()<<'\n';
        std::cerr<<"Child "<<static_cast<Child&>(ci->data(0)).d<<'\n';
    }
    catch(std::exception& e)
    {
        std::cerr<<"Error: "<<e.what()<<'\n';
    }*/

}
