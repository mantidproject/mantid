//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/LoadSNSNexus.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FileValidator.h"
#include "MantidGeometry/Detector.h"
#include "MantidGeometry/ShapeFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/LogParser.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/PhysicalConstants.h"

#include <cmath>
#include <sstream>
#include <fstream>
#include <limits>

namespace Mantid
{
namespace NeXus
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadSNSNexus)

using namespace Kernel;
using namespace API;

/// Empty default constructor
LoadSNSNexus::LoadSNSNexus():m_L1(0) {}

/// Initialisation method.
void LoadSNSNexus::init()
{
    std::vector<std::string> exts;
    exts.push_back("NXS");
    exts.push_back("nxs");
    declareProperty("Filename","",new FileValidator(exts));
    declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output));

    BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
    mustBePositive->setLower(0);
    declareProperty("SpectrumMin", EMPTY_INT(), mustBePositive);
    declareProperty("SpectrumMax", EMPTY_INT(), mustBePositive->clone());
    declareProperty(new ArrayProperty<int>("SpectrumList"));
}

/** Executes the algorithm. Reading in the file and creating and populating
*  the output workspace
* 
*  @throw Exception::FileError If the Nexus file cannot be found/opened
*  @throw std::invalid_argument If the optional properties are set to invalid values
*/
void LoadSNSNexus::exec()
{
    // Create the root Nexus class
    NXRoot root(getPropertyValue("Filename"));

    int nPeriods = root.groups().size();
    WorkspaceGroup_sptr sptrWSGrp= WorkspaceGroup_sptr(new WorkspaceGroup);

    if (nPeriods > 1)
    {
        setProperty("OutputWorkspace",boost::dynamic_pointer_cast<Workspace>(sptrWSGrp));
    }

    double progress_step = 1.0/nPeriods;
    int period = 0;
    // Loop over entries loading each entry into a separate workspace
    for(std::vector<NXClassInfo>::const_iterator it=root.groups().begin();it!=root.groups().end();it++)
    if (it->nxclass == "NXentry")
    {
        ++period;
        double progress_start = (period-1)*progress_step;
        double progress_end = progress_start + progress_step;

        Workspace_sptr ws = loadEntry(root.openEntry(it->nxname),period,progress_start,progress_end);

        // Save the workspace property
        if (nPeriods == 1)
            setProperty("OutputWorkspace",ws);
        else // for higher periods create new workspace properties
        {
            std::ostringstream suffix;
            suffix << '_' << period;
            std::string wsName = getProperty("OutputWorkspace");
            wsName += suffix.str();
            std::string wsPropertyName = "OutputWorkspace" + suffix.str();
            declareProperty(new WorkspaceProperty<Workspace>(wsPropertyName,wsName,Direction::Output));
            setProperty(wsPropertyName,ws);
            sptrWSGrp->add(wsName);
        }
    }

}

/** Loads one entry from an SNS Nexus file
 *  @param entry The entry to read the data from
 *  @param period The period of the data
 *  @param progress_start The starting progress
 *  @param progress_end The ending progress
 *  @return A shared pointer to the created workspace
 */
API::Workspace_sptr LoadSNSNexus::loadEntry(NXEntry entry,int period, double progress_start, double progress_end)
{
    // To keep sorted bank names
    std::set<std::string,CompareBanks> banks;
    std::set<std::string> monitors;

    // Calculate the workspace dimensions
    int nSpectra = 0;
    int nBins = 0;
    
    for(std::vector<NXClassInfo>::const_iterator it=entry.groups().begin();it!=entry.groups().end();it++)
    if (it->nxclass == "NXdata") // Count detectors
    {
            NXData dataGroup = entry.openNXData(it->nxname);
            NXFloat data = dataGroup.openFloatData();
            if (data.rank() != 3) throw std::runtime_error("SNS NXdata is expected to be a 3D array");
            if (nBins == 0) nBins = data.dim2();
            nSpectra += data.dim0() * data.dim1();
            banks.insert(it->nxname); // sort the bank names
    }
    else if (it->nxclass == "NXmonitor") // Count monitors
    {
        nSpectra += 1;
        monitors.insert(it->nxname);
    }

    // Create the output workspace
    DataObjects::Workspace2D_sptr ws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (WorkspaceFactory::Instance().create("Workspace2D",nSpectra,nBins+1,nBins));
    ws->setTitle(entry.name());
    ws->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

    // Init loaded spectra counter
    int spec = 0;

    // Load monitor readings
    for(std::set<std::string>::const_iterator it=monitors.begin();it!=monitors.end();it++)
    {
        NXData dataGroup = entry.openNXData(*it);
        NXFloat timeBins = dataGroup.openNXFloat("time_of_flight");
        timeBins.load();
        MantidVec& X = ws->dataX(spec);
        X.assign(timeBins(),timeBins()+nBins+1);
        NXFloat data = dataGroup.openFloatData();
        data.load();
        MantidVec& Y = ws->dataY(spec);
        Y.assign(data(),data()+nBins);
        MantidVec& E = ws->dataE(spec);
        std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt);
        spec++;
    }

    int specData = spec;

    // Load detector readings
    Progress progress(this,progress_start,progress_end,nSpectra);
    for(std::set<std::string,CompareBanks>::const_iterator it=banks.begin();it!=banks.end();it++)
    {
        NXData dataGroup = entry.openNXData(*it);
        if (spec == specData)
        {
            NXFloat timeBins = dataGroup.openNXFloat("time_of_flight");
            timeBins.load();
            MantidVec& X = ws->dataX(spec);
            X.assign(timeBins(),timeBins()+nBins+1);
        }
        NXInt data = dataGroup.openIntData();
        for(int i = 0;i<data.dim0();i++)
            for(int j = 0; j < data.dim1(); j++)
            {
                if (spec > specData) ws->dataX(spec) = ws->dataX(specData);
                data.load(i,j);
                ////-- simulate input --
                //for(int k=0;k<nBins;k++)
                //    data()[k] = float(i*1000 + j*100 + k);
                ////--------------------
                MantidVec& Y = ws->dataY(spec);
                Y.assign(data(),data()+nBins);
                MantidVec& E = ws->dataE(spec);
                std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt);
                spec++;
                progress.report();
            }
    }

    boost::shared_array<int> spectra(new int[nSpectra]);
    for(int i=0;i<nSpectra;i++)
    {
        spectra[i] = i + 1;
        ws->getAxis(1)->spectraNo(i)= i+1;
    }
    ws->mutableSpectraMap().populate(spectra.get(),spectra.get(),nSpectra);

    NXFloat proton_charge = entry.openNXFloat("proton_charge");
    proton_charge.load();
    ws->getSample()->setProtonCharge(*proton_charge());

    loadInstrument(ws,entry,banks,monitors);

    return ws;
}

/** Creates a list of selected spectra to load from input interval and list properties.
 *  @return An integer vector with spectra numbers to load. If an empty vector is returned
 *  load all spectra in the file.
 */
std::vector<int> LoadSNSNexus::getSpectraSelection()
{
    std::vector<int> spec_list = getProperty("SpectrumList");
    int spec_max = getProperty("SpectrumMax");
    int spec_min = getProperty("SpectrumMin");
    bool is_list = !spec_list.empty();
    bool is_interval = (spec_max != EMPTY_INT());
    if ( spec_max == EMPTY_INT() ) spec_max = 0;

    // Compile a list of spectra numbers to load
    std::vector<int> spec;
    if( is_interval )
    {
        if ( spec_max < spec_min )
        {
            g_log.error("Invalid Spectrum min/max properties");
            throw std::invalid_argument("Inconsistent properties defined");
        }
        for(int i=spec_min;i<=spec_max;i++)
            spec.push_back(i);
        if (is_list)
        {
            for(size_t i=0;i<spec_list.size();i++)
            {
                int s = spec_list[i];
                if ( s < 0 ) continue;
                if (s < spec_min || s > spec_max)
                    spec.push_back(s);
            }
        }
    }
    else if (is_list)
    {
        spec_max=0;
        spec_min=std::numeric_limits<int>::max();
        for(size_t i=0;i<spec_list.size();i++)
        {
            int s = spec_list[i];
            if ( s < 0 ) continue;
            spec.push_back(s);
            if (s > spec_max) spec_max = s;
            if (s < spec_min) spec_min = s;
        }
    }
    else
    {
        //spec_min=0;
        //spec_max=nSpectra;
        //for(int i=spec_min;i<=spec_max;i++)
        //    spec.push_back(i);
    }

    return spec;
}

/** Loads the instrument from the SNS file
 *  @param localWS The workspace
 *  @param entry The entry in the Nexus file
 *  @param banks A sorted set of bank names
 *  @param monitors A set of monitor names
 */
void LoadSNSNexus::loadInstrument(API::Workspace_sptr localWS,
                            NXEntry entry,    
                            const std::set<std::string,CompareBanks>& banks,
                            const std::set<std::string>& monitors)

{
    DataObjects::Workspace2D_sptr ws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(localWS);

    // Clear off any existing instrument for this workspace
    ws->setInstrument(boost::shared_ptr<Instrument>(new Instrument));

    // Get reference to Instrument and set its name
    boost::shared_ptr<API::Instrument> instrument = ws->getBaseInstrument();

    std::string xmlSphere = "<sphere id=\"some-shape\">"
             "<centre x=\"0.0\"  y=\"0.0\" z=\"0.0\" />"
             "<radius val=\"0.03\" />"
             "</sphere>";

    Geometry::ShapeFactory shapeCreator;
    boost::shared_ptr<Geometry::Object> sphere = shapeCreator.createShape(xmlSphere);

    Geometry::ObjComponent *samplepos = new Geometry::ObjComponent("Sample",sphere,instrument.get());
    instrument->add(samplepos);
    instrument->markAsSamplePos(samplepos);
    samplepos->setPos(0.0,0.0,0.0);

    Geometry::ObjComponent *source = new Geometry::ObjComponent("Source",sphere,instrument.get());
    instrument->add(source);
    instrument->markAsSource(source);

    NXInstrument nxInstr = entry.openNXInstrument("instrument");
    NXChar name = nxInstr.openNXChar("name");
    name.load();
    instrument->setName(name());

    double l1;
    // If user has provided an L1, use that
    if ( ! Kernel::ConfigService::Instance().getValue("instrument.L1", l1) )
    {
        try // check if moderator/distance exists
        {
            NXFloat nxL1 = nxInstr.openNXFloat("moderator/distance");
            nxL1.load();
            l1 = *nxL1();
            if (m_L1 == 0.) m_L1 = l1;
        }
        catch(...) // hope that m_L1 has been saved earlier
        {
            l1 = m_L1;
        }
    }
    source->setPos(0.0,0.0,-1.0*l1);

    int det = 1;
    for(std::set<std::string>::const_iterator it=monitors.begin();it!=monitors.end();it++)
    {

        NXDouble z = entry.openNXDouble(*it + "/distance");
        z.load();
        Geometry::Detector *detector = new Geometry::Detector("mon",sphere,samplepos);
        Geometry::V3D pos(0,0,*z());

        detector->setPos(pos);

        // set detector ID, add copy to instrument and mark it
        detector->setID(det);
        instrument->add(detector);
        instrument->markAsMonitor(detector);
        det++;
    }

    for(std::set<std::string,CompareBanks>::const_iterator it=banks.begin();it!=banks.end();it++)
    {
        NXDetector nxDet = nxInstr.openNXDetector(*it);

        NXFloat detSize = nxDet.openNXFloat("origin/shape/size");
        detSize.load();

        double szX = detSize[0] != detSize[0] ? 0.001 : detSize[0]/2;
        double szY = detSize[1] != detSize[1] ? 0.001 : detSize[1]/2;
        double szZ = 0.001; //detSize[2] != detSize[2] ? 0.001 : detSize[2]/2;

        std::ostringstream xmlShapeStream;
        xmlShapeStream 
            << " <cuboid id=\"detector-shape\"> " 
            << "<left-front-bottom-point x=\""<<szX<<"\" y=\""<<-szY<<"\" z=\""<<-szZ<<"\"  /> " 
            << "<left-front-top-point  x=\""<<szX<<"\" y=\""<<-szY<<"\" z=\""<<szZ<<"\"  /> " 
            << "<left-back-bottom-point  x=\""<<-szX<<"\" y=\""<<-szY<<"\" z=\""<<-szZ<<"\"  /> " 
            << "<right-front-bottom-point  x=\""<<szX<<"\" y=\""<<szY<<"\" z=\""<<-szZ<<"\"  /> " 
            << "</cuboid>";

        std::string xmlShape(xmlShapeStream.str());
        boost::shared_ptr<Geometry::Object> shape = shapeCreator.createShape(xmlShape);

        Geometry::V3D shift; 
        Geometry::Quat rot;
        getBankOrientation(nxDet,shift,rot);

        NXFloat distance = nxDet.openDistance();
        distance.load();
        NXFloat azimuth = nxDet.openAzimuthalAngle();
        azimuth.load();
        NXFloat polar = nxDet.openPolarAngle();
        polar.load();
        for(int i=0;i<distance.dim0();i++)
        for(int j=0;j<distance.dim1();j++)
        {
            float r = distance(i,j);
            float angle = polar(i,j);
            float phi = azimuth(i,j);

            // check for a NaN
            if (r != r || angle != angle || phi != phi)
            {
                r = 0.;
                angle = 0.;
                phi = 0.;
            }

            // Create a new detector. Instrument will take ownership of pointer so no need to delete.
            Geometry::Detector *detector = new Geometry::Detector("det",shape,samplepos);
            Geometry::V3D pos;

            pos.spherical(r, angle/M_PI*180., phi/M_PI*180.);
            detector->setPos(pos);
            detector->setRot(rot);

            // set detector ID, add copy to instrument and mark it
            detector->setID(det);
            instrument->add(detector);
            instrument->markAsDetector(detector);
            det++;
        }
    }
}

/** Calculate rotation axis from direction cosines.
 */
//void LoadSNSNexus::calcRotation(double xx,double xy,double xz,double yx,double yy,double yz,double& angle, Geometry::V3D& axis)
void LoadSNSNexus::calcRotation(const Geometry::V3D& X,const Geometry::V3D& Y,const Geometry::V3D& Z,double& angle, Geometry::V3D& axis)
{
    //double angle1 = 10.;
    //Geometry::V3D axis1(1,2,0);
    //axis1.normalize();
    //std::cerr<<"axis1 "<<axis1<<' '<<angle1<<'\n';
    //Geometry::Quat rot(angle1,axis1);
    //Geometry::V3D X1(1,0,0);
    //Geometry::V3D Y1(0,1,0);
    //Geometry::V3D Z1(0,0,1);
    //rot.rotate(X1);
    //rot.rotate(Y1);
    //rot.rotate(Z1);

    double xx = X.X();
    double xy = X.Y();
    double xz = X.Z();

    double yx = Y.X();
    double yy = Y.Y();
    double yz = Y.Z();

    double zx = Z.X();
    double zy = Z.Y();
    double zz = Z.Z();

    //std::cerr<<xx<<' '<<xy<<' '<<xz<<'\n';
    //std::cerr<<yx<<' '<<yy<<' '<<yz<<'\n';
    //std::cerr<<zx<<' '<<zy<<' '<<zz<<'\n';

    if (xx == 1.)
    {
        axis(1,0,0);
        angle = acos(yy)/M_PI*180.;
        if (yz < 0) angle = -angle;
    }
    else if (yy == 1.)
    {
        axis(0,1,0);
        angle = acos(zz)/M_PI*180.;
        if (zx < 0) angle = -angle;
    }
    else if (zz == 1.)
    {
        axis(0,0,1);
        angle = acos(xx)/M_PI*180.;
        if (xy < 0) angle = -angle;
    }
    else
    {

        throw std::runtime_error("The case of arbitrary direction cosines is not implemented yet");

        double A1 = xx - 1. + xz*zx/(1.-zz);
        double A2 = xy + xz*zy/(1.-zz);
        double A = - A2 / A1;

        double B = sqrt( (1. - xx)/(1. - yy) );

        std::cerr<<"A,B="<<A<<' '<<B<<'\n';

        double cosY = sqrt( (1. - B*B)/(A*A - B*B) );
        double cosX = A*cosY;
        double cosZ = sqrt( 1. - cosX*cosX - cosY*cosY );

        Geometry::V3D N(cosX,cosY,cosZ);

        double tstX = N.scalar_prod(X);
        double tstY = N.scalar_prod(Y);
        double tstZ = N.scalar_prod(Z);

        std::cerr<<"Test:\n";
        std::cerr<<cosX<<' '<<cosY<<' '<<cosZ<<'\n';
        std::cerr<<tstX<<' '<<tstY<<' '<<tstZ<<'\n';

        axis(cosX,cosY,cosZ);

    }


}

/**  Get detector position and orientation
 *   @param nxDet The NXDetector element
 *   @param shift The translation of the detector
 *   @param rot The orientation of the detector
 */
void LoadSNSNexus::getBankOrientation(NXDetector nxDet, Geometry::V3D& shift, Geometry::Quat& rot)
{
    NXFloat translation = nxDet.openNXFloat("origin/translation/distance");
    translation.load();
    shift = Geometry::V3D(translation[0],translation[1],translation[2]);

    NXFloat orientation = nxDet.openNXFloat("origin/orientation/value");
    orientation.load();
    double angle;
    Geometry::V3D axis;

    Geometry::V3D X(orientation[0],orientation[1],orientation[2]);
    Geometry::V3D Y(orientation[3],orientation[4],orientation[5]);
    Geometry::V3D Z = X.cross_prod(Y);

    calcRotation(X,Y,Z,angle,axis);

    std::cerr<<"rot axis "<<axis<<'\n';
    std::cerr<<"angle "<<angle<<'\n';

    rot = Geometry::Quat(angle,axis);
}

double LoadSNSNexus::dblSqrt(double in)
{
    return sqrt(in);
}
} // namespace DataHandling
} // namespace Mantid
