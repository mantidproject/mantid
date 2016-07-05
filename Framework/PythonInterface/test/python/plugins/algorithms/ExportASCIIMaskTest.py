if True:
    masks = [1,4,8,10,11,12,12,198,199,200]
    test_ws = CreateSampleWorkspace()
    test_ws.maskDetectors(masks)
    
    f_name = os.path.join(config.getString('defaultsave.directory'),'test_ws.msk')
    r_masks = ExportASCIIMask(test_ws,Filename=f_name)
    
    wmsk = ''
    with open(f_name,'r') as res_file:
        for line in res_file:
            wmsk = line
    
    print "Input mask: ",masks
    print "Extracted mask: ",r_masks
    print "Saved mask: ",wmsk
    
    os.remove(f_name)
