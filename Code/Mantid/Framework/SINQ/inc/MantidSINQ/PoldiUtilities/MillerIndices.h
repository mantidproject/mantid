#ifndef MANTID_SINQ_MILLERINDICES_H
#define MANTID_SINQ_MILLERINDICES_H

#include "MantidSINQ/DllConfig.h"
#include <vector>

namespace Mantid {
namespace Poldi {

class MANTID_SINQ_DLL MillerIndices {
public:
    MillerIndices(int h, int k, int l);
    ~MillerIndices() {}

    int h() const;
    int k() const;
    int l() const;

    int operator[](int index);

    const std::vector<int>& asVector() const;

private:
    void populateVector();

    int m_h;
    int m_k;
    int m_l;

    std::vector<int> m_asVector;
};


}
}


#endif // MANTID_SINQ_MILLERINDICES_H
