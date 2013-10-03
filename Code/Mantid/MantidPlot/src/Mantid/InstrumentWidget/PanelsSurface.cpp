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

/**
  * Translate the bank by a vector.
  * @param shift :: Translation vector.
  */
void FlatBankInfo::translate(const QPointF &shift)
{
    double du = shift.x();
    double dv = shift.y();
    polygon.translate(shift);
    for(size_t i = startDetectorIndex; i < endDetectorIndex; ++i)
    {
        UnwrappedDetector &udet = surface->m_unwrappedDetectors[i];
        udet.u += du;
        udet.v += dv;
    }
}

PanelsSurface::PanelsSurface(const InstrumentActor* rootActor,const Mantid::Kernel::V3D& origin,const Mantid::Kernel::V3D& axis):
    UnwrappedSurface(rootActor),
    m_pos(origin),
    m_zaxis(axis)
{
    setupAxes();
    init();
}

PanelsSurface::~PanelsSurface()
{
    clearBanks();
}

/**
 * Initialize the surface.
 */
void PanelsSurface::init()
{
    m_unwrappedDetectors.clear();
    m_assemblies.clear();

    size_t ndet = m_instrActor->ndetectors();
    //m_unwrappedDetectors.resize(ndet);
    if (ndet == 0) return;

    // Pre-calculate all the detector positions (serial because
    // I suspect the IComponent->getPos() method to not be properly thread safe)
    m_instrActor->cacheDetPos();

    Instrument_const_sptr inst = m_instrActor->getInstrument();

    findFlatBanks();
    spreadBanks();

    for(int i = 0; i < m_flatBanks.size(); ++i)
    {
        RectF rect( m_flatBanks[i]->polygon.boundingRect() );
        double umin = rect.x0();
        double umax = rect.x1();
        double vmin = rect.y0();
        double vmax = rect.y1();
        if ( umin < m_u_min ) m_u_min = umin;
        if ( umax > m_u_max ) m_u_max = umax;
        if ( vmin < m_v_min ) m_v_min = vmin;
        if ( vmax > m_v_max ) m_v_max = vmax;
    }

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

/**
  * Given the z axis, define the x and y ones.
  */
void PanelsSurface::setupAxes()
{
    double R, theta, phi;
    m_zaxis.getSpherical( R, theta, phi );
    if ( theta <= 45.0 )
    {
        m_xaxis = Mantid::Kernel::V3D(1,0,0);
    }
    else if ( phi <= 45.0 )
    {
        m_xaxis = Mantid::Kernel::V3D(0,1,0);
    }
    else
    {
        m_xaxis = Mantid::Kernel::V3D(0,0,1);
    }
    m_yaxis = m_zaxis.cross_prod( m_xaxis );
    m_yaxis.normalize();
    m_xaxis = m_yaxis.cross_prod( m_zaxis );
    m_origin.rx() = m_xaxis.scalar_prod( m_pos );
    m_origin.ry() = m_yaxis.scalar_prod( m_pos );
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
            auto elem = assembly->getChild((int)i);
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
    clearBanks();
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
    //int index = m_flatBanks.size();
    // save bank info
    FlatBankInfo *info = new FlatBankInfo(this);
    info->id = bankId;
    info->startDetectorIndex = m_unwrappedDetectors.size();
    bool doneRotation = false;
    // keep reference position on the bank's plane
    Mantid::Kernel::V3D pos0;
    QPointF p0,p1;
    Mantid::Geometry::Instrument_const_sptr instr = m_instrActor->getInstrument();
    // loop over the assemblies and process the detectors
    foreach(ComponentID id, objCompAssemblies)
    {
        Mantid::Geometry::ICompAssembly_const_sptr assembly = boost::dynamic_pointer_cast<const Mantid::Geometry::ICompAssembly>(
                    instr->getComponentByID(id));
        assert(assembly);
        int nelem = assembly->nelements();
        m_unwrappedDetectors.reserve( m_unwrappedDetectors.size() + nelem );
        for(int i = 0; i < nelem; ++i)
        {
            // setup detector info
            Mantid::Geometry::IDetector_const_sptr det = boost::dynamic_pointer_cast<const Mantid::Geometry::IDetector>( assembly->getChild(i) );
            Mantid::Kernel::V3D pos = det->getPos();
            if ( !doneRotation )
            {
                pos0 = pos;
                p0.rx() = m_xaxis.scalar_prod( pos0 );
                p0.ry() = m_yaxis.scalar_prod( pos0 );
                info->rotation = calcBankRotation( pos0, normal );
                Mantid::Kernel::V3D pos1 = assembly->getChild(nelem-1)->getPos();
                pos1 -= pos0;
                info->rotation.rotate(pos1);
                pos1 += pos0;
                p1.rx() = m_xaxis.scalar_prod( pos1 );
                p1.ry() = m_yaxis.scalar_prod( pos1 );
                QVector<QPointF> vert;
                vert << p0 << p1;
                info->polygon = QPolygonF(vert);
                doneRotation = true;
            }
            UnwrappedDetector udet;
            udet.detector = det;
            m_instrActor->getColor( det->getID() ).getUB3( &udet.color[0] );
            pos -= pos0;
            info->rotation.rotate(pos);
            pos += pos0;
            udet.u = m_xaxis.scalar_prod( pos );
            udet.v = m_yaxis.scalar_prod( pos );
            udet.uscale = udet.vscale = 1.0;
            //info->rect.include( QPointF(udet.u,udet.v) );
            udet.height = 0.001;
            udet.width = 0.001;
            m_unwrappedDetectors.push_back( udet );
        }
        UnwrappedDetector &udet0 = *(m_unwrappedDetectors.end() - nelem);
        UnwrappedDetector &udet1 = m_unwrappedDetectors.back();
        QVector<QPointF> vert;
        vert << p0 << p1 << QPointF(udet0.u,udet0.v) << QPointF(udet1.u,udet1.v);
        info->polygon = info->polygon.united(QPolygonF(vert));
    }
    info->endDetectorIndex = m_unwrappedDetectors.size();
    m_flatBanks << info;
}

/**
  * Calculate the rotation needed to place a bank on the projection plane.
  *
  * @param detPos :: Position of a detector of the bank.
  * @param normal :: Normal to the bank's plane.
  */
Mantid::Kernel::Quat PanelsSurface::calcBankRotation(const Mantid::Kernel::V3D &detPos, Mantid::Kernel::V3D normal) const
{
    Mantid::Kernel::Quat R; // identity
    if ( normal.cross_prod(m_zaxis).nullVector() )
    {
        return Mantid::Kernel::Quat();
    }

    // signed shortest distance from the bank's plane to the origin (m_pos)
    double a = normal.scalar_prod( m_pos - detPos );
    // if a is negative the origin is on the "back" side of the plane
    // (the "front" side is facing in the direction of the normal)
    if ( a < 0.0 )
    {
        // we need to flip the normal to make the side looking at the origin to be the front one
        normal *= -1;
    }

    return Mantid::Kernel::Quat( normal, m_zaxis );
}

/**
  * Spread the banks over the projection plane so that they don't overlap.
  *
  */
void PanelsSurface::spreadBanks()
{
    int heavy = findLargestBank();
    for(int i = 0; i < m_flatBanks.size(); ++i)
    {
        // leave the largest bank where it is
        if ( i == heavy ) continue;
        FlatBankInfo *info = m_flatBanks[i];
        QPolygonF poly = info->polygon;
        QRectF rect = poly.boundingRect();
        // define direction of movement for the bank: radially away from origin
        QPointF centre = rect.center();
        QPointF dir = centre - m_origin;
        qreal length = sqrt(dir.x()*dir.x() + dir.y()*dir.y());
        if ( length < 1e-5 )
        {
            dir.setX(1.0);
            dir.setY(0.0);
        }
        else
        {
            dir /= length;
        }
        qreal step = ( fabs(rect.width()*dir.x()) + fabs(rect.height()*dir.y()) ) / 4;
        dir *= step;
        if ( step == 0.0 ) continue;
        // move the bank until it doesn't overlap with anything else
        while( isOverlapped(poly,i) )
        {
            poly.translate( dir );
        }
        // move all detectors of the bank
        info->translate(poly.boundingRect().center() - centre);
    }
}

/**
  * Find index of the largest bank.
  */
int PanelsSurface::findLargestBank() const
{
    double maxArea = 0.0;
    int index = 0;
    for(int i = 0; i < m_flatBanks.size(); ++i)
    {
        const FlatBankInfo *info = m_flatBanks[i];
        QRectF rect = info->polygon.boundingRect();
        double area = rect.height() * rect.width();
        if ( area > maxArea )
        {
            index = i;
            maxArea = area;
        }
    }
    return index;
}

bool PanelsSurface::isOverlapped(QPolygonF &rect, int iexclude) const
{
    for(int i = 0; i < m_flatBanks.size(); ++i)
    {
        if ( i == iexclude ) continue;
        //if ( rect.doesIntersect(m_flatBanks[i]->rect) ) return true;
        QPolygonF poly = rect.intersected(m_flatBanks[i]->polygon);
        if (poly.size() > 0 ) return true;
    }
    return false;
}

void PanelsSurface::clearBanks()
{
    for(int i = 0; i < m_flatBanks.size(); ++i)
    {
        if ( m_flatBanks[i] ) delete m_flatBanks[i];
    }
    m_flatBanks.clear();
}

