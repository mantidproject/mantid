source(findFile("scripts", "global_checks.py"))

def check_vsi_state(expected_state, message=None):
    ws_ctx_menu = waitForObject(":MantidPlot - untitled.WorkspaceContextMenu_QMenu")
    vsi_menuitem = get_action(ws_ctx_menu, "Show Vates Simple Interface")
    test.compare(fix_bool(vsi_menuitem.enabled), expected_state, message)

def check_view_filter_button_state(bname, expected_state, message=None):
    waitForObject(":splitter_2.splitter_QSplitter")
    full_name = ":splitter_2.%s_QPushButton" % bname
    button = findObject(full_name)
    button_state = fix_bool(button.enabled)
    test.compare(button_state, expected_state, message)

def check_sv_opened(ws_name, slice_num, expected_state, message=None):
    sv_name = ":Slice Viewer (%s) Slice%d_MantidQt::SliceViewer::SliceViewerWindow" % (ws_name, slice_num)
    waitFor("object.exists('%s')" % sv_name, 20000)
    test.compare(fix_bool(findObject(sv_name).enabled), expected_state, message)

def check_mode_button_state(mode_name, expected_state, message=None):
    mode_button_name = ':Vates Simple Interface.%sButton_QPushButton' % mode_name
    waitFor("object.exists('%s')" % mode_button_name, 20000)
    test.compare(fix_bool(findObject(mode_button_name).enabled), expected_state, message)