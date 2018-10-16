"""
You can run this widget independently by for example:

    from mantidqt.widgets.samplelogs.presenter import SampleLogs
    from mantid.simpleapi import Load
    from qtpy.QtWidgets import QApplication

    ws=Load('CNCS_7860')

    app = QApplication([])
    window = SampleLogs(ws)
    app.exec_()
"""
