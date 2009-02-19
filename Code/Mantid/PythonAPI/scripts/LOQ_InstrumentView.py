# LOQ instrument view script
wsName = getSelectedWorkspaceName()

if wsName == "":
	alg=LoadRawDialog()
	wsName = alg.getPropertyValue("OutputWorkspace")
	
insView = getInstrumentView(wsName)
insView.showWindow()
insView.selectComponent("HAB-module")
