#include "MantidDataHandling/LoadNXSPE.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/SpectraAxis.h"

#include <nexus/NeXusFile.hpp>
#include <nexus/NeXusException.hpp>
#include "MantidNexus/NexusClasses.h"


#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidGeometry/Surfaces/Sphere.h"

#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <boost/regex.hpp>

namespace Mantid
{
  namespace DataHandling
  {

    DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadNXSPE);

    using namespace Mantid::Kernel;
    using namespace Mantid::API;

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    LoadNXSPE::LoadNXSPE()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    LoadNXSPE::~LoadNXSPE()
    {
    }

    //----------------------------------------------------------------------------------------------

    /**
     * Calculate the confidence in the string value. This is used for file identification.
     * @param value
     * @return confidence 0 - 100%
     */
    int LoadNXSPE::identiferConfidence(const std::string& value)
    {
      int confidence = 0;
      if (value.compare("NXSPE") == 0)
      {
        confidence = 99;
      }
      else
      {
        boost::regex re("^NXSP", boost::regex::icase);
        if (boost::regex_match(value, re))
        {
          confidence = 95;
        }
      }
      return confidence;
    }

    /**
     * Return the confidence with with this algorithm can load the file
     * @param descriptor A descriptor for the file
     * @returns An integer specifying the confidence level. 0 indicates it will not be used
     */
    int LoadNXSPE::confidence(Kernel::NexusDescriptor & descriptor) const
    {
      int confidence(0);
      typedef std::map<std::string, std::string> string_map_t;
      try
      {
        ::NeXus::File file = ::NeXus::File(descriptor.filename());
        string_map_t entries = file.getEntries();
        for (string_map_t::const_iterator it = entries.begin(); it != entries.end(); ++it)
        {
          if (it->second == "NXentry")
          {
            file.openGroup(it->first, it->second);
            file.openData("definition");
            const std::string value = file.getStrData();
            confidence = identiferConfidence(value);
          }
        }
      } catch (::NeXus::Exception&)
      {
      }
      return confidence;
    }

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
     */
    void LoadNXSPE::init()
    {
      std::vector<std::string> exts;
      exts.push_back(".nxspe");
      exts.push_back("");
      declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts), "An NXSPE file");
      declareProperty(new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
          "The name of the workspace that will be created.");
    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
     */
    void LoadNXSPE::exec()
    {
      std::string filename = getProperty("Filename");
      //quicly check if it's really nxspe
      try
      {
        ::NeXus::File file(filename);
        std::string mainEntry = (*(file.getEntries().begin())).first;
        file.openGroup(mainEntry, "NXentry");
        file.openData("definition");
        if (identiferConfidence(file.getStrData()) < 1)
        {
          throw std::invalid_argument("Not NXSPE");
        }
        file.close();
      }
      catch (...)
      {
        throw std::invalid_argument("Not NeXus or not NXSPE");
      }

      //Load the data
      ::NeXus::File file(filename);

      std::string mainEntry = (*(file.getEntries().begin())).first;
      file.openGroup(mainEntry, "NXentry");

      file.openGroup("NXSPE_info", "NXcollection");
      std::map<std::string, std::string> entries = file.getEntries();
      std::vector<double> temporary;
      double fixed_energy, psi = 0.;

      if (!entries.count("fixed_energy"))
      {
        throw std::invalid_argument("fixed_energy field was not found");
      }
      file.openData("fixed_energy");
      file.getData(temporary);
      fixed_energy = temporary.at(0);
      file.closeData();

      if (entries.count("psi"))
      {
        file.openData("psi");
        file.getData(temporary);
        psi = temporary.at(0);
        file.closeData();
      }

      int kikfscaling = 0;
      if (entries.count("ki_over_kf_scaling"))
      {
        file.openData("ki_over_kf_scaling");
        std::vector<int> temporaryint;
        file.getData(temporaryint);
        kikfscaling = temporaryint.at(0);
        file.closeData();
      }

      file.closeGroup(); //NXSPE_Info

      file.openGroup("data", "NXdata");
      entries = file.getEntries();

      if (!entries.count("data"))
      {
        throw std::invalid_argument("data field was not found");
      }
      file.openData("data");
      ::NeXus::Info info = file.getInfo();
      std::size_t numSpectra = static_cast<std::size_t>(info.dims.at(0));
      std::size_t numBins = static_cast<std::size_t>(info.dims.at(1));
      std::vector<double> data;
      file.getData(data);
      file.closeData();

      if (!entries.count("error"))
      {
        throw std::invalid_argument("error field was not found");
      }
      file.openData("error");
      std::vector<double> error;
      file.getData(error);
      file.closeData();

      if (!entries.count("energy"))
      {
        throw std::invalid_argument("energy field was not found");
      }
      file.openData("energy");
      std::vector<double> energies;
      file.getData(energies);
      file.closeData();

      if (!entries.count("azimuthal"))
      {
        throw std::invalid_argument("azimuthal field was not found");
      }
      file.openData("azimuthal");
      std::vector<double> azimuthal;
      file.getData(azimuthal);
      file.closeData();

      if (!entries.count("azimuthal_width"))
      {
        throw std::invalid_argument("azimuthal_width field was not found");
      }
      file.openData("azimuthal_width");
      std::vector<double> azimuthal_width;
      file.getData(azimuthal_width);
      file.closeData();

      if (!entries.count("polar"))
      {
        throw std::invalid_argument("polar field was not found");
      }
      file.openData("polar");
      std::vector<double> polar;
      file.getData(polar);
      file.closeData();

      if (!entries.count("polar_width"))
      {
        throw std::invalid_argument("polar_width field was not found");
      }
      file.openData("polar_width");
      std::vector<double> polar_width;
      file.getData(polar_width);
      file.closeData();

      //distance might not have been saved in all NXSPE files
      std::vector<double> distance;
      if (entries.count("distance"))
      {
        file.openData("distance");
        file.getData(distance);
        file.closeData();
      }

      file.closeGroup(); //data group
      file.closeGroup(); //Main entry
      file.close();

      //check if dimensions of the vectors are correct
      if ((error.size() != data.size()) || (azimuthal.size() != numSpectra)
          || (azimuthal_width.size() != numSpectra) || (polar.size() != numSpectra)
          || (polar_width.size() != numSpectra)
          || ((energies.size() != numBins) && (energies.size() != numBins + 1)))
      {
        throw std::invalid_argument("incompatible sizes of fields in the NXSPE file");
      }

    MatrixWorkspace_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>
        (WorkspaceFactory::Instance().create("Workspace2D",numSpectra,energies.size(),numBins));
    // Need to get hold of the parameter map
    Geometry::ParameterMap& pmap = outputWS->instrumentParameters();
    outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("DeltaE");
    outputWS->setYUnit("SpectraNumber");

    std::vector<double>::iterator itdata=data.begin(),iterror=error.begin(),itdataend,iterrorend;
    API::Progress prog = API::Progress(this, 0.0, 0.9, numSpectra);
    for (std::size_t i=0; i<numSpectra; ++i)
    {
      itdataend=itdata+numBins;
      iterrorend=iterror+numBins;
      outputWS->dataX(i)=energies;
      if (((*itdata)==std::numeric_limits<double>::quiet_NaN())||(*itdata<=-1e10))//masked bin
      {
        outputWS->dataY(i)=std::vector<double>(numBins,0);
        outputWS->dataE(i)=std::vector<double>(numBins,0);
        pmap.addBool(outputWS->getDetector(i).get(),"masked",true);
      }
      else
      {
        outputWS->dataY(i)=std::vector<double>(itdata,itdataend);
        outputWS->dataE(i)=std::vector<double>(iterror,iterrorend);
      }
      itdata=(itdataend);
      iterror=(iterrorend);
      prog.report();
    }

      //add logs
      outputWS->mutableRun().addLogData(new PropertyWithValue<double>("Ei", fixed_energy));
      outputWS->mutableRun().addLogData(new PropertyWithValue<double>("psi", psi));
      outputWS->mutableRun().addLogData(
          new PropertyWithValue<std::string>("ki_over_kf_scaling", kikfscaling == 1 ? "true" : "false"));

      //Set Goniometer
      Geometry::Goniometer gm;
      gm.pushAxis("psi", 0, 1, 0, psi);
      outputWS->mutableRun().setGoniometer(gm, true);

      //generate instrument
      Geometry::Instrument_sptr instrument(new Geometry::Instrument("NXSPE"));
      outputWS->setInstrument(instrument);

      Geometry::ObjComponent *source = new Geometry::ObjComponent("source");
      source->setPos(0.0, 0.0, -10.0);
      instrument->add(source);
      instrument->markAsSource(source);
      Geometry::ObjComponent *sample = new Geometry::ObjComponent("sample");
      instrument->add(sample);
      instrument->markAsSamplePos(sample);

      Geometry::Object_const_sptr cuboid(createCuboid(0.1, 0.1, 0.1)); //FIXME: memory hog on rendering. Also, make each detector separate size
      for (std::size_t i = 0; i < numSpectra; ++i)
      {
        double r = 1.0;
        if (!distance.empty())
        {
          r = distance.at(i);
        }

        Kernel::V3D pos;
        pos.spherical(r, polar.at(i), azimuthal.at(i));

        Geometry::Detector *det = new Geometry::Detector("pixel", static_cast<int>(i + 1), sample);
        det->setPos(pos);
        det->setShape(cuboid);
        instrument->add(det);
        instrument->markAsDetector(det);
      }

      setProperty("OutputWorkspace", outputWS);
    }

    Geometry::Object_sptr LoadNXSPE::createCuboid(double dx, double dy, double dz)
    {

      dx = 0.5 * std::fabs(dx);
      dy = 0.5 * std::fabs(dy);
      dz = 0.5 * std::fabs(dz);
      /*
       std::stringstream planeName;

       planeName.str("px ");planeName<<-dx;
       std::string C1=planeName.str();
       planeName.str("px ");planeName<<dx;
       std::string C2=planeName.str();
       planeName.str("px ");planeName<<-dy;
       std::string C3=planeName.str();
       planeName.str("px ");planeName<<dy;
       std::string C4=planeName.str();
       planeName.str("px ");planeName<<-dz;
       std::string C5=planeName.str();
       planeName.str("px ");planeName<<dz;
       std::string C6=planeName.str();

       // Create surfaces
       std::map<int,Geometry::Surface*> CubeSurMap;
       CubeSurMap[1]=new Geometry::Plane();
       CubeSurMap[2]=new Geometry::Plane();
       CubeSurMap[3]=new Geometry::Plane();
       CubeSurMap[4]=new Geometry::Plane();
       CubeSurMap[5]=new Geometry::Plane();
       CubeSurMap[6]=new Geometry::Plane();

       CubeSurMap[1]->setSurface(C1);
       CubeSurMap[2]->setSurface(C2);
       CubeSurMap[3]->setSurface(C3);
       CubeSurMap[4]->setSurface(C4);
       CubeSurMap[5]->setSurface(C5);
       CubeSurMap[6]->setSurface(C6);
       CubeSurMap[1]->setName(1);
       CubeSurMap[2]->setName(2);
       CubeSurMap[3]->setName(3);
       CubeSurMap[4]->setName(4);
       CubeSurMap[5]->setName(5);
       CubeSurMap[6]->setName(6);

       // Cube (id 68)
       // using surface ids:  1-6
       std::string ObjCube="1 -2 3 -4 5 -6";

       Geometry::Object_sptr retVal = Geometry::Object_sptr(new Geometry::Object);
       retVal->setObject(68,ObjCube);
       retVal->populate(CubeSurMap);
       */
      std::string S41 = "so 0.01";         // Sphere at origin radius 0.01

      // First create some surfaces
      std::map<int, Geometry::Surface*> SphSurMap;
      SphSurMap[41] = new Geometry::Sphere();
      SphSurMap[41]->setSurface(S41);
      SphSurMap[41]->setName(41);

      // A sphere
      std::string ObjSphere = "-41";
      Geometry::Object_sptr retVal = Geometry::Object_sptr(new Geometry::Object);
      retVal->setObject(41, ObjSphere);
      retVal->populate(SphSurMap);

      return retVal;
    }

  } // namespace Mantid
} // namespace DataHandling
