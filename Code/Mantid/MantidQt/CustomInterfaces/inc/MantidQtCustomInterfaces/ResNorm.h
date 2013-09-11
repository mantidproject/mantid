#ifndef MANTIDQTCUSTOMINTERFACESIDA_RESNORM_H_
#define MANTIDQTCUSTOMINTERFACESIDA_RESNORM_H_

#include "MantidQtCustomInterfaces/IndirectBayesTab.h"

namespace MantidQt
{
	namespace CustomInterfaces
	{
		class ResNorm : public IndirectBayesTab
		{
			Q_OBJECT

		public:
			ResNorm(QWidget * parent = 0);

		private:
			virtual void validate();
			virtual void run();
		};
	} // namespace CustomInterfaces
} // namespace MantidQt

#endif  MANTIDQTCUSTOMINTERFACESIDA_RESNORM_H_
