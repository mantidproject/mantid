# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.py3compat import mock
import systemtesting
from ErrorReporter.error_report_presenter import ErrorReporterPresenter # noqa


class ErrorReportServerTests(systemtesting.MantidSystemTest):
    def setUp(self):
        mock_view = mock.MagicMock()
        self.error_report_presenter = ErrorReporterPresenter(mock_view, '1')

    def test_no_share_sends_nothing(self):
        status = self.error_report_presenter.error_handler(True, 2, '', '', '')
        self.assertEqual(status, -1)

    def test_share_non_identifiable_returns_created_status(self):
        status = self.error_report_presenter.error_handler(True, 1, '', '', '')
        self.assertEqual(status, 201)

    def test_share_identifiable_works_for_empty_values(self):
        status = self.error_report_presenter.error_handler(True, 0, '', '', '')
        self.assertEqual(status, 201)

    def test_share_identifiable_works_for_just_name(self):
        status = self.error_report_presenter.error_handler(True, 0, 'public_name', '', '')
        self.assertEqual(status, 201)

    def test_share_identifiable_works_for_just_email(self):
        status = self.error_report_presenter.error_handler(True, 0, '', 'public_email', '')
        self.assertEqual(status, 201)

    def test_share_identifiable_works_for_just_textbox(self):
        status = self.error_report_presenter.error_handler(True, 0, '', '', 'Something went wrong')
        self.assertEqual(status, 201)

    def test_share_identifiable_works_with_no_name(self):
        status = self.error_report_presenter.error_handler(True, 0, '', 'public_email', 'Something went wrong')
        self.assertEqual(status, 201)

    def test_share_identifiable_works_with_no_email(self):
        status = self.error_report_presenter.error_handler(True, 0, 'public_name', '', 'Something went wrong')
        self.assertEqual(status, 201)

    def test_share_identifiable_works_with_no_textbox(self):
        status = self.error_report_presenter.error_handler(True, 0, 'public_name', 'public_email', '')
        self.assertEqual(status, 201)

    def test_share_identifiable_works_with_all(self):
        status = self.error_report_presenter.error_handler(True, 0, 'public_name', 'public_email', 'Something went wrong')
        self.assertEqual(status, 201)

    def excludeInPullRequests(self):
        return True

    def runTest(self):
        self.setUp()
        self.test_no_share_sends_nothing()
        self.test_share_non_identifiable_returns_created_status()
        self.test_share_identifiable_works_for_empty_values()
        self.test_share_identifiable_works_for_just_name()
        self.test_share_identifiable_works_for_just_email()
        self.test_share_identifiable_works_for_just_textbox()
        self.test_share_identifiable_works_with_no_name()
        self.test_share_identifiable_works_with_no_email()
        self.test_share_identifiable_works_with_no_textbox()
        self.test_share_identifiable_works_with_all()
