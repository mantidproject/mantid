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

using Mantid::DataHandling::ConvertFullprofToXML;

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;

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
    Poco::XML::Document* doc=0;
    TS_ASSERT_THROWS_NOTHING(doc = pParser.parseString(xmlText));
    TS_ASSERT(doc);
    if(doc) 
    {
      Poco::XML::Element* rootElem = doc->documentElement();
      TS_ASSERT(rootElem->hasChildNodes());
      Poco::XML::NodeList* componentLinkNodeList = rootElem->getElementsByTagName("component-link");; // get component-link elements
      size_t numComponentLinks = componentLinkNodeList->length();
      TS_ASSERT_EQUALS(numComponentLinks,3); // Three component-link elements expected
      if( numComponentLinks == 3)  // We only check the component-links if there are the expected number of them.
      { 
        // Whole Instrument
        Poco::XML::Element* componentLinkElem1 = static_cast<Poco::XML::Element*>(componentLinkNodeList->item(0));
        TS_ASSERT(componentLinkElem1);
        if(componentLinkElem1)
        {
          TS_ASSERT_EQUALS(componentLinkElem1->getAttribute("name"),"wholeInstrument");

          Poco::XML::NodeList* parameterNodeList = componentLinkElem1->getElementsByTagName("parameter"); // get parameter elements
          size_t numParameters = parameterNodeList->length();
          TS_ASSERT_EQUALS(numParameters,4); // Four parameter elements expected

          if( numParameters == 4) // We only check parameters if there are the expected number of them.
          { 
            Poco::XML::Element* paramElem1 = static_cast<Poco::XML::Element*>(parameterNodeList->item(0));
            do_test_paramemeter( paramElem1, "IkedaCarpenterPV:Alpha0", 0.000008, "TOF", "", true );

            Poco::XML::Element* paramElem2 = static_cast<Poco::XML::Element*>(parameterNodeList->item(1));
            do_test_paramemeter( paramElem2, "IkedaCarpenterPV:Beta0", 6.251096, "TOF", "", true );

            Poco::XML::Element* paramElem3 = static_cast<Poco::XML::Element*>(parameterNodeList->item(2));
            do_test_paramemeter( paramElem3, "IkedaCarpenterPV:Alpha1", 0.0, "TOF", "", true );

            Poco::XML::Element* paramElem4 = static_cast<Poco::XML::Element*>(parameterNodeList->item(3));
            do_test_paramemeter( paramElem4, "IkedaCarpenterPV:Kappa", 0.0, "", "", true );
          }

          parameterNodeList->release();  // Finished with parameter list
        }

        // Bank1
        Poco::XML::Element* componentLinkElem2 = static_cast<Poco::XML::Element*>(componentLinkNodeList->item(1));
        TS_ASSERT(componentLinkElem2);
        if(componentLinkElem2)
        {
          TS_ASSERT_EQUALS(componentLinkElem2->getAttribute("name"),"Bank1");

          Poco::XML::NodeList* parameterNodeList = componentLinkElem2->getElementsByTagName("parameter"); // get parameter elements
          size_t numParameters = parameterNodeList->length();
          TS_ASSERT_EQUALS(numParameters,2); // Two parameter elements expected

          if(numParameters== 2) // We only check parameters if there are the expected number of them.
          {
            Poco::XML::Element* paramElem1 = static_cast<Poco::XML::Element*>(parameterNodeList->item(0));
            do_test_paramemeter( paramElem1, "IkedaCarpenterPV:SigmaSquared", 0.0, "TOF^2", "dSpacing", false );

            Poco::XML::Element* paramElem2 = static_cast<Poco::XML::Element*>(parameterNodeList->item(1));
            do_test_paramemeter( paramElem2, "IkedaCarpenterPV:Gamma", 0.0, "TOF", "dSpacing", false );
          }

          parameterNodeList->release();  // Finished with parameter list
        }

        // Bank3
        Poco::XML::Element* componentLinkElem3 = static_cast<Poco::XML::Element*>(componentLinkNodeList->item(2));
        TS_ASSERT(componentLinkElem3);
        if(componentLinkElem3)
        {
          TS_ASSERT_EQUALS(componentLinkElem3->getAttribute("name"),"Bank3");

          Poco::XML::NodeList* parameterNodeList = componentLinkElem2->getElementsByTagName("parameter"); // get parameter elements
          size_t numParameters = parameterNodeList->length();
          TS_ASSERT_EQUALS(numParameters,2); // Two parameter elements expected

          if(numParameters== 2) // We only check parameters if there are the expected number of them.
          {
            Poco::XML::Element* paramElem1 = static_cast<Poco::XML::Element*>(parameterNodeList->item(0));
            do_test_paramemeter( paramElem1, "IkedaCarpenterPV:SigmaSquared", 0.0, "TOF^2", "dSpacing", false );

            Poco::XML::Element* paramElem2 = static_cast<Poco::XML::Element*>(parameterNodeList->item(1));
            do_test_paramemeter( paramElem2, "IkedaCarpenterPV:Gamma", 0.0, "TOF", "dSpacing", false );
          }

          parameterNodeList->release();  // Finished with parameter list
        }

        componentLinkNodeList->release(); // Finished with component-link list
        doc->release(); // Finished with document
      }
    }

    //Clean up
    Poco::File(inputFilename).remove();
    Poco::File(outputFilename).remove();

    return;
  }

    //----------------------------------------------------------------------------------------------
  /** Do a test on a parameter element
    */
  void do_test_paramemeter(const Poco::XML::Element* paramElem, const std::string& name, const double eq, const std::string& resultUnit, const std::string& unit, bool fixed )
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
             std::string eqString = formulaElem->getAttribute("eq"); // Don't yet know how to convert to double!
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
      ofile << "ALFBE        1.500      3.012      5.502      9.639                      \n";
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
