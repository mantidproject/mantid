#ifndef MANTID_DATAHANDLING_CONVERTFULLPROFTOXMLTEST_H_
#define MANTID_DATAHANDLING_CONVERTFULLPROFTOXMLTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/ConvertFullprofToXML.h"
#include <fstream>
#include <Poco/File.h>

#include <Poco/DOM/Document.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/AutoPtr.h>

using Mantid::DataHandling::ConvertFullprofToXML;

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Poco::XML;

using namespace std;

class ConvertFullprofToXMLTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertFullprofToXMLTest *createSuite() { return new ConvertFullprofToXMLTest(); }
  static void destroySuite( ConvertFullprofToXMLTest *suite ) { delete suite; }

    //----------------------------------------------------------------------------------------------
  /** Test conversion
    */
  void testInit()
  {

    // Init LoadFullprofResolution
    ConvertFullprofToXML alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test conversion
    */
  void testExec()
  {
    // Generate file
    string inputFilename("TestConvertFullprofToXMLInput.irf");
    string outputFilename("TestConvertFullprofToXMLOutput.xml");
    generate2BankIrfFile(inputFilename);

    // Init LoadFullprofResolution
    ConvertFullprofToXML alg;
    alg.initialize();

    // Set up
    alg.setProperty("InputFilename", inputFilename);
    alg.setProperty("InstrumentName","POWGEN");
    alg.setProperty("OutputFileName", outputFilename);

    // Execute
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // has the algorithm written a file to disk?
    outputFilename = alg.getPropertyValue("outputFilename"); //Get absolute path
    TS_ASSERT( Poco::File(outputFilename).exists() );

    // Check output file
    std::string xmlText = Strings::loadFile(outputFilename);
    Poco::XML::DOMParser pParser;
    AutoPtr<Document> doc=0;
    TS_ASSERT_THROWS_NOTHING(doc = pParser.parseString(xmlText));
    TS_ASSERT(doc);
    if(doc) 
    {
      AutoPtr<Element> rootElem = doc->documentElement();
      TS_ASSERT(rootElem->hasChildNodes());
      AutoPtr<NodeList> componentLinkNodeList = rootElem->getElementsByTagName("component-link");; // get component-link elements
      size_t numComponentLinks = componentLinkNodeList->length();
      TS_ASSERT_EQUALS(numComponentLinks,3); // Three component-link elements expected
      if( numComponentLinks == 3)  // We only check the component-links if there are the expected number of them.
      { 
        // Whole Instrument
        AutoPtr<Element> componentLinkElem1 = static_cast<Poco::XML::Element*>(componentLinkNodeList->item(0));
        TS_ASSERT(componentLinkElem1);
        if(componentLinkElem1)
        {
          TS_ASSERT_EQUALS(componentLinkElem1->getAttribute("name"),"POWGEN");

          AutoPtr<NodeList> parameterNodeList = componentLinkElem1->getElementsByTagName("parameter"); // get parameter elements
          size_t numParameters = parameterNodeList->length();
          TS_ASSERT_EQUALS(numParameters,4); // Four parameter elements expected

          if( numParameters == 4) // We only check parameters if there are the expected number of them.
          { 
            AutoPtr<Element> paramElem1 = static_cast<Poco::XML::Element*>(parameterNodeList->item(0));
            do_test_paramemeter( paramElem1, "IkedaCarpenterPV:Alpha0", 0.000008, 0.0,"TOF", "", true );

            AutoPtr<Element> paramElem2 = static_cast<Poco::XML::Element*>(parameterNodeList->item(1));
            do_test_paramemeter( paramElem2, "IkedaCarpenterPV:Beta0", 6.251096, 0.0, "TOF", "", true );

            AutoPtr<Element> paramElem3 = static_cast<Poco::XML::Element*>(parameterNodeList->item(2));
            do_test_paramemeter( paramElem3, "IkedaCarpenterPV:Alpha1", 0.1, 0.0, "TOF", "", true );

            AutoPtr<Element> paramElem4 = static_cast<Poco::XML::Element*>(parameterNodeList->item(3));
            do_test_paramemeter( paramElem4, "IkedaCarpenterPV:Kappa", -0.1, 0.0, "", "", true );
          }

          parameterNodeList->release();  // Finished with parameter list
        }

        // Bank1
        AutoPtr<Element> componentLinkElem2 = static_cast<Poco::XML::Element*>(componentLinkNodeList->item(1));
        TS_ASSERT(componentLinkElem2);
        if(componentLinkElem2)
        {
          TS_ASSERT_EQUALS(componentLinkElem2->getAttribute("name"),"bank1");

          AutoPtr<NodeList> parameterNodeList = componentLinkElem2->getElementsByTagName("parameter"); // get parameter elements
          size_t numParameters = parameterNodeList->length();
          TS_ASSERT_EQUALS(numParameters,2); // Two parameter elements expected

          if(numParameters== 2) // We only check parameters if there are the expected number of them.
          {
            AutoPtr<Element> paramElem1 = static_cast<Poco::XML::Element*>(parameterNodeList->item(0));
            do_test_paramemeter( paramElem1, "IkedaCarpenterPV:SigmaSquared", 0.00044, 0.355, "TOF^2", "dSpacing", false );

            AutoPtr<Element> paramElem2 = static_cast<Poco::XML::Element*>(parameterNodeList->item(1));
            do_test_paramemeter( paramElem2, "IkedaCarpenterPV:Gamma", 0.0, 0.0, "TOF", "dSpacing", false );
          }

          parameterNodeList->release();  // Finished with parameter list
        }

        // Bank3
        AutoPtr<Element> componentLinkElem3 = static_cast<Poco::XML::Element*>(componentLinkNodeList->item(2));
        TS_ASSERT(componentLinkElem3);
        if(componentLinkElem3)
        {
          TS_ASSERT_EQUALS(componentLinkElem3->getAttribute("name"),"bank3");

          AutoPtr<NodeList> parameterNodeList = componentLinkElem3->getElementsByTagName("parameter"); // get parameter elements
          size_t numParameters = parameterNodeList->length();
          TS_ASSERT_EQUALS(numParameters,2); // Two parameter elements expected

          if(numParameters== 2) // We only check parameters if there are the expected number of them.
          {
            AutoPtr<Element> paramElem1 = static_cast<Poco::XML::Element*>(parameterNodeList->item(0));
            do_test_paramemeter( paramElem1, "IkedaCarpenterPV:SigmaSquared", 10.0, 0.0, "TOF^2", "dSpacing", false );

            AutoPtr<Element> paramElem2 = static_cast<Poco::XML::Element*>(parameterNodeList->item(1));
            do_test_paramemeter( paramElem2, "IkedaCarpenterPV:Gamma", 2.742, 0.0, "TOF", "dSpacing", false );
          }
        }
      }
    }

    //Clean up
    Poco::File(inputFilename).remove();
    Poco::File(outputFilename).remove();

    return;
  }

  //---------------------------------------------------------------------------------------------
  /** Test missing instrument parameter
  */
  void testMissingInstrument()
  {
    // Generate file
    string inputFilename("TestConvertFullprofToXMLInput.irf");
    string outputFilename("TestConvertFullprofToXMLMissingInstrumentOutput.xml");
    generate2BankIrfFile(inputFilename);

    // Init LoadFullprofResolution
    ConvertFullprofToXML alg;
    alg.initialize();

    // Set up
    alg.setProperty("InputFilename", inputFilename);
    alg.setProperty("InstrumentName","");
    alg.setProperty("OutputFileName", outputFilename);

    // Execute
    TS_ASSERT(! alg.execute());

    // Not only should algorithm fail, but also writes nothing to file.
    outputFilename = alg.getPropertyValue("outputFilename"); //Get absolute path
    TS_ASSERT( !Poco::File(outputFilename).exists() );

   //Clean up
    Poco::File(inputFilename).remove();
  }

  //----------------------------------------------------------------------------------------------
  /** Do test on a parameter element
  ** parameElem: parameter element to be tested
  ** name:       expected name of parameter element to be tested 
  ** eq1:        expected value of first double
  ** eq2:        expected value of second double, if expected
  ** resultUnit: expected value of result-unit
  ** unit:       expected value of unit
  ** fixed:      true if parameter is expected to be fixed
  */
  void do_test_paramemeter(const Poco::XML::Element* paramElem, const std::string& name, const double eq1, const double eq2, const std::string& resultUnit, const std::string& unit, bool fixed )
  {
    TS_ASSERT(paramElem);
    if(paramElem)
    {
      TS_ASSERT_EQUALS(paramElem->getAttribute("type"),"fitting");
      TS_ASSERT_EQUALS(paramElem->getAttribute("name"),name);
      Poco::XML::Element* formulaElem = paramElem->getChildElement("formula");
      TS_ASSERT(formulaElem);
      if(formulaElem)
      {
        std::string eqString = formulaElem->getAttribute("eq"); 
        do_test_eq_value (eqString, name, eq1, eq2 );
        TS_ASSERT_EQUALS(formulaElem->getAttribute("result-unit"),resultUnit);
        TS_ASSERT_EQUALS(formulaElem->getAttribute("unit"),unit);
      }
      Poco::XML::Element* fixedElem = paramElem->getChildElement("fixed");
      if(fixed)
      {
        TS_ASSERT(fixedElem);
      } 
      else
      {
        TS_ASSERT(!fixedElem);
      }
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Do test on the eq value of given parameter element.
  **  eqValue: value to be tested
  **  name:    name of parameter element to be tested (determines format of eqValue)
  **  eq1:     expected value of first double in eqValue
  **  eq2:     expected value of second double in eqValue, if expected
  */
  void do_test_eq_value (const std::string& eqValue, const std::string& name, const double eq1, const double eq2)
  {
    if(name == "IkedaCarpenterPV:SigmaSquared")
    {
      // eqValue string expected to be something like "0.00043999999999999996*centre^2+0.35499999999999993"
      size_t endEq1 = eqValue.find("*centre^2+",1);
      if(endEq1 == std::string::npos) TS_FAIL("'*centre^2+' not found in the value of 'eq' for Sigma squared.");
      else 
      {
        std::string eq1Value = eqValue.substr(0,endEq1);
        std::string eq2Value = eqValue.substr(endEq1+10,std::string::npos);
        double eq1v = parse_double(eq1Value);
        TS_ASSERT_DELTA( eq1v, eq1, 0.0000001);
        double eq2v = parse_double(eq2Value);
        TS_ASSERT_DELTA( eq2v, eq2, 0.0000001);
      }
    }
    else if (name == "IkedaCarpenterPV:Gamma") 
    {
      // eqValue string expected to be something like "2.742*centre"
      size_t endEq1 = eqValue.find("*centre",1);
      if(endEq1 == std::string::npos) TS_FAIL("'*centre' not found in the value of 'eq' for Gamma.");
      else
      {
        std::string eq1Value = eqValue.substr(0,endEq1);
        double eq1v = parse_double(eq1Value);
        TS_ASSERT_DELTA( eq1v, eq1, 0.0000001)
      }
    }
    else
    {
      // eqValue string expected to be just a double
      double eqv = parse_double(eqValue);
      TS_ASSERT_DELTA( eqv, eq1, 0.0000001)
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Read a double value from a string and test success of this
  */
  double parse_double ( const std::string& value )
  {
    try
    {
      return boost::lexical_cast<double>( value ) ;
    }
    catch(boost::bad_lexical_cast&)
    { 
      std::string errorMessage = "Can't read double from '"+value+"'.";
      TS_FAIL(errorMessage);
      return 0.0; 
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Generate a 2 bank .irf file
  */
  void generate2BankIrfFile(string filename)
  {
    ofstream ofile;
    ofile.open(filename.c_str());

    if (ofile.is_open())
    {
      ofile << "  Instrumental resolution function for POWGEN/SNS  A Huq  2013-12-03  ireso: 6 \n";
      ofile << "! For use in testing ConvertFullprofToXML        (Res=6)                       \n";
      ofile << "! ----------------------------------------------  Bank 1  CWL =   0.5330A      \n";
      ofile << "!  Type of profile function: back-to-back exponentials * pseudo-Voigt          \n";
      ofile << "!       Tof-min(us)    step      Tof-max(us)                                   \n";
      ofile << "TOFRG   5000.2300      4.0002  51000.0000                                      \n";
      ofile << "!          Zero    Dtt1                                                        \n";
      ofile << "ZD2TOF     -1.00   22580.59157                                                 \n";
      ofile << "!          Zerot   Dtt1t         Dtt2t    x-cross Width                        \n";
      ofile << "ZD2TOT  933.50214   22275.21084     1.0290  0.0000002  5.0957                  \n";
      ofile << "!     TOF-TWOTH of the bank                                                    \n";
      ofile << "TWOTH     90.00                                                                \n";
      ofile << "!           Sig-2       Sig-1       Sig-0                                      \n";
      ofile << "SIGMA     514.546      0.00044      0.355                                      \n";
      ofile << "!           Gam-2       Gam-1       Gam-0                                      \n";
      ofile << "GAMMA       0.000       0.000       0.000                                      \n";
      ofile << "!         alph0       beta0       alph1       beta1                            \n";
      ofile << "ALFBE    0.000008    6.251096    0.100000   -0.100000                          \n";
      ofile << "!         alph0t      beta0t      alph1t      beta1t                           \n";
      ofile << "ALFBT   0.010156   85.918922    0.000000    0.000000                           \n";
      ofile << "END                                                                            \n";
      ofile << "! ----------------------------------------------  Bank 3                 \n";
      ofile << "!  Type of profile function: back-to-back exponentials * pseudo-Voigt    \n";
      ofile << "!       Tof-min(us)    step      Tof-max(us)                             \n";
      ofile << "TOFRG   9800.0000      5.0000   86000.0000                               \n";
      ofile << "!       Zero   Dtt1                                                      \n";
      ofile << "ZD2TOF     0.00  22586.10156                                             \n";
      ofile << "!       Zerot    Dtt1t       Dtt2t    x-cross    Width                   \n";
      ofile << "ZD2TOT -42.76068   22622.76953    0.30    0.3560    2.4135               \n";
      ofile << "!     TOF-TWOTH of the bank                                              \n";
      ofile << "TWOTH    90.000                                                          \n";
      ofile << "!       Sig-2     Sig-1     Sig-0                                        \n";
      ofile << "SIGMA  72.366    10.000     0.000                                        \n";
      ofile << "!       Gam-2     Gam-1     Gam-0                                        \n";
      ofile << "GAMMA     0.000     2.742      0.000                                     \n";
      ofile << "!          alph0       beta0       alph1       beta1                     \n";
      ofile << "ALFBE    0.000008    6.251096    0.100000   -0.100000                    \n";
      ofile << "!         alph0t      beta0t      alph1t      beta1t                     \n";
      ofile << "ALFBT       86.059     96.487     13.445      3.435                      \n";

      ofile.close();
    }
    else
    {
      throw runtime_error("Unable to open file to write.");
    }

    return;
  }

};


#endif /* MANTID_DATAHANDLING_CONVERTFULLPROFTOXMLTEST_H_ */
