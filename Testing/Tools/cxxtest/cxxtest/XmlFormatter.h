// Licensed under the LGPL, see http://www.gnu.org/licenses/lgpl.html

#ifndef __CXXTEST__XMLFORMATTER_H
#define __CXXTEST__XMLFORMATTER_H

//
// The XmlFormatter is a TestListener that
// prints reports of the errors to an output
// stream in the form of an XML document.
//

// The following definitions are used if stack trace support is enabled,
// to give the traces an easily-parsable XML format.  If stack tracing is
// not enabled, then these definitions will be ignored.
#define CXXTEST_STACK_TRACE_ESCAPE_AS_XML
#define CXXTEST_STACK_TRACE_NO_ESCAPE_FILELINE_AFFIXES

#define CXXTEST_STACK_TRACE_INITIAL_PREFIX "<stack-frame function=\""
#define CXXTEST_STACK_TRACE_INITIAL_SUFFIX "\"/>\n"
#define CXXTEST_STACK_TRACE_OTHER_PREFIX CXXTEST_STACK_TRACE_INITIAL_PREFIX
#define CXXTEST_STACK_TRACE_OTHER_SUFFIX CXXTEST_STACK_TRACE_INITIAL_SUFFIX
#define CXXTEST_STACK_TRACE_ELLIDED_MESSAGE ""
#define CXXTEST_STACK_TRACE_FILELINE_PREFIX "\" location=\""
#define CXXTEST_STACK_TRACE_FILELINE_SUFFIX ""


#include <cxxtest/TestRunner.h>
#include <cxxtest/TestListener.h>
#include <cxxtest/TestTracker.h>
#include <cxxtest/ValueTraits.h>
#include <cxxtest/ErrorFormatter.h>
#include <cxxtest/StdHeaders.h>
#include <ctime>
#ifdef _WIN32
#include <time.h>
#else
#include <sys/time.h>
#endif
#include <time.h>
#include <iostream>
#include <sstream>
#include <cstring>
#include <stdexcept>


namespace CxxTest
{
    class TeeOutputStreams
    {
    private:
       class teebuffer : public std::basic_streambuf<char>
       {
          typedef std::basic_streambuf<char> streambuf_t;
       public:
          teebuffer(streambuf_t * buf1, streambuf_t * buf2)
             : buffer1(buf1), buffer2(buf2)
          {}

          virtual int overflow(int c)
          {
             if (c == EOF)
                return !EOF;
             else
             {
                int const ans1 = buffer1->sputc(static_cast<char>(c));
                int const ans2 = buffer2->sputc(static_cast<char>(c));
                return ans1 == EOF || ans2 == EOF ? EOF : c;
             }
          }

          virtual int sync()
          {
             int ans1 = buffer1->pubsync();
             int ans2 = buffer2->pubsync();
             return ans1 || ans2 ? -1 : 0;
          }

          streambuf_t * buffer1;
          streambuf_t * buffer2;
       };

    public:
       TeeOutputStreams(std::ostream& _cout, std::ostream& _cerr)
          : out(),
            err(),
            orig_cout(_cout),
            orig_cerr(_cerr),
            tee_out(out.rdbuf(), _cout.rdbuf()),
            tee_err(err.rdbuf(), _cerr.rdbuf())
       {
          orig_cout.rdbuf(&tee_out);
          orig_cerr.rdbuf(&tee_err);
       }

       ~TeeOutputStreams()
       {
          orig_cout.rdbuf(tee_out.buffer2);
          orig_cerr.rdbuf(tee_err.buffer2);
       }

       std::stringstream out;
       std::stringstream err;

    private:
       std::ostream&  orig_cout;
       std::ostream&  orig_cerr;
       teebuffer      tee_out;
       teebuffer      tee_err;
    };

    class ElementInfo
    {
    public:
        std::string name;
        std::stringstream value;
        std::map<std::string,std::string> attribute;

        ElementInfo()
           : name(), value(), attribute()
        {}

        ElementInfo(const ElementInfo& rhs)
           : name(rhs.name), value(rhs.value.str()), attribute(rhs.attribute)
        {}

        ElementInfo& operator=(const ElementInfo& rhs)
        {
           name = rhs.name;
           value.str(rhs.value.str());
           attribute = rhs.attribute;
           return *this;
        }

        template <class Type>
        void add(const std::string& name, Type& value)
            {
            std::ostringstream os;
            os << value;
            attribute[name] = os.str();
            }

        void write(OutputStream& os) {
            os << "        <" << name.c_str() << " ";
            std::map<std::string,std::string>::iterator curr=attribute.begin();
            std::map<std::string,std::string>::iterator end =attribute.end();
            while (curr != end) {
              os << curr->first.c_str() 
                 << "=\"" << curr->second.c_str() << "\" ";
              curr++;
              }
            if (value.str().empty()) {
                os << "/>";
            }
            else {
                os << ">" << escape(value.str()).c_str() 
                   << "</" << name.c_str() << ">";
            }
            os.endl(os);
            }

        std::string escape(const std::string& str)
        {
            std::string escStr = "";
            for(size_t i = 0; i < str.length(); i++)
            {
                switch(str[i])
                {
                    case '"':  escStr += "&quot;"; break;
                    case '\'': escStr += "&apos;"; break;
                    case '<':  escStr += "&lt;"; break;
                    case '>':  escStr += "&gt;"; break;
                    case '&':  escStr += "&amp;"; break;
                    default:   escStr += str[i]; break;
                }
            }
            return escStr;
        }

    };

    class TestCaseInfo
    {
    public:

        TestCaseInfo() : fail(false), error(false), runtime(0.0), totalRuntime(0.0) {}
        std::string className;
        std::string testName;
        std::string line;
        bool fail;
        bool error;
        /// Runtime of just the test
        double runtime;
        /// Runtime including setup/teardown.
        double totalRuntime;
        /// CPU fraction = what fraction of the CPU(s) was used.
        double CPUFraction;
        std::list<ElementInfo> elements;
        typedef std::list<ElementInfo>::iterator element_t;
        std::string world;

        element_t add_element(const std::string& name)
            {
            element_t elt = elements.insert(elements.end(), ElementInfo());
            elt->name=name;
            return elt;
            }

        element_t update_element(const std::string& name)
            {
            element_t elt = elements.begin();
            while ( elt != elements.end() )
               {
               if ( elt->name == name )
                  return elt;
               }
            return add_element(name);
            }

        void write( OutputStream &o )
          {
          std::ostringstream timestream1;
          timestream1 << runtime;
          std::ostringstream timestream2;
          timestream2 << totalRuntime;
          std::ostringstream CPUFraction_stream;
          CPUFraction_stream << CPUFraction;

            o << "    <testcase classname=\"" << tracker().world().worldName() << "." << className.c_str()  << "\""
              << " name=\"" << testName.c_str()  << "\""
              << " line=\"" << line.c_str() << "\""
              << " time=\"" << timestream1.str().c_str() << "\""
              << " totalTime=\"" <<  timestream2.str().c_str() << "\""
              << " CPUFraction=\"" << CPUFraction_stream.str().c_str() << "\""
              << "";
            bool elts=false;


            // Add the CPU fraction measurement
            o << "> <measurement><name>CPUFraction</name><value>" << CPUFraction_stream.str().c_str() << "</value></measurement>";
            elts = true; // force there to alway be elements, since I just wrote one

            element_t curr = elements.begin();
            element_t end  = elements.end();
            while (curr != end) {
              if (!elts) {
                 o << ">";
                 o.endl(o);
                 elts=true;
              }
              curr->write(o);
              curr++;
              }

            if (elts)
               o << "    </testcase>";
            else
               o << " />";
            o.endl(o);
          }

    };

    class XmlFormatter : public TestListener
    {
        public:
        XmlFormatter( OutputStream *o, OutputStream *ostr, std::ostringstream *os) 
           : cpuStartTime(0), cpuStopTime(0), _o(o), _ostr(ostr), _os(os), stream_redirect(NULL)
        {}

        virtual ~XmlFormatter()
        {
          delete _ostr;
        }

        std::list<TestCaseInfo> info;
        std::list<TestCaseInfo>::iterator testcase;
        typedef std::list<ElementInfo>::iterator element_t;
        std::string classname;
        int ntests;
        int nfail;
        int nerror;
        double totaltime;
        // The type of this variable is different depending on the platform
      #ifdef _WIN32
        clock_t
      #else
        timeval
      #endif
        testStartTime, testStopTime, testRunStartTime, testRunStopTime;

        /// CPU time (for all processors)
        clock_t cpuStartTime, cpuStopTime;

        int run()
        {
            TestRunner::runAllTests( *this );
            return tracker().failedTests();
        }

        void enterWorld( const WorldDescription & /*desc*/ )
        {
            ntests=0;
            nfail=0;
            nerror=0;
            totaltime=0.0;
        }

        static void totalTests( OutputStream &o )
        {
            char s[WorldDescription::MAX_STRLEN_TOTAL_TESTS];
            const WorldDescription &wd = tracker().world();
            o << wd.strTotalTests( s ) 
              << (wd.numTotalTests() == 1 ? " test" : " tests");
        }

        void enterSuite( const SuiteDescription& desc )
        {
                classname = desc.suiteName();
                // replace "::" namespace with java-style "."
                size_t pos = 0;
                while( (pos = classname.find("::", pos)) !=
                       CXXTEST_STD(string::npos) )
                   classname.replace(pos, 2, ".");
                while ( ! classname.empty() && classname[0] == '.' )
                   classname.erase(0,1);

                //CXXTEST_STD(cout) << "HERE " << desc.file() << " " 
                //                  << classname << CXXTEST_STD(endl);

                //classname=desc.suiteName();
                //(*_o) << "file=\"" << desc.file() << "\" ";
                //(*_o) << "line=\"" << desc.line() << "\"";
                //_o->flush();
        }

        void leaveSuite( const SuiteDescription & )
        {
                std::list<TestCaseInfo>::iterator curr = info.begin();
                std::list<TestCaseInfo>::iterator end  = info.end();
                while (curr != end) {
                    if (curr->fail) nfail++;
                    if (curr->error) nerror++;
                    totaltime += curr->totalRuntime;
                    ntests++;
                    curr++;
                }
                curr = info.begin();
                end  = info.end();
                while (curr != end) {
                  (*curr).write(*_ostr);
                  curr++;
                }
                info.clear();
        }

        void enterTest( const TestDescription & desc )
        {
            #ifdef _WIN32
                testStartTime = clock();
            #else
                gettimeofday(&testStartTime, 0);
            #endif
                testcase = info.insert(info.end(),TestCaseInfo());
                testcase->testName = desc.testName();
                testcase->className = classname;
                std::ostringstream os;
                os << desc.line();
                testcase->line = os.str();

           if ( stream_redirect )
              CXXTEST_STD(cerr) << "ERROR: The stream_redirect != NULL" 
                                << CXXTEST_STD(endl);

           stream_redirect = 
              new TeeOutputStreams(CXXTEST_STD(cout), CXXTEST_STD(cerr));
        }

        /** Call this method before the run() call for a single test
         * Used for timing, mostly
         * */
        void enterRun( const TestDescription & /*desc*/ )
        {
          //Also CPU time.
          cpuStartTime = clock();

          // Record the time now.
          #ifdef _WIN32
            testRunStartTime = clock();
          #else
            gettimeofday(&testRunStartTime, 0);
          #endif
        }

        /** Call this method after the run() call, but before tearDown()
         * Used for timing, mostly
         * */
        void leaveRun( const TestDescription & /*desc*/ )
        {
          //Also CPU time.
          cpuStopTime = clock();

          // Record the time now.
          #ifdef _WIN32
            testRunStopTime = clock();
          #else
            gettimeofday(&testRunStopTime, 0);
          #endif
        }

        void leaveTest( const TestDescription & )
        {
           if ( stream_redirect != NULL )
           {
                std::string out = stream_redirect->out.str();
                if ( ! out.empty() )
                {
                   // silently ignore the '.'
                   if ( out[0] != '.' || out.size() > 1 )
                      testcase->add_element("system-out")->value << out;
                }
                if ( ! stream_redirect->err.str().empty() )
                   testcase->add_element("system-err")->value << stream_redirect->err.str();

                delete stream_redirect;
                stream_redirect = NULL;
           }


        #ifdef _WIN32
           const double testTime = double(clock() - testStartTime)/CLOCKS_PER_SEC;
           const double testRunTime = double(testRunStopTime - testRunStartTime)/CLOCKS_PER_SEC;
        #else
           gettimeofday(&testStopTime, 0);
           double sec = double(testStopTime.tv_sec - testStartTime.tv_sec);
           double usec = double(testStopTime.tv_usec - testStartTime.tv_usec);
           double testTime = sec + (usec / 1000000.0);

           sec = double(testRunStopTime.tv_sec - testRunStartTime.tv_sec);
           usec = double(testRunStopTime.tv_usec - testRunStartTime.tv_usec);
           double testRunTime = sec + (usec / 1000000.0);
        #endif

           // The CPU runtime, which on linux will be from all processors. Don't know about windows, think it's wall-clock time.
           double cpuTime = double(cpuStopTime - cpuStartTime)/CLOCKS_PER_SEC;
           // CPU fraction = what fraction of the CPU(s) was used.
           double CPUFraction = cpuTime / testRunTime;

           // Changed: we show the run() time, EXCLUDING the setup time.
           // Set the run time for this test
           testcase->runtime = testRunTime;
           // We still record both times for possible future use
           testcase->totalRuntime = testTime;
           // We still record both times for possible future use
           testcase->CPUFraction = CPUFraction;
        }

        void leaveWorld( const WorldDescription& desc )
        {
          std::ostringstream timeStream;
          timeStream << totaltime;
                (*_o) << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << endl;
                (*_o) << "<testsuite name=\"" << desc.worldName() << "\" ";
                (*_o) << " tests=\"" << ntests 
                      << "\" errors=\"" << nerror 
                      << "\" failures=\"" << nfail 
                      << "\" package=\"" << desc.worldName()
                      << "\" time=\"" << timeStream.str().c_str() << "\" >";
                _o->endl(*_o);
                (*_o) << _os->str().c_str();
                _os->clear();
                (*_o) << "</testsuite>" << endl;
                _o->flush();

        }

        void trace( const char* /*file*/, unsigned line, const char *expression )
        {
            throw_if_test_info_empty();
            element_t elt = testcase->add_element("trace");
            elt->add("line",line);
            elt->value << expression;
        }

        void warning( const char* /*file*/, unsigned line, const char *expression )
        {
            throw_if_test_info_empty();
            element_t elt = testcase->add_element("warning");
            elt->add("line",line);
            elt->value << expression;
        }

        void failedTest( const char* file, unsigned line, const char* expression )
        {
          testFailure( file, line, "failure") << "Test failed: " << expression;
        }

        void failedAssert( const char *file, unsigned line, const char *expression )
        {
            testFailure( file, line, "failedAssert" ) 
               << "Assertion failed: " << expression;
        }

        void failedAssertEquals( const char *file, unsigned line,
                                 const char* xStr, const char* yStr,
                                 const char *x, const char *y )
        {
            testFailure( file, line, "failedAssertEquals" )
               << "Error: Expected ("
               << xStr << " == " << yStr << "), found ("
               << x << " != " << y << ")";
        }

        void failedAssertSameData( const char *file, unsigned line,
                                   const char *xStr, const char *yStr, const char *sizeStr,
                                   const void* /*x*/, const void* /*y*/, unsigned size )
        {
            testFailure( file, line, "failedAssertSameData")
               << "Error: Expected " << sizeStr 
               << " (" << size << ")  bytes to be equal at ("
               << xStr << ") and (" << yStr << "), found";
        }

        void failedAssertSameFiles( const char *file, unsigned line,
                                   const char *, const char *,
                                   const char* explanation
                                   )
        {
            testFailure( file, line, "failedAssertSameFiles" )
               << "Error: " << explanation;
        }

        void failedAssertDelta( const char *file, unsigned line,
                                const char *xStr, const char *yStr, const char *dStr,
                                const char *x, const char *y, const char *d )
        {
            testFailure( file, line, "failedAssertDelta" )
               << "Error: Expected (" 
               << xStr << " == " << yStr << ") up to " << dStr 
               << " (" << d << "), found (" 
               << x << " != " << y << ")";
        }

        void failedAssertDiffers( const char *file, unsigned line,
                                  const char *xStr, const char *yStr,
                                  const char *value )
        {
            testFailure( file, line, "failedAssertDiffers" )
               << "Error: Expected (" 
               << xStr << " != " << yStr << "), found (" 
               << value << ")";
        }

        void failedAssertLessThan( const char *file, unsigned line,
                                   const char *xStr, const char *yStr,
                                   const char *x, const char *y )
        {
            testFailure( file, line, "failedAssertLessThan" )
               << "Error: Expected (" <<
               xStr << " < " << yStr << "), found (" <<
               x << " >= " << y << ")";
        }

        void failedAssertLessThanEquals( const char *file, unsigned line,
                                         const char *xStr, const char *yStr,
                                         const char *x, const char *y )
        {
            testFailure( file, line, "failedAssertLessThanEquals" )
               << "Error: Expected (" <<
               xStr << " <= " << yStr << "), found (" <<
               x << " > " << y << ")";
        }

        void failedAssertRelation( const char *file, unsigned line,
                                   const char *relation, const char *xStr, const char *yStr,
                                   const char *x, const char *y )
        {
            testFailure( file, line, "failedAssertRelation" )
               << "Error: Expected " << relation << "( " <<
               xStr << ", " << yStr << " ), found !" << relation 
               << "( " << x << ", " << y << " )";
        }

        void failedAssertPredicate( const char *file, unsigned line,
                                    const char *predicate, const char *xStr, const char *x )
        {
            testFailure( file, line, "failedAssertPredicate" )
               << "Error: Expected " << predicate << "( " <<
               xStr << " ), found !" << predicate << "( " << x << " )";
        }

        void failedAssertThrows( const char *file, unsigned line,
                                 const char *expression, const char *type,
                                 bool otherThrown )
        {
            testFailure( file, line, "failedAssertThrows" )
               << "Error: Expected (" << expression << ") to throw ("  <<
               type << ") but it " 
               << (otherThrown ? "threw something else" : "didn't throw");
        }

        void failedAssertThrowsNot( const char *file, unsigned line, const char *expression )
        {
            testFailure( file, line, "failedAssertThrowsNot" )
               << "Error: Expected (" << expression 
               << ") not to throw, but it did";
        }

    protected:

        OutputStream *outputStream() const
        {
            return _o;
        }

    private:
        XmlFormatter( const XmlFormatter & );
        XmlFormatter &operator=( const XmlFormatter & );

        void throw_if_test_info_empty()
        {
          if (info.empty())
          {
            throw std::runtime_error("Invalid use of TS_ASSERT statement outside of a test method.");
          }
        }

       std::stringstream& testFailure( const char* file, unsigned line, const char *failureType)
        {
            throw_if_test_info_empty();
            testcase->fail=true;
            element_t elt = testcase->update_element("failure");
            if ( elt->value.str().empty() )
            {
               elt->add("type",failureType);
               elt->add("line",line);
               elt->add("file",file);
            }
            else
               elt->value << CXXTEST_STD(endl);
            return elt->value;
            //failedTest(file,line,message.c_str());
        }

#if 0
        void attributeBinary( const char* name, const void *value, unsigned size )
        {
            (*_o) << name;
            (*_o) << "=\"";
            dump(value, size);
            (*_o) << "\" ";
        }

        void dump( const void *buffer, unsigned size )
        {
            if (!buffer) return;

            unsigned dumpSize = size;
            if ( maxDumpSize() && dumpSize > maxDumpSize() )
                dumpSize = maxDumpSize();

            const unsigned char *p = (const unsigned char *)buffer;
            for ( unsigned i = 0; i < dumpSize; ++ i )
                (*_o) << byteToHex( *p++ ) << " ";
            if ( dumpSize < size )
                (*_o) << "... ";
        }
#endif

        static void endl( OutputStream &o )
        {
            OutputStream::endl( o );
        }

        OutputStream *_o;
        OutputStream *_ostr;
        std::ostringstream *_os;

        TeeOutputStreams *stream_redirect;
    };
}

#endif // __CXXTEST__XMLFORMATTER_H

// Copyright 2008 Sandia Corporation. Under the terms of Contract
// DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government
// retains certain rights in this software.

