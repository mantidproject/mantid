// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ALGORITHMPROGRESSWIDGET_H
#define ALGORITHMPROGRESSWIDGET_H
//----------------------------------
// Includes
//----------------------------------
#include "MantidAPI/AlgorithmObserver.h"

#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/Message.h"

#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressDialogWidget.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressPresenter.h"
#include <QHBoxLayout>
#include <QPointer>
#include <QPushButton>
#include <QWidget>
#include <memory>

class QProgressBar;

/** The AlgorithmProgressWidget shows the main progress bar always visible on the
 * Workbench. The general structure is:
 * - The presenter hooks onto a startingHandle that tracks all starting algorithms
 * - When an algorithm starts an observer is hooked onto it
 * - On updates, the progress bar is updated
 * - On finish the algorithm observer is removed
 *
 * This class is also the creator/owner of the Details window, which can show multiple
 * progress bars for algorithms running simultaneously (e.g. from 2 code editors).
 */

namespace MantidQt {
namespace MantidWidgets {

    /// This widget & code structure was rewritten in C++ from the original implementation in Python
    /// https://github.com/mantidproject/mantid/tree/75e6bb4b242e028e2cdae5f1a87c0bc60795047f/qt/python/mantidqt/widgets/algorithmprogress
    class EXPORT_OPT_MANTIDQT_COMMON AlgorithmProgressWidget : public QWidget {
        Q_OBJECT
    public:
        AlgorithmProgressWidget(QWidget* parent = nullptr);

        constexpr QProgressBar* progressBar() const;

        /// Setup the view for whenever an algorithm has started.
        void algorithmStarted();
        /// Setup the view for whenever an algorithm has ended.
        void algorithmEnded();

    public slots:
        void showDetailsDialog();

    private:
        // Widgets are managed by Qt. This class owns them, and Qt will delete them on close.
        /// Progress bar shown on the workbench
        QProgressBar* const m_progressBar;
        /// Layout that contains all the widget for displaying
        QHBoxLayout* const m_layout;
        /// Button to display the Details window
        QPushButton* const m_detailsButton;
        /// Pointer to the details dialog. Will be set to nullptr by QPointer when the dialog closes
        QPointer<AlgorithmProgressDialogWidget> m_details;
        /// The presenter of the ProgressWidget
        std::unique_ptr<AlgorithmProgressPresenter> m_presenter;
    };

} // namespace MantidWidgets
} // namespace MantidQt
#endif // ALGORITHMPROGRESSWIDGET_H