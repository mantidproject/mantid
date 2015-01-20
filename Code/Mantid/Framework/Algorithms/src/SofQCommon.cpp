#include "MantidAlgorithms/SofQCommon.h"

namespace Mantid {
namespace Algorithms {
/** The procedure analyses emode and efixed properties provided to the algorithm
 *and identify the energy analysis mode and the way the properties are defined
 *@param workspace     :: input workspace which may or may not have incident
 *energy property (Ei) attached to it as the run log
 *@param hostAlgorithm :: the pointer to SofQ algorithm hosting the base class.
 *This algorithm expects to have EMode and EFixed properties attached to it.
*/
void SofQCommon::initCachedValues(API::MatrixWorkspace_const_sptr workspace,
                                  API::Algorithm *const hostAlgorithm) {
  // Retrieve the emode & efixed properties
  const std::string emode = hostAlgorithm->getProperty("EMode");
  // Convert back to an integer representation
  m_emode = 0;
  if (emode == "Direct")
    m_emode = 1;
  else if (emode == "Indirect")
    m_emode = 2;
  m_efixed = hostAlgorithm->getProperty("EFixed");

  // Check whether they should have supplied an EFixed value
  if (m_emode == 1) // Direct
  {
    // If GetEi was run then it will have been stored in the workspace, if not
    // the user will need to enter one
    if (m_efixed == 0.0) {
      if (workspace->run().hasProperty("Ei")) {
        Kernel::Property *p = workspace->run().getProperty("Ei");
        Kernel::PropertyWithValue<double> *eiProp =
            dynamic_cast<Kernel::PropertyWithValue<double> *>(p);
        if (!eiProp)
          throw std::runtime_error("Input workspace contains Ei but its "
                                   "property type is not a double.");
        m_efixed = (*eiProp)();
      } else {
        throw std::invalid_argument("Input workspace does not contain an "
                                    "EFixed value. Please provide one or run "
                                    "GetEi.");
      }
    } else {
      m_efixedGiven = true;
    }
  } else {
    if (m_efixed != 0.0) {
      m_efixedGiven = true;
    }
  }
}

/**
 * Return the efixed for this detector. In Direct mode this has to be property
 set up earlier and in Indirect mode it may be the property of a component
                                        if not specified globally for the
 instrument.
 * @param det A pointer to a detector object
 * @return The value of efixed
 */
double SofQCommon::getEFixed(Geometry::IDetector_const_sptr det) const {
  double efixed(0.0);
  if (m_emode == 1) // Direct
  {
    efixed = m_efixed;
  } else // Indirect
  {
    if (m_efixedGiven)
      efixed = m_efixed; // user provided a value
    else {
      std::vector<double> param = det->getNumberParameter("EFixed");
      if (param.empty())
        throw std::runtime_error(
            "Cannot find EFixed parameter for component \"" + det->getName() +
            "\". This is required in indirect mode. Please check the IDF "
            "contains these values.");
      efixed = param[0];
    }
  }
  return efixed;
}
}
}