#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFFITTINGMODEL_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFFITTINGMODEL_H_

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <array>
#include <unordered_map>
#include <vector>

using namespace Mantid;

namespace MantidQT {
namespace CustomInterfaces {

class MANTIDQT_ENGGDIFFRACTION_DLL EnggDiffFittingModel {
public:

	void addWorkspace(const int runNumber, const int bank, const API::MatrixWorkspace_sptr ws);
	API::MatrixWorkspace_sptr getWorkspace(const int runNumber, const int bank);
	std::vector<int> getAllRunNumbers();

private:

	static const int MAX_BANKS = 2;
	std::array<std::unordered_map<int, Mantid::API::MatrixWorkspace_sptr>, MAX_BANKS> m_wsMap;

};

} // namespace CustomInterfaces
} // namespace MantidQT
#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_ENGGDIFFFITTINGMODEL_H_