#include "MantidGeometry/Crystal/IsotropicAtomBraggScatterer.h"
#include "MantidKernel/Atom.h"
#include <stdexcept>

#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MandatoryValidator.h"

#include "MantidGeometry/Crystal/BraggScattererFactory.h"

namespace Mantid
{
namespace Geometry
{

using namespace Kernel;

/// Constructor which takes an element symbol, fractional coordinates, isotropic atomic displacement parameter and occupancy.
IsotropicAtomBraggScatterer::IsotropicAtomBraggScatterer() :
    BraggScattererInCrystalStructure(),
    m_atom(),
    m_label()
{
}

/// Clones the instance.
BraggScatterer_sptr IsotropicAtomBraggScatterer::clone() const
{
    IsotropicAtomBraggScatterer_sptr clone = boost::make_shared<IsotropicAtomBraggScatterer>();
    clone->initialize();
    clone->setProperties(this->asString(false, ';'));

    return clone;
}

/// Tries to obtain element specific data for the given symbol using PhysicalConstants::getAtom.
void IsotropicAtomBraggScatterer::setElement(const std::string &element)
{
    PhysicalConstants::Atom atom = PhysicalConstants::getAtom(element);

    m_atom = atom.neutron;
    m_label = atom.symbol;
}

/// Returns the string representation of the contained element.
std::string IsotropicAtomBraggScatterer::getElement() const
{
    return m_label;
}

/// Returns the internally stored NeutronAtom that holds element specific data.
PhysicalConstants::NeutronAtom IsotropicAtomBraggScatterer::getNeutronAtom() const
{
    return m_atom;
}

/// Returns the occupancy.
double IsotropicAtomBraggScatterer::getOccupancy() const
{
    return getProperty("Occupancy");
}

/// Returns the isotropic atomic displacement parameter.
double IsotropicAtomBraggScatterer::getU() const
{
    return getProperty("U");
}

/**
 * Calculates the structure factor
 *
 * This method calculates the structure factor, taking into account contributions from all
 * atoms on the stored position _and all symmetrically equivalent_.
 * For details, please refer to the class documentation in the header file.
 *
 * @param hkl :: HKL for which the structure factor should be calculated
 * @return Structure factor (complex).
 */
StructureFactor IsotropicAtomBraggScatterer::calculateStructureFactor(const V3D &hkl) const
{
    double amplitude = getOccupancy() * getDebyeWallerFactor(hkl) * getScatteringLength();

    StructureFactor sum(0.0, 0.0);

    std::vector<V3D> equivalentPositions = getEquivalentPositions();
    for(auto pos = equivalentPositions.begin(); pos != equivalentPositions.end(); ++pos) {
        double phase = 2.0 * M_PI * (*pos).scalar_prod(hkl);
        sum += amplitude * StructureFactor(cos(phase), sin(phase));
    }

    return sum;
}

/**
 * Declares properties of this scatterer model
 *
 * In addition to the properties of BraggScatterer, this class implements three more properties,
 * as described in the general class documentation, with some restrictions on allowed
 * values:
 *  - U must be 0 or greater
 *  - Occupancy must be on the interval [0,1]
 *  - Element must be present.
 */
void IsotropicAtomBraggScatterer::declareScattererProperties()
{
    // Default behavior requires this.
    setElement("H");

    boost::shared_ptr<BoundedValidator<double> > uValidator = boost::make_shared<BoundedValidator<double> >();
    uValidator->setLower(0.0);

    declareProperty(new PropertyWithValue<double>("U", 0.0, uValidator), "Isotropic atomic displacement in Angstrom^2");

    IValidator_sptr occValidator = boost::make_shared<BoundedValidator<double> >(0.0, 1.0);
    declareProperty(new PropertyWithValue<double>("Occupancy", 1.0, occValidator), "Site occupancy, values on interval [0,1].");

    declareProperty(new PropertyWithValue<std::string>("Element", "H", boost::make_shared<MandatoryValidator<std::string> >()));
}

/// After setting the element as a string, the corresponding
void IsotropicAtomBraggScatterer::afterScattererPropertySet(const std::string &propertyName)
{
    if(propertyName == "Element") {
        setElement(getPropertyValue(propertyName));
    }
}

/// Returns the Debye-Waller factor, using an isotropic atomic displacement and the stored unit cell.
double IsotropicAtomBraggScatterer::getDebyeWallerFactor(const V3D &hkl) const
{
    V3D dstar = getCell().getB() * hkl;

    return exp(-2.0 * M_PI * M_PI * getU() * dstar.norm2());
}

/// Returns the scattering length of the stored element.
double IsotropicAtomBraggScatterer::getScatteringLength() const
{
    return m_atom.coh_scatt_length_real;
}

DECLARE_BRAGGSCATTERER(IsotropicAtomBraggScatterer)

} // namespace Geometry
} // namespace Mantid
