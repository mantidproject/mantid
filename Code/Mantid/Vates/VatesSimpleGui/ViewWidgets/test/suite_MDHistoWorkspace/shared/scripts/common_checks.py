def check_vsi_state(expected_state, message=None):
    ws_ctx_menu = waitForObject(":MantidPlot - untitled.WorkspaceContextMenu_QMenu")
    vsi_menuitem = get_action(ws_ctx_menu, "Show Vates Simple Interface")
    test.compare(vsi_menuitem.enabled, expected_state, message)