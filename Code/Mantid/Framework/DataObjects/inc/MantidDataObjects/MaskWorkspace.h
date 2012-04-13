#ifndef MANTID_DATAOBJECTS_MASKWORKSPACE_H
#define MANTID_DATAOBJECTS_MASKWORKSPACE_H

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace DataObjects
{

    class DLLExport MaskWorkspace : public SpecialWorkspace2D
    {
    public:
        MaskWorkspace();
        MaskWorkspace(std::size_t numvectors);
        MaskWorkspace(Mantid::Geometry::Instrument_const_sptr instrument,
                      const bool includeMonitors=false);
        ~MaskWorkspace();

        bool isMasked(const detid_t detectorID) const;
        std::size_t getNumberMasked() const;
        virtual const std::string id() const;

    private:
        /// Private copy constructor. NO COPY ALLOWED
        MaskWorkspace(const MaskWorkspace&);
        /// Private copy assignment operator. NO ASSIGNMENT ALLOWED
        MaskWorkspace& operator=(const MaskWorkspace&);

    };

    ///shared pointer to the MaskWorkspace class
    typedef boost::shared_ptr<MaskWorkspace> MaskWorkspace_sptr;

    ///shared pointer to a const MaskWorkspace
    typedef boost::shared_ptr<const MaskWorkspace> MaskWorkspace_const_sptr;

} // namespace DataObjects
} // namespace Mantid


#endif // MANTID_DATAOBJECTS_MASKWORKSPACE_H
