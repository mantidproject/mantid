#include "PanelsSurface.h"
#include "GLActorVisitor.h"
#include "CompAssemblyActor.h"
#include "ObjCompAssemblyActor.h"
#include "RectangularDetectorActor.h"

#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidKernel/V3D.h"

#include <QCursor>
#include <QMessageBox>
#include <QApplication>

using namespace Mantid::Geometry;

PanelsSurface::PanelsSurface(const InstrumentActor* rootActor,const Mantid::Kernel::V3D& origin,const Mantid::Kernel::V3D& axis):
    UnwrappedSurface(rootActor),
    m_pos(origin),
    m_zaxis(axis)
{
    init();
}

/**
 * Initialize the surface.
 */
void PanelsSurface::init()
{
    m_unwrappedDetectors.clear();
    m_assemblies.clear();

    size_t ndet = m_instrActor->ndetectors();
    m_unwrappedDetectors.resize(ndet);
    if (ndet == 0) return;

    // Pre-calculate all the detector positions (serial because
    // I suspect the IComponent->getPos() method to not be properly thread safe)
    m_instrActor->cacheDetPos();

    Instrument_const_sptr inst = m_instrActor->getInstrument();

    findFlatBanks();

    m_u_min = 0;
    m_u_max = 1;
    m_v_min = 0;
    m_v_max = 1;
    m_height_max = 0.1;
    m_width_max = 0.1;
    m_viewRect = RectF( QPointF(m_u_min,m_v_min), QPointF(m_u_max,m_v_max) );

}

void PanelsSurface::project(const Mantid::Kernel::V3D &pos, double &u, double &v, double &uscale, double &vscale) const
{
    (void)pos;
    u = v = uscale = vscale = 0.0;
}

void PanelsSurface::rotate(const UnwrappedDetector &udet, Mantid::Kernel::Quat &R) const
{
    (void)udet;
    (void)R;
}

//-----------------------------------------------------------------------------------------------//

class FlatBankFinder: public GLActorConstVisitor
{
    PanelsSurface &m_surface;
public:
    FlatBankFinder(PanelsSurface &surface):m_surface(surface){}

    bool visit(const GLActor*){return false;}
    bool visit(const GLActorCollection*){return false;}
    bool visit(const ComponentActor*){return false;}
    bool visit(const InstrumentActor*){return false;}
    bool visit(const ObjCompAssemblyActor*){return false;}

    bool visit(const CompAssemblyActor* actor)
    {
        auto assembly = actor->getCompAssembly();
        assert(assembly);
        size_t nelem = static_cast<size_t>(assembly->nelements());
        // assemblies with one element cannot be flat (but its element can be)
        if ( nelem == 1 )
        {
            //std::cerr << "Single element, out" << std::endl;
            return false;
        }
        int ndetectors = 0;
        QList<ComponentID> objCompAssemblies;
        // normal to the plane, undefined at first
        Mantid::Kernel::V3D normal(0,0,0);
        Mantid::Kernel::V3D x,y,pos;
        for(size_t i = 0; i < nelem; ++i)
        {
            auto elem = assembly->getChild(i);
            ObjCompAssembly* objCompAssembly = dynamic_cast<ObjCompAssembly*>( elem.get() );
            if ( !objCompAssembly )
            {
                CompAssembly* compAssembly = dynamic_cast<CompAssembly*>( elem.get() );
                if ( !compAssembly || compAssembly->nelements() != 1 )
                {
                    //std::cerr << "Not a CompAssembly, out" << std::endl;
                    return false;
                }
                elem = compAssembly->getChild(0);
                objCompAssembly = dynamic_cast<ObjCompAssembly*>( elem.get() );
                if ( !objCompAssembly )
                {
                    //std::cerr << "Not a ObjCompAssembly, out" << std::endl;
                    return false;
                }
            }
            if ( i == 0 )
            {
                pos = objCompAssembly->getChild(0)->getPos();
                x = objCompAssembly->getChild(1)->getPos() - pos;
                x.normalize();
            }
            else if ( i == 1 )
            {
                y = objCompAssembly->getChild(0)->getPos() - pos;
                y.normalize();
                normal = x.cross_prod( y );
                if ( normal.nullVector() )
                {
                    y = objCompAssembly->getChild(1)->getPos() - objCompAssembly->getChild(0)->getPos();
                    y.normalize();
                    normal = x.cross_prod( y );
                }
                if ( normal.nullVector() )
                {
                    std::cerr << "Colinear ObjCompAssemblies, out" << std::endl;
                    return false;
                }
                normal.normalize();
            }
            else
            {
                Mantid::Kernel::V3D vector = objCompAssembly->getChild(0)->getPos() - objCompAssembly->getChild(1)->getPos();
                vector.normalize();
                if ( fabs(vector.scalar_prod(normal)) > 1e-3 )
                {
                    std::cerr << "Out of plane, out" << std::endl;
                    return false;
                }
            }
            ndetectors += objCompAssembly->nelements();
            objCompAssemblies << objCompAssembly->getComponentID();
        }
        std::cerr << "CompAssemblyActor " << ndetectors << std::endl;
        if ( !objCompAssemblies.isEmpty() )
        {
            m_surface.addFlatBank(assembly->getComponentID(), normal, objCompAssemblies);
        }
        return false;
    }

    bool visit(const RectangularDetectorActor* actor)
    {
        std::cerr << "RectangularDetectorActor " << actor->getNumberOfDetectors() << std::endl;
        return false;
    }
};

/**
  * Traverse the instrument tree and find the banks which detectors lie in the same plane.
  *
  */
void PanelsSurface::findFlatBanks()
{
    FlatBankFinder finder(*this);
    m_instrActor->accept( finder );
}

//-----------------------------------------------------------------------------------------------//

/**
  * Add a flat bank from an assembly of ObjCompAssemblies.
  * @param bankId :: Component ID of the bank.
  * @param normal :: Normal vector to the bank's plane.
  * @param objCompAssemblies :: List of component IDs. Each component must cast to ObjCompAssembly.
  */
void PanelsSurface::addFlatBank(ComponentID bankId, const Mantid::Kernel::V3D &normal, QList<ComponentID> objCompAssemblies)
{
    int index = m_flatBanks.size();
    FlatBankInfo info;
    info.id = bankId;
    info.normal = normal;
    m_flatBanks << info;
    Mantid::Geometry::Instrument_const_sptr instr = m_instrActor->getInstrument();
    foreach(ComponentID id, objCompAssemblies)
    {
        Mantid::Geometry::ICompAssembly_const_sptr assembly = boost::dynamic_pointer_cast<const Mantid::Geometry::ICompAssembly>(
                    instr->getComponentByID(id));
        assert(assembly);
        int nelem = assembly->nelements();
        for(int i = 0; i < nelem; ++i)
        {
            Mantid::Geometry::IDetector_const_sptr det = boost::dynamic_pointer_cast<const Mantid::Geometry::IDetector>(assembly->getChild(i));
        }
    }
}

