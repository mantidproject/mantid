

def changedColorRangeHandler(obj, _min, _max, _log):
    global min, max, log
    min = _min
    max = _max
    log = _log
    print min, max, log

def main():
    global min, max, log
    changedColorRangeHandler(None, 0,0,0)
    
    startApplication("ColorBarWidgetDemo")

    installSignalHandler(":ColorBarWidget_ColorBarWidget",
            "changedColorRange(double,double,bool)", "changedColorRangeHandler")
        
    spinDown(waitForObject(":valMax_QScienceSpinBox"))
    test.verify( waitFor( "abs(max-99) < 1e-3", 250 ), "Signal was received" )
     
    spinDown(waitForObject(":valMax_QScienceSpinBox"))
    test.verify( waitFor( "abs(max-98) < 1e-3", 250 ), "Signal was received" )
    
    spinDown(waitForObject(":valMax_QScienceSpinBox"))
    test.verify( waitFor( "abs(max-97) < 1e-3", 250 ), "Signal was received" )
#    
#    spinUp(waitForObject(":valMin_QScienceSpinBox"))
#    print 'up ', min, max, log
#    print 'up ', min, max, log
#    print 'up ', min, max, log
#    print 'up ', min, max, log
#    print 'up ', min, max, log
#    
#    test.verify( waitFor( "abs(min-1) < 1e-2", 2250 ), "Signal was received %g" % min )
#    
#    spinUp(waitForObject(":valMin_QScienceSpinBox"))
#    test.verify( waitFor( "abs(min-2) < 1e-3", 1250 ), "Signal was received %g" % min )
#
#    spinUp(waitForObject(":valMin_QScienceSpinBox"))
#    test.verify( waitFor( "abs(min-3) < 1e-3", 1250 ), "Signal was received %g" % min )
#    
#    clickButton(waitForObject(":Log_QCheckBox"))
#    test.verify( waitFor( "log", 1250 ), "Signal was received %g" % log )

    print 'at the end ', min, max, log
        
    sendEvent("QCloseEvent", waitForObject(":_QMainWindow"))
