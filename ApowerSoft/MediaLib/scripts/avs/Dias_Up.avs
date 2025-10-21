 # TransAll avisynth plugin created by V.C.Mohan

    diasblack=at.levels(0,1,255,0,0,true)
    half=int(ft/2)
    TransPush(at.Trim1(0,half), diasblack.Trim1(0,half),half+1,"up")++diasblack.Trim1(0,-1)++TransPush(diasblack.Trim1(0,half), bt.Trim1(0,half),half+1,"down")
