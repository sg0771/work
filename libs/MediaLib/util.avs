Function Motion_wx(clip c, string action)
{
    endframe=Min(39*FrameRate/20,c.framecount-1)
    endframe>=1? Apply(action+"_wx",c,endframe):c
}

Function MoveLeft_wx(clip c, int endframe)
{
    c.KenBurnsEffect(startZoomFactor=120,endZoomFactor=120,StartAlign=4,EndAlign=6,startAngle=0,endAngle=0, EndFrame=endframe)
}

Function MoveRight_wx(clip c, int endframe)
{
    c.KenBurnsEffect(startZoomFactor=120,endZoomFactor=120,StartAlign=6,EndAlign=4,startAngle=0,endAngle=0,EndFrame=endframe)
}

Function MoveUp_wx(clip c, int endframe)
{
     c.KenBurnsEffect(startZoomFactor=120,endZoomFactor=120,StartAlign=2,EndAlign=8,startAngle=0,endAngle=0,EndFrame=endframe)
}
Function MoveDown_wx(clip c, int endframe)
{

	c.KenBurnsEffect(startZoomFactor=120,endZoomFactor=120,StartAlign=8,EndAlign=2,startAngle=0,endAngle=0,EndFrame=endframe)
}
Function MoveLeftUp_wx(clip c, int endframe)
{

	c.KenBurnsEffect(startZoomFactor=120,endZoomFactor=120,StartAlign=1,EndAlign=9,startAngle=0,endAngle=0,EndFrame=endframe)
}
Function MoveLeftDown_wx(clip c, int endframe)
{

	c.KenBurnsEffect(startZoomFactor=120,endZoomFactor=120,StartAlign=7,EndAlign=3,startAngle=0,endAngle=0,EndFrame=endframe)
}
Function MoveRightUp_wx(clip c, int endframe)
{

	c.KenBurnsEffect(startZoomFactor=120,endZoomFactor=120,StartAlign=3,EndAlign=7,startAngle=0,endAngle=0,EndFrame=endframe)
}

Function MoveRightDown_wx(clip c, int endframe)
{

	c.KenBurnsEffect(startZoomFactor=120,endZoomFactor=120,StartAlign=9,EndAlign=1,startAngle=0,endAngle=0,EndFrame=endframe)
}

Function ZoomOut_wx(clip c, int endframe)
{

	c.KenBurnsEffect(startZoomFactor=120,endZoomFactor=100,StartAlign=5,EndAlign=5,startAngle=0,endAngle=0,EndFrame=endframe)
}

Function ZoomIn_wx(clip c, int endframe)
{

	c.KenBurnsEffect(startZoomFactor=100,endZoomFactor=120,StartAlign=5,EndAlign=5,startAngle=0,endAngle=0,EndFrame=endframe)
}

Function None_wx(clip c, int endframe)
{

	c.loop(endframe)
}

Function RotateClock45_wx(clip c, int endframe)
{

	c.EffectRotation(sf=0,ef=endframe,id=45,fd=0,cc=0)
}
Function RotateCounterClock45_wx(clip c, int endframe)
{

	c.EffectRotation(sf=0,ef=endframe,id=-45,fd=0,cc=0)
}
#loop(352)


function KenBurnsEffect(clip c, 
\ int "startAlign", float "startX1", float "startY1", float "startX2", float "startY2", float "startZoomFactor", float "startPanX", float "startPanY", float "startAngle",
\ int   "endAlign", float   "endX1", float   "endY1", float   "endX2", float   "endY2", float   "endZoomFactor", float   "endPanX", float   "endPanY", float   "endAngle",
\ int "width", int "height", int "StartFrame", int "EndFrame", bool "KeepState", string "ResizeMethod", float "SourcePAR", float "SourceDAR", float "TargetPAR", float "TargetDAR",
\ int "useZoomBox", float "speed", int "cubic", int "cubicX", int "cubicY", int "cubicA", int "cubicZ", int "color")
{
	#Set Defaults
	startAlign = Default(startAlign, -5)
	endAlign = Default(endAlign, -5)
	startZoomFactor = Default(startZoomFactor, 100.0)
	endZoomFactor = Default(endZoomFactor, 100.0)
	startPanX = Default(startPanX, 0.0)
	startPanY = Default(startPanY, 0.0)
	endPanX = Default(endPanX, 0.0)
	endPanY = Default(endPanY, 0.0)
	startAngle = Default(startAngle, 0.0)
	endAngle = Default(endAngle, 0.0)
	width = Default(width, c.width())
	height = Default(height, c.height())
	length = c.framecount()-1
	StartFrame = Default(StartFrame, 0)
	EndFrame = Default(EndFrame, length)
	KeepState = Default(KeepState, True)
	ResizeMethod = Default(ResizeMethod, "BilinearResize")
	SourceAR = float(c.width())/float(c.height())
	TargetAR = float(width)/float(height)
	useZoomBox = Default(useZoomBox, 2)
	speed = Default(speed, 1)
	cubic = Default(cubic, 0)
	cubicX = Default(cubicX, cubic)
	cubicY = Default(cubicY, cubic)
	cubicZ = Default(cubicZ, cubic)
	cubicA = Default(cubicA, cubic)
	color = Default(color, $000000)
	
	#Calculate Defaults
	modzoom = Max(float(c.width())/float(width),float(c.height())/float(height))
	startZoomFactor = startZoomFactor<0 ? (absf(startZoomFactor)/100.0)*modzoom : (absf(startZoomFactor)/100.0)
	endZoomFactor = endZoomFactor<0 ? (absf(endZoomFactor)/100.0)*modzoom : (absf(endZoomFactor)/100.0)
	
	#Check For Any Unreasonable Inputs
	Assert1(startZoomFactor<>0, "KenBurnsEffect: startZoomFactor can not be zero.")
	Assert1(endZoomFactor<>0, "KenBurnsEffect: endZoomFactor can not be zero.")
	Assert1((Defined(startX1) && Defined(startY1) && (Defined(startX2) || Defined(startY2))) || !(Defined(startX1) && Defined(startY1) && Defined(startX2) && Defined(startY2)), "KenBurnsEffect: when using Mode 1, you must define startX1,startY1, startX2 and/or startY2")
	Assert1((Defined(endX1) && Defined(endY1) && (Defined(endX2) || Defined(endY2))) || !(Defined(endX1) && Defined(endY1) && Defined(endX2) && Defined(endY2)), "KenBurnsEffect: when using Mode 1, you must define endX1,endY1, endX2 and/or endY2")
	Assert1(startAlign<9 || startAlign>-9, "KenBurnsEffect: startAlign [" + String(startAlign) + "] should be between -9 and 9.")
	Assert1(endAlign<9 || endAlign>-9, "KenBurnsEffect: endAlign [" + String(endAlign) + "] should be between -9 and 9.")
	Assert1(IsRGB32(c) || useZoomBox==2, "KenBurnsEffect: Clip Needs To Be RGB32. Use ConvertToRGB32() or set useZoomBox=2")
	Assert1(!((cubic>0 || cubicX>0 || cubicY>0 || cubicZ>0 || cubicA>0) && useZoomBox==2), "KenBurnsEffect: Can not use the Cubic option when useZoomBox=2. Set useZoomBox to 0 or 1 or don't use cubic.")
	Assert1(endFrame-startFrame<=length, "KenBurnsEffect: Transition length[" + String(endFrame-startFrame) + "] greater then clip lenght[" + String(length) + "]")
	Assert1(startFrame<endFrame, "KenBurnsEffect: Starting Frame[" + String(startFrame) + "] greater then Ending Frame[" + String(endFrame) + "]")
	#Assert(useZoomBox == 0 && !(startAngle == 0), "KenBurnsEffect: You must set useZoomBox=0, if using startAngle [" + String(startAngle) + "]")
	#Assert(useZoomBox == 0 && !(endAngle == 0), "KenBurnsEffect: You must set useZoomBox=0, if using endAngle [" + String(endAngle) + "]")
	
	
	#Do Some Magic
	Evals (CalcBox(c, 2, width, height, TargetAR, SourceAR, startZoomFactor, startPanX, startPanY, startAlign, startX1, startY1, startX2, startY2, SourcePAR, SourceDAR, TargetPAR, TargetDAR))
	Evals (CalcBox(c, 3, width, height, TargetAR, SourceAR, endZoomFactor, endPanX, endPanY, endAlign, endX1, endY1, endX2, endY2, SourcePAR, SourceDAR, TargetPAR, TargetDAR))
	
	
	#Check For Any Unreasonable Inputs
	Assert1(startX1<startX2, "KenBurnsEffect: Start X1[" + String(startX1) + "] point larger then Start X2 Point[" + String(startX2) + "]")
	Assert1(startY1<startY2, "KenBurnsEffect: Start Y1[" + String(startY1) + "] point larger then Start Y2 Point[" + String(startY2) + "]")
	Assert1(endX1<endX2, "KenBurnsEffect: End X1[" + String(endX1) + "] point larger then End X2 Point[" + String(endX2) + "]")
	Assert1(endY1<endY2, "KenBurnsEffect: End Y1[" + String(endY1) + "] point larger then End Y2 Point[" + String(endY2) + "]")	

	
	#non pan frames 
	start = useZoomBox>0 && startFrame>0 ? KeepState ? c.Trim1(0, -startFrame).ZoomBoxIt(width, height, ResizeMethod, startX1, startY1, startX2, startY2, color) : c.Trim1(0, -startFrame) : nop()
	end = useZoomBox>0 && endFrame<length ? KeepState ? c.Trim1(endFrame+1, 0).ZoomBoxIt(width, height, ResizeMethod, endX1, endY1, endX2, endY2, color) : c.Trim1(endFrame+1, 0) : nop()
	#zoom and pan frames
	cAlt = c.Trim1(startFrame, endFrame)
	cAlt = useZoomBox==2 ? Animate2(0, cAlt.framecount()-1, "ZoomBoxIt", cAlt, width, height, ResizeMethod, float(startX1),float(startY1), float(startX2),float(startY2), color, cAlt, width, height, ResizeMethod, float(endX1),float(endY1),  float(endX2),float(endY2), color) :nop()

	
	#add in static frames
	cAlt = useZoomBox==2 && startFrame>0 ? start + cAlt : cAlt
	cAlt = useZoomBox==2 && endFrame<length ? cAlt + end : cAlt
	
	#get center of start rectangle from first box
	startCenterX = startX1+((startX2-startX1)/2.0)-0.5
	startCenterY = startY1+((startY2-startY1)/2.0)-0.5
	
	#get center of end rectangle from second box
	endCenterX = endX1+((endX2-endX1)/2.0)-0.5
	endCenterY = endY1+((endY2-endY1)/2.0)-0.5
	
	#Speed Logic pt 1 - Resize clip 
	width = float(width)/speed
	height = float(height)/speed	
	
	#get starting zoom factor from first box
	startZoomFactorX = width/float(startX2-startX1)
	startZoomFactorY = height/float(startY2-startY1)
	
	#get ending zoom factor from second box
	endZoomFactorX = width/float(endX2-endX1)
	endZoomFactorY = height/float(endY2-endY1)
	
	
	#Pan starting and ending x point
	srcxSE = SplineCalc(c, cubicX, startFrame, startCenterX, endFrame, endCenterX)
	
	#Pan starting and ending y point
	srcySE = SplineCalc(c, cubicY, startFrame, startCenterY, endFrame, endCenterY)
	
	#Zoom X
	factor_x_SE = SplineCalc(c, cubicZ, startFrame, startZoomFactorX, endFrame, endZoomFactorX)
	
	#Zoom Y
	factor_y_SE = SplineCalc(c, cubicZ, startFrame, startZoomFactorY, endFrame, endZoomFactorY)
	
	#Rotation
	angleSE = SplineCalc(c, cubicA, startFrame, startAngle, endFrame, endAngle)
	
	
	#non pan frames 
	start = useZoomBox==0 ? c.Zoom(srcx=String(startCenterX), srcy=String(startCenterY), factorx=String(startZoomFactorX), factory=String(startZoomFactorY), width=round(width), height=round(height), angle=String(startAngle)).Trim1(0, -startFrame) : start
	end = useZoomBox==0   ? c.Zoom(srcx=String(endCenterX), srcy=String(endCenterY), factorx=String(endZoomFactorX), factory=String(endZoomFactorY), width=round(width), height=round(height), angle=String(endAngle)).Trim1(endFrame+1, 0) : end
	
	#Do the zoom/pan
	c = useZoomBox<2 ? c.Zoom(srcx=srcxSE, srcy=srcySE, factorx=factor_x_SE, factory=factor_y_SE, width=round(width), height=round(height), angle=angleSE).Trim1(startFrame, endFrame) : nop()
	
	#speed Logic pt 2 - Scale back up or down depending on speed set
	c = useZoomBox==1 ? Evals(ResizeMethod + "(c, " + String(round(width*speed)) + ", " + String(round(height*speed)) + ")" ) : c
		
	#Add in static frames
	c = useZoomBox<2 && startFrame>0 ? start + c : c
	c = useZoomBox<2 && endFrame<length ? c + end : c
	
	#speed Logic pt 2 - Scale back up or down depending on speed set
	c = useZoomBox==0 ? Evals(ResizeMethod + "(c, " + String(round(width*speed)) + ", " + String(round(height*speed)) + ")" ) : c
	useZoomBox==2 ? cAlt :c
}

# ZoomBox() - June 12st, 2008
#  Put a "box" around a clip. Box can be used to zoom in or out. Accepts Any Colorspace. Can correct for DAR (Display Aspect Ratio).
# 
# 
# Inputs
#   clip c: Accepts Any Colorspace.
#  Optional Parameters:
#   int width: output width
#   int height: output height
#   string ResizeMethod: name of resize function. Default = BilinearResize
#   float ModAR: Set the Output Aspect Ratio. If 0 then it uses the source aspect ratio (needed for animate).
# 
#  Mode 2 - Align clip
#   int Align: -9 to +9 (0=Do not use Align). Positive values - fit with no black borders, may crop clip. Negative values - fit with black borders, may letterbox clip.
#                1:top left.     2:top center.     3:top right. 
#                4:middle left.  5:middle center.  6:middle bottom. 
#                7:bottom left.  8:bottom center.  9:bottom right.
# 
#  Mode 1 - Coordinates for ZoomBox:
#   float x1: upper left x cord
#   float y1: upper left y cord
#   float x2: lower right x cord
#   float y2: lower right y cord
#               3 or 4 out of 4 points must be defined when using Mode 1
#               when omitting a single point, it will calculate the missing value, with the Aspect Ratio in mind.
#  
#  float zoomFactor: 100 = 100%, ect... 
#                      Negative values is the zoom of the pixels, positive values is the zoom of the frame. 
#                      When converting AR, this isn't perfect.
#  float panX: shift the clip x pixels. Value in source pixels. - left, + right
#  float panY: shift the clip x pixels. Value in source pixels. - up, + down
#  int color: Color of letterbox border. Default: $000000 (Black).
# 
# 
# Notes
#  Clip should be deinterlaced for best visual quality.
#  Aspect ratio for output and box(x1,y1,x2,y2) should be the same, unless you want to distort the clip.
#  Default behavor is to center clip by adding borders (Align=-5).
#  Useful for resizing clip sources (like pictures) to match the dimension of another clip.
#  If you specify Align (Mode 2) and x1,y1,x2,y2 (Mode 1), Mode 1 will be used and the align will controll the position of the zoomFactor.
#  Mode 1 (x1,y1,x2,y2) can take values from a crop opperation. x2 and y2 must be negative though; so if x2 or y2 equals 0, replace 0 with -0.00001.

Function ZoomBox(clip c, int "width", int "height", string "ResizeMethod",
\ float "SourcePAR", float "SourceDAR", float "TargetPAR", float "TargetDAR",
\ int "Align", float "x1", float "y1", float "x2", float "y2", float "zoomFactor", float "panX", float "panY", 
\ int "color")
{
	#Set Defaults
	width = Default(width, c.width())
	height = Default(height, c.height())
	ResizeMethod = Default(ResizeMethod, "BilinearResize")
	SourceAR = float(c.width())/float(c.height())
	TargetAR = float(width)/float(height)
	Align = Default(Align, -5) #Align=-5 (center and add borders to fit).
	zoomFactor = Default(zoomFactor, 100.0)
	panX = Default(panX, 0)
	panY = Default(panY, 0)
	color = Default(color, $000000)

	modzoom = Max(float(c.width())/float(width),float(c.height())/float(height))
	zoomFactor = zoomFactor<0 ? (absf(zoomFactor)/100.0)*modzoom : (absf(zoomFactor)/100.0)
	
	#Check Inputs
	Assert1(zoomFactor<>0, "ZoomBox: zoomFactor can not be zero.")
	Assert1((Defined(x1) && Defined(y1) && (Defined(x2) || Defined(y2))) || !(Defined(x1) && Defined(y1) && Defined(x2) && Defined(y2)), "ZoomBox: when using Mode 1, you must define x1,y1, x2 and/or y2")
	Assert1(Align<9 || Align>-9, "ZoomBox: Align [" + String(Align) + "] should be between -9 and 9.")
	
	#Do Some Magic
	Evals (CalcBox(c, 1, width, height, TargetAR, SourceAR, zoomFactor, panX, panY, Align, x1, y1, x2, y2, SourcePAR, SourceDAR, TargetPAR, TargetDAR))

	BoxAR = float(x2-x1)/float(y2-y1)
	#Check for any unreasonable inputs before resize opperation
	Assert1(x1<x2, "ZoomBox: x1[" + String(x1) + "] point larger then x2 Point[" + String(x2) + "]")
	Assert1(y1<y2, "ZoomBox: y1[" + String(y1) + "] point larger then y2 Point[" + String(y2) + "]")
	

	ZoomBoxIt(c, width, height, ResizeMethod, x1, y1, x2, y2, color)
}

Function ZoomBoxIt(clip c, int width, int height, string ResizeMethod, float x1, float y1, float x2, float y2, int color)
{
	#Pad clip so resizer interpolates from border when zooming out.
	#Conditional Borders... only add if it's going to be used. Border size is size of input frame. Prevents "streaking" when zooming out alot.
	borderLeft   = x1 < 0 ? c.width()/2*2:0
	borderTop    = y1 < 0 ? c.height()/2*2:0
	borderRight  = x2 > c.width()/2*2 ? c.width()/2*2:0
	borderBottom = y2 > c.height()/2*2 ? c.height()/2*2:0
	c = c.AddBorders(borderLeft,borderTop,borderRight,borderBottom,color)
	
	#Do it! Yes there are only 2 lines that directly act upon the clip.
	Evals(ResizeMethod + "(c, " + String(round(width)) + ", " + String(round(height)) + ", src_left=" + String(x1+borderLeft) + ", src_top=" + String(y1+borderTop) + ", src_width=" + String(x2-x1) + ", src_height=" + String(y2-y1) + ")")	
}

Function CalcBox(clip c, int mode, int width, int height, float TargetAR, float SourceAR, float zoomFactor, float panX, float panY, float Align, float "x1", float "y1", float "x2", float "y2", float "SourcePAR", float "SourceDAR", float "TargetPAR", float "TargetDAR")
{
	#PAR/DAR Calculations
	SourceDAR = Defined(SourcePAR) ?
		\ SourcePAR==0 ? SourceAR : SourcePAR*SourceAR
	\ : Defined(SourceDAR) ? 
		\ SourceDAR==0 ? SourceAR : float(SourceDAR)
	\ : SourceAR
	TargetDAR =  Defined(TargetPAR) ?
		\ TargetPAR==0 ? TargetAR : TargetPAR*TargetAR
	\ : Defined(TargetDAR) ? 
		\ TargetDAR==0 ? TargetAR : float(TargetDAR)
	\ : TargetAR
	
	#Calc ModAR
	ModAR = SourceDAR*TargetAR/TargetDAR
	
	
	#If Align=5 or -5 then center clip. -5: Add borders. 5: Crop. "Mode 2"
	#Display Aspect Ratio = Final Output Ratio. No Change, Show All Pixels
	#Display Aspect Ratio > Final Output Ratio. Add Height or Crop Width 
	#Display Aspect Ratio < Final Output Ratio. Add Width or Crop Height 
	EvalString = 
	\   Align<>0 && ModAR==TargetAR ?
	\ "x1=0
	   x2=c.width()
	   y1=0
	   y2=c.height()"
	
	
	\ : (Align==-5 || Align==-4 || Align==-6) && ModAR>TargetAR ?
	\ "y1=0 - ((height*ModAR-width)/2.0)*(c.height()/float(width))
	   y2=c.height() + ((height*ModAR-width)/2.0)*(c.height()/float(width))
	   x1=0" + "
	   x2=c.width()"
	\ : (Align==5 || Align==2 || Align==8) && ModAR>TargetAR ?
	\ "x1=0 + ((height-width/ModAR)/2.0)*(c.width()/float(height))
	   x2=c.width() - ((height-width/ModAR)/2.0)*(c.width()/float(height))
	   y1=0
	   y2=c.height()" 
	\ : (Align==-5 || Align==-2 || Align==-8)  && ModAR<TargetAR ?
	\ "x1=0 - ((width/ModAR-height)/2.0)*(c.width()/float(height))
	   x2=c.width() + ((width/ModAR-height)/2.0)*(c.width()/float(height))
	   y1=0
	   y2=c.height()"
	\ : (Align==5 || Align==4 || Align==6)  && ModAR<TargetAR  ?
	\ "y1=0 + ((width-height*ModAR)/2.0)*(c.height()/float(width))
	   y2=c.height() - ((width-height*ModAR)/2.0)*(c.height()/float(width))
	   x1=0
	   x2=c.width()"
	
	
	\ : (Align==-1 || Align==-2 || Align==-3) && ModAR>TargetAR ?
	\ "y1=0
	   y2=c.height() + ((height*ModAR-width)/1.0)*(c.height()/float(width))
	   x1=0
	   x2=c.width()"
	\ : (Align==-1 || Align==-4 || Align==-7)  && ModAR<TargetAR ?
	\ "x1=0
	   x2=c.width() + ((width/ModAR-height)/1.0)*(c.width()/float(height))
	   y1=0
	   y2=c.height()"
	\ : (Align==-9 || Align==-8 || Align==-7) && ModAR>TargetAR ?
	\ "y1=0 - ((height*ModAR-width)/1.0)*(c.height()/float(width))
	   y2=c.height()
	   x1=0
	   x2=c.width()"
	\ : (Align==-9 || Align==-6 || Align==-3)  && ModAR<TargetAR ?
	\ "x1=0 - ((width/ModAR-height)/1.0)*(c.width()/float(height))
	   x2=c.width()
	   y1=0
	   y2=c.height()"
	
	
	\ : (Align==2 || Align==1 || Align==3) && ModAR<TargetAR  ?
	\ "y1=0
	   y2=c.height() - ((width-height*ModAR)/1.0)*(c.height()/float(width))
	   x1=0
	   x2=c.width()" 
	\ : (Align==8 || Align==7 || Align==9) && ModAR<TargetAR  ?
	\ "y1=0 + ((width-height*ModAR)/1.0)*(c.height()/float(width))
	   y2=c.height()
	   x1=0
	   x2=c.width()" 
	\ : (Align==4 || Align==1 || Align==7) && ModAR>TargetAR ?
	\ "x1=0
	   x2=c.width() - ((height-width/ModAR)/1.0)*(c.width()/float(height))
	   y1=0
	   y2=c.height()" 
	\ : (Align==6 || Align==3 || Align==9) && ModAR>TargetAR ?
	\ "x1=0 + ((height-width/ModAR)/1.0)*(c.width()/float(height))
	   x2=c.width()
	   y1=0
	   y2=c.height()"
	
	
	\ : ""
	Defined(x1) || Defined(y1) || Defined(x2) || Defined(y2) ? nop() : Evals(EvalString)

	#For non Align Input "Mode 1"
	x1tmp = Default(x1, 0)
	y1tmp = Default(y1, 0)
	x2tmp = Default(x2, c.width())
	y2tmp = Default(y2, c.height())
	
	#Take Crop Like Input
	x2tmp = x2tmp<=x1tmp && x2tmp<0 ? c.width() + x2tmp : x2tmp
	y2tmp = y2tmp<=y1tmp && y2tmp<0 ? c.height() + y2tmp : y2tmp
	
	#Set Optional parameters "Mode 1"
	test = 0
	test = Defined(x1) ? test+1 : test+0
	test = Defined(x2) ? test+1 : test+0
	test = Defined(y1) ? test+1 : test+0
	test = Defined(y2) ? test+1 : test+0
	Assert1(test>=3, "ZoomBox/KenBurnsEffect: At least 3 out of 4 x1, y1, x2, y2 points must be defined; or do not set any")
	
	y2t = !Defined(y2) && Defined(y1) && Defined(x1) && Defined(x2) ? y1tmp + ((x2tmp-x1tmp)/TargetAR)/(SourceAR/ModAR) : y2tmp
	x2t = !Defined(x2) && Defined(x1) && Defined(y1) && Defined(y2) ? x1tmp + ((y2tmp-y1tmp)*TargetAR)*(SourceAR/ModAR) : x2tmp
	y1t = !Defined(y1) && Defined(y2) && Defined(x1) && Defined(x2) ? y2tmp - ((x2tmp-x1tmp)/TargetAR)/(SourceAR/ModAR) : y1tmp
	x1t = !Defined(x1) && Defined(x2) && Defined(y1) && Defined(y2) ? x2tmp - ((y2tmp-y1tmp)*TargetAR)*(SourceAR/ModAR) : x1tmp
	
	#Calc Pan Factor
	x1 = x1t-panX
	y1 = y1t-panY
	x2 = x2t-panX
	y2 = y2t-panY
	
	#Calc Zoom Factor
	CenterX = (x2-x1)/2.0
	CenterY = (y2-y1)/2.0
	CenterY = (Align<0 && ModAR>TargetAR)? CenterY*(TargetAR/ModAR): (Align>0 && ModAR<TargetAR)? CenterY*(TargetAR/ModAR): CenterY 
	CenterX = (Align>0 && ModAR>TargetAR)? CenterX/(TargetAR/ModAR): (Align<0 && ModAR<TargetAR)? CenterX/(TargetAR/ModAR): CenterX
	
	Align = absf(Align)
	x1 = (Align==0 || Align==5 || Align==2 || Align==8) && zoomFactor<>1 ? CenterX-((CenterX-x1)/(zoomFactor)) : (Align==3 || Align==6 || Align==9) && zoomFactor<>1 ? CenterX*2.0-(CenterX*2.0-x1)/zoomFactor : x1
	y1 = (Align==0 || Align==5 || Align==4 || Align==6) && zoomFactor<>1 ? CenterY-((CenterY-y1)/(zoomFactor)) : (Align==7 || Align==8 || Align==9) && zoomFactor<>1 ? CenterY*2.0-(CenterY*2.0-y1)/zoomFactor : y1
	x2 = (Align==0 || Align==5 || Align==2 || Align==8) && zoomFactor<>1 ? CenterX+((x2-CenterX)/(zoomFactor)) : (Align==7 || Align==4 || Align==1) && zoomFactor<>1 ? 1.0/zoomFactor*CenterX+(x2-CenterX)/zoomFactor : x2
	y2 = (Align==0 || Align==5 || Align==4 || Align==6) && zoomFactor<>1 ? CenterY+((y2-CenterY)/(zoomFactor)) : (Align==3 || Align==2 || Align==1) && zoomFactor<>1 ? 1.0/zoomFactor*CenterY+(y2-CenterY)/zoomFactor : y2
	
	
	startString = "startX1=" + String(x1) + "
	startY1=" + String(y1) + "
	startX2=" + String(x2) + "
	startY2=" + String(y2) + ""
	
	endString = "endX1=" + String(x1) + "
	endY1=" + String(y1) + "
	endX2=" + String(x2) + "
	endY2=" + String(y2) + ""
	
	boxString = "x1=" + String(x1) + "
	y1=" + String(y1) + "
	x2=" + String(x2) + "
	y2=" + String(y2) + ""
	
	Return mode==1 ? boxString : mode==2 ? startString : mode==3 ? endString : ""
}


Function SplineCalc(clip c, int mode, int startFrame, float startF, int endFrame, float endF)
{
	splineString = mode==1 ?
	\ "spline(n, "
	\ + String(startFrame-1) + "," + String(startF) + ", "
	\ + String(startFrame) + "," + String(startF) + ", "
	\ + String(endFrame) + "," + String(endF) + ", "
	\ + String(endFrame+1) + "," + String(endF) + ", true)"
	
	\ : mode==2 ? "spline(n, "
	\ + String(startFrame) + "," + String(startF) + ", "
	\ + String(startFrame+(endFrame-startFrame)/3.0) + "," + String(startF) + ", "
	\ + String(endFrame-(endFrame-startFrame)/3.0) + "," + String(endF) + ", "
	\ + String(endFrame) + "," + String(endF) + ", true)"
	
	\ : mode==3 ? "spline(n, "
	\ + String(startFrame) + "," + String(startF) + ", "
	\ + String(startFrame+(endFrame-startFrame)/3.0) + "," + String(endF) + ", "
	\ + String(endFrame-(endFrame-startFrame)/3.0) + "," + String(startF) + ", "
	\ + String(endFrame) + "," + String(endF) + ", true)"
	
	\ : mode==4 ? "spline(n, "
	\ + String(startFrame) + "," + String(startF) + ", "
	\ + String((endFrame-startFrame)/2.0) + "," + String(startF) + ", "
	\ + String(endFrame) + "," + String(endF) + ", true)"
	
	\ : mode==5 ? "spline(n, "
	\ + String(startFrame) + "," + String(startF) + ", "
	\ + String((endFrame-startFrame)/2.0) + "," + String(endF) + ", "
	\ + String(endFrame) + "," + String(endF) + ", true)"
	
	\ : "spline(n, "
	\ + String(startFrame) + "," + String(startF) + ", "
	\ + String(endFrame) + "," + String(endF) + ", true)"
	Return splineString
}
Function Trans_wx(clip c1,clip c2, string "action", int "duration") {
    action = Default(action, "TransAccordSingleCloseHor")
    duration = Default(duration, 2)
    
    duration = duration*FrameRate/20
    Apply(action, c1,c2, duration)
}



function TransAccordSingleCloseHor(clip c1, clip c2, int duration)
{
    c1.TransAccord(c2,duration,"vert",False,False)

}
function TransAccordSingleCloseVert(clip c1, clip c2, int duration)
{
    c1.TransAccord(c2,duration,"hor",False,False)

}


function TransAccordTwinsCloseHor(clip c1, clip c2, int duration)
{
	c1.TransAccord(c2,duration,"vert",True,False)
}

function TransAccordTwinsCloseVert(clip c1, clip c2, int duration)
{

	c1.TransAccord(c2,duration,"hor",True,False)
}
#枚举与文件名不一致
function TransBubblesDynamic(clip c1, clip c2, int duration)
{
 
	c1.TransBubbles(c2,duration,False)
}

function TransCentralDisappear(clip c1, clip c2, int duration)
{
 
	c1.TransCentral(c2,duration,False,true,0)
}

function TransCentralEmerge(clip c1, clip c2, int duration)
{
 
	c1.TransCentral(c2,duration,True,true,0)
}
#TransCentralTurn 
function TransCentralTurnClockwise(clip c1, clip c2, int duration)
{
 
	c1.TransCentral(c2,duration,True,true,-1)
}

function TransCentralTurnClockwiseDisappear(clip c1, clip c2, int duration)
{
  
	c1.TransCentral(c2,duration,False,true,-1)
}


function TransCrumpleClose(clip c1, clip c2, int duration)
{
 
	c1.TransCrumple(c2,duration,"fan",False)
}
function TransCrumpleOpen(clip c1, clip c2, int duration)
{
 
	c1.TransCrumple(c2,duration,"fan",True)
}
function TransDiscoClockwiseEmerge(clip c1, clip c2, int duration)
{
 
	c1.TransDisco(c2,duration,48,-1,True)
}
function TransDiscoEmerge(clip c1, clip c2, int duration)
{
 
	c1.TransDisco(c2,duration,48,0,True)
}
function TransDoorClose(clip c1, clip c2, int duration)
{
 
	c1.TransDoor(c2,duration,True,False)
}

        

function TransDoorOpen(clip c1, clip c2, int duration)
{
 
	c1.TransDoor(c2,duration,True,True)
}
function TransFlipPageDown(clip c1, clip c2, int duration)
{
 
	c1.TransFlipPage(c2,duration,"down")
}
function TransFlipPageRight(clip c1, clip c2, int duration)
{
  
	c1.TransFlipPage(c2,duration,"right")
}
function TransFunnelDown(clip c1, clip c2, int duration)
{
 
	c1.TransFunnel(c2,duration,"down")
}
function TransFunnelLeft(clip c1, clip c2, int duration)
{
 
	c1.TransFunnel(c2,duration,"left")
}
function TransFunnelRight(clip c1, clip c2, int duration)
{
 
	c1.TransFunnel(c2,duration,"right")
}
function TransFunnelUp(clip c1, clip c2, int duration)
{
 
	c1.TransFunnel(c2,duration,"up")
}
function TransHFlipNoTurn(clip c1, clip c2, int duration)
{
    c1.TransAccord(c2,duration,"hor",False,False)
}
function TransMarbles(clip c1, clip c2, int duration)
{
 
	c1.TransMarbles(c2,duration,40, 16,true)
}


#maximum radius of discs

function TransPaintBricks(clip c1, clip c2, int duration)
{
    c1.TransAccord(c2,duration,"hor",False,False)
}
function TransPaintFence(clip c1, clip c2, int duration)
{
 
	c1.TransPaint(c2,duration,"fence")
}
function TransPaintStrokes(clip c1, clip c2, int duration)
{
    c1.TransAccord(c2,duration,"hor",False,False)
}
function TransPushDown(clip c1, clip c2, int duration)
{
 
	c1.TransPush(c2,duration,"down")
}
function TransPushLeft(clip c1, clip c2, int duration)
{
 
	c1.TransPush(c2,duration,"left")
}

function TransMarbles(clip c1, clip c2, int duration)
{
    c1.TransAccord(c2,duration,"hor",False,False)
}
function TransPushUp(clip c1, clip c2, int duration)
{
 
	c1.TransPush(c2,duration,"up")
}
#Wave length Wave amplitude
function TransRippleCenter(clip c1, clip c2, int duration)
{
 
	c1.TransRipple(c2,duration,28,2,"center")
}
function TransRippleNW(clip c1, clip c2, int duration)
{
 
	c1.TransRipple(c2,duration,28,2,"nw")
}
function TransRollRight(clip c1, clip c2, int duration)
{
    c1.TransAccord(c2,duration,"hor",False,False)
}
function TransRollDown(clip c1, clip c2, int duration)
{
    c1.TransAccord(c2,duration,"hor",False,False)
}
function TransRollOutSE(clip c1, clip c2, int duration)
{
 
	c1.TransRoll(c2,duration,4,True)
}
function TransRollOutNE(clip c1, clip c2, int duration)
{
 
	c1.TransRoll(c2,duration,2,True)
}

function TransScratchHline(clip c1, clip c2, int duration)
{
 
	c1.TransScratch(c2,duration,"hline")
}
function TransScratchRain(clip c1, clip c2, int duration)
{
 
	c1.TransScratch(c2,duration,"rain")
}
function TransScratchVline(clip c1, clip c2, int duration)
{
 
	c1.TransScratch(c2,duration,"vline")
}
function TransSlideInCenter(clip c1, clip c2, int duration)
{
 
	c1.TransSlide(c2,duration,0,False)
}
function TransSlideInEast(clip c1, clip c2, int duration)
{
 
	c1.TransSlide(c2,duration,3,False)
}
function TransSlideInNE(clip c1, clip c2, int duration)
{
 
	c1.TransSlide(c2,duration,2,False)
}
function TransSlideInNW(clip c1, clip c2, int duration)
{
 
	c1.TransSlide(c2,duration,8,False)
}


          
function TransSlideInSE(clip c1, clip c2, int duration)
{
 
	c1.TransSlide(c2,duration,4,False)
}
function TransSlideInSW(clip c1, clip c2, int duration)
{
 
	c1.TransSlide(c2,duration,6,False)
}
function TransSlideInWest(clip c1, clip c2, int duration)
{
 
	c1.TransSlide(c2,duration,7,False)
}
function TransSpriteDown(clip c1, clip c2, int duration)
{
 
	c1.TransSprite(c2,duration,"down")
}
function TransSpriteLeft(clip c1, clip c2, int duration)
{
 
	c1.TransSprite(c2,duration,"left")
}
function TransSpriteRight(clip c1, clip c2, int duration)
{
 
	c1.TransSprite(c2,duration,"right")
}

function TransSpriteUp(clip c1, clip c2, int duration)
{
 
	c1.TransSprite(c2,duration,"up")
}

 
 
function TransSwingInOneDoorFirstCorner(clip c1, clip c2, int duration)
{
 
	c1.TransSwing(c2,duration,False,1,1,True)
}

function TransSwingInTwoDoorsFirstCorner(clip c1, clip c2, int duration)
{
 
	c1.TransSwing(c2,duration,False,2,1,True)
}

     

function TransSwingInTwoDoorsSecondCorner(clip c1, clip c2, int duration)
{
 
	c1.TransSwing(c2,duration,False,2,2,True)
}

function TransSwingInTwoDoorsThirdCorner(clip c1, clip c2, int duration)
{
 
	c1.TransSwing(c2,duration,False,2,3,True)
}

function TransSwingInTwoDoorsFourthCorner(clip c1, clip c2, int duration)
{
 
	c1.TransSwing(c2,duration,False,2,4,True)
}

function TransSwingInFourDoorsFirstCorner(clip c1, clip c2, int duration)
{
 
	c1.TransSwing(c2,duration,False,4,1,True)
}
function TransSwingInFourDoorsSecondCorner(clip c1, clip c2, int duration)
{
 
	c1.TransSwing(c2,duration,False,4,2,True)
}

function TransSwingInfourDoorsThirdCorner(clip c1, clip c2, int duration)
{
 
	c1.TransSwing(c2,duration,False,4,3,True)
}
function TransSwingInFourDoorsFourthCorner(clip c1, clip c2, int duration)
{
 
	c1.TransSwing(c2,duration,False,4,4,True)
}

           



function TransWeaveVert(clip c1, clip c2, int duration)
{
 
	c1.TransWeave(c2,duration,"vert")
}
#width of slats or of pattern
function TransVenetianBlindsDiam(clip c1, clip c2, int duration)
{
 
	c1.TransVenetianBlinds(c2,duration,40,"diam")
}
function TransVenetianBlindsHOR(clip c1, clip c2, int duration)
{
 
	c1.TransVenetianBlinds(c2,duration,40,"hor")
}
function TransVenetianBlindsVert(clip c1, clip c2, int duration)
{
  
	c1.TransVenetianBlinds(c2,duration,40,"vert")
}
function TransVFlipNoTurn(clip c1, clip c2, int duration)
{
    c1.TransAccord(c2,duration,"hor",False,False)
}
 

function TransWeaveHOR(clip c1, clip c2, int duration)
{
   
	c1.TransWeave(c2,duration,"hor")
}
function TransWeaveJigsawPuzzle(clip c1, clip c2, int duration)
{
 
	c1.TransWeave(c2,duration,"jigsaw")
}
function TransVenetianBlindsCheq(clip c1, clip c2, int duration)
{
 
	c1.TransVenetianBlinds(c2,duration,40,"cheq")
}     
     
Function Filter_Shader_wx(clip c, string shader, int "start", int "end") {
    start = Default(start, 0)
    end = Default(end, 200000000)

    start = start*FrameRate/20
    end = end*FrameRate/20
    end = Min(end, c.framecount - 1)
    end >= start ? c.GlFilter(start, end, 0, 0, shader) : c
}

Function Filter_wx(clip c, string "action", int "start", int "end") {
    action = Default(action, "None")
    start = Default(start, 0)
    end = Default(end, 200000000)

    start = start
    end = end
    end = Min(end, c.framecount - 1)
    end >= start ? Apply("Filter_" + action + "_wx", c, start, end) : c
}



Function Filter_G_wx(clip c, int "start", int "end") {
    C.frei0r("G","",start,end)
}
Function Filter_Vertigo_wx(clip c, int "start", int "end") {
    C.frei0r("Vertigo","0.2",start,end)
}
Function Filter_Vignette_wx(clip c, int "start", int "end") {
    C.frei0r("vignette","",start,end)
}
Function Filter_Rgbnoise_wx(clip c, int "start", int "end") {
    C.frei0r("rgbnoise","0.2",start,end)
}
Function Filter_Distort0r_wx(clip c, int "start", int "end") {
    C.frei0r("distort0r","0.01",start,end)
}
Function Filter_Nervous_wx(clip c, int "start", int "end") {
    C.frei0r("nervous","",start,end)
}
Function Filter_Glow_wx(clip c, int "start", int "end") {
    C.frei0r("glow","1",start,end)
}
Function Filter_IIRblur_wx(clip c, int "start", int "end") {
    C.frei0r("IIRblur","",start,end)
}


# VideoFilter.AddFilterEffect

Function Filter_None_wx(clip c, int "start", int "end") {
    c
}

Function Filter_DiscoLightsAll_wx(clip c, int "start", int "end") {
    c.EffectDiscoLights(sf=start, ef=end, spot="all", dim=70)
}

Function Filter_DiscoLightsLarge_wx(clip c, int "start", int "end") {
    c.EffectDiscoLights(sf=start, ef=end, spot="large", dim=70)
}

Function Filter_DiscoLightsMedium_wx(clip c, int "start", int "end") {
    c.EffectDiscoLights(sf=start, ef=end, spot="medium", dim=70)
}

Function Filter_DiscoLightsSmall_wx(clip c, int "start", int "end") {
    c.EffectDiscoLights(sf=start, ef=end, spot="small", dim=70)
}

Function Filter_FiguredGlass_wx(clip c, int "start", int "end") {
    c.EffectFiguredGlass(sf=start, ef=end, mag=2)
}

Function Filter_RainHeavy_wx(clip c, int "start", int "end") {
    c.EffectRain(sf=start,ef=end,type="heavy",red=255,green=255,blue=255)
}

Function Filter_RainLight_wx(clip c, int "start", int "end") {
    c.EffectRain(sf=start,ef=end,type="light",red=255,green=255,blue=255)
}

Function Filter_RainMedium_wx(clip c, int "start", int "end") {
    c.EffectRain(sf=start,ef=end,type="med",red=255,green=255,blue=255)
}

Function Filter_Water_wx(clip c, int "start", int "end") {
    c.EffectWater(sf=start,ef=end)
}

Function Filter_WavesLeft_wx(clip c, int "start", int "end") {
    c.EffectWaves(sf=start,ef=end,dir="left")
}

Function Filter_WavesNE_wx(clip c, int "start", int "end") {
    c.EffectWaves(sf=start,ef=end,dir="ne")
}

Function Filter_WavesNW_wx(clip c, int "start", int "end") {
    c.EffectWaves(sf=start,ef=end,dir="nw")
}

Function Filter_WavesRight_wx(clip c, int "start", int "end") {
    c.EffectWaves(sf=start,ef=end,dir="right")
}

Function Filter_WavesSE_wx(clip c, int "start", int "end") {
    c.EffectWaves(sf=start,ef=end,dir="se")
}

Function Filter_WavesSW_wx(clip c, int "start", int "end") {
    c.EffectWaves(sf=start,ef=end,dir="sw")
}

Function Filter_ConezIncerted_wx(clip c, int "start", int "end") {
    bottom = int(FrameWidth / 2)
    base = bottom % 4 != 0 ? bottom + 4 - bottom % 4 : bottom
    top = int(FrameWidth)
    c.EffectConez(sf=start,ef=end,prog=false,base=base,top=top,shade=-50,bkgrnd=true)
}

Function Filter_Conez_wx(clip c, int "start", int "end") {
    bottom = int(FrameWidth / 2)
    base = bottom % 4 != 0 ? bottom - bottom % 4 : bottom
    top = int(FrameWidth)
    c.EffectConez(sf=start,ef=end,prog=false,base=top,top=base,shade=-50,bkgrnd=true)
}

Function Filter_Conez2To3_wx(clip c, int "start", int "end") {
    bottom = int(FrameWidth * 2 / 3)
    base = bottom % 4 != 0 ? bottom - bottom % 4 : bottom
    c.EffectConez(sf=start,ef=end,prog=false,base=base,top=base,shade=-50,bkgrnd=true)
}

Function Filter_Distorter_wx(clip c, int "start", int "end") {
    c.EffectDistorter(sf=start,ef=end)
}

Function Filter_FogConstant_wx(clip c, int "start", int "end") {
    c.EffectFog(sf=start,ef=end,startfog=30,endfog=30,density=50)
}

Function Filter_FogDecreasing_wx(clip c, int "start", int "end") {
    c.EffectFog(sf=start,ef=end,startfog=70,endfog=0,density=50)
}

Function Filter_FogIncreasing_wx(clip c, int "start", int "end") {
    c.EffectFog(sf=start,ef=end,startfog=0,endfog=70,density=50)
}

Function Filter_Ripples_wx(clip c, int "start", int "end") {
    c.EffectRipples2(sf=start,ef=end,amp=1,eamp=4)
}

Function Filter_RotationLeft10d_wx(clip c, int "start", int "end") {
    c.EffectRotation(sf=start,ef=end,id=10,fd=10,cc=0)
}

Function Filter_RotationRight10d_wx(clip c, int "start", int "end") {
    c.EffectRotation(sf=start,ef=end,id=-10,fd=-10,cc=0)
}

Function Filter_RotationLeftToRight10d_wx(clip c, int "start", int "end") {
    c.EffectRotation(sf=start,ef=end,id=10,fd=-10,cc=0)
}

Function Filter_RotationRightToLeft10d_wx(clip c, int "start", int "end") {
    c.EffectRotation(sf=start,ef=end,id=-10,fd=10,cc=0)
}

Function Filter_RotationRoundToRectangle_wx(clip c, int "start", int "end") {
    ir = int(FrameWidth > FrameHeight ? FrameHeight / 2 : FrameWidth / 2)
    fr = int(Sqrt(FrameWidth * FrameWidth + FrameHeight * FrameHeight) / 2)
    c.EffectRotation(sf=start,ef=end,id=-1,fd=0,ir=ir,fr=fr,cc=0)
}

Function Filter_RotationRectangleToRound_wx(clip c, int "start", int "end") {
    fr = int(FrameWidth > FrameHeight ? FrameHeight / 2 : FrameWidth / 2)
    ir = int(Sqrt(FrameWidth * FrameWidth + FrameHeight * FrameHeight) / 2)
    c.EffectRotation(sf=start,ef=end,id=0,fd=-1,ir=ir,fr=fr,cc=0)
}

Function Filter_SnowLight_wx(clip c, int "start", int "end") {
    c.EffectSnowFlakes(sf=start,ef=end,dense=30,drift=40,fall=20)
}

Function Filter_SnowMedium_wx(clip c, int "start", int "end") {
    c.EffectSnowFlakes(sf=start,ef=end,dense=50,drift=60,fall=40)
}

Function Filter_SnowHeavy_wx(clip c, int "start", int "end") {
    c.EffectSnowFlakes(sf=start,ef=end,dense=80,drift=80,fall=80)
}

Function Filter_Emboss_wx(clip c, int "start", int "end") {
    c.ApplyRange(start,end,"ChangeHUE",11)
}

Function Filter_OldPicture_wx(clip c, int "start", int "end") {
    c.ApplyRange(start,end,"ChangeHUE",10)
}

Function Filter_EdgeFeather_wx(clip c, int "start", int "end") {
    c.ApplyRange(start,end,"ChangeHUE",0)
}

Function Filter_Casting_wx(clip c, int "start", int "end") {
    c.ApplyRange(start,end,"ChangeHUE",1)
}

Function Filter_Freezing_wx(clip c, int "start", int "end") {
    c.ApplyRange(start,end,"ChangeHUE",2)
}

Function Filter_FrostedGlass_wx(clip c, int "start", int "end") {
    c.ApplyRange(start,end,"ChangeHUE",3)
}

Function Filter_EdgeLight_wx(clip c, int "start", int "end") {
    c.ApplyRange(start,end,"ChangeHUE",4)
}

Function Filter_EdgeLightGray_wx(clip c, int "start", int "end") {
    c.ApplyRange(start,end,"ChangeHUE",8).ApplyRange(start,end,"ShowBlue")
}

Function Filter_EdgeLightRed_wx(clip c, int "start", int "end") {
    c.ApplyRange(start,end,"ChangeHUE",6)
}

Function Filter_EdgeLightGreen_wx(clip c, int "start", int "end") {
    c.ApplyRange(start,end,"ChangeHUE",7)
}

Function Filter_EdgeLightBlue_wx(clip c, int "start", int "end") {
    c.ApplyRange(start,end,"ChangeHUE",8)
}

Function Filter_OverExposure_wx(clip c, int "start", int "end") {
    c.ApplyRange(start,end,"ChangeHUE",9)
}

Function Filter_Sketch_wx(clip c, int "start", int "end") {
    c.ApplyRange(start,end,"ChangeHUE",8).ApplyRange(start,end,"ShowBlue").ApplyRange(start,end,"Invert")
}

Function Filter_Negative_wx(clip c, int "start", int "end") {
    c.ApplyRange(start,end,"Invert")
}

Function Filter_VerticalStripes_wx(clip c, int "start", int "end") {
    c.ApplyRange(start,end,"AddGrainC",50,0,1.0,1.0,0,False,true)
}

Function Filter_VerticalStripesStatic_wx(clip c, int "start", int "end") {
    c.ApplyRange(start,end,"AddGrainC",50,0,1.0,1.0,0,True,true)
}

Function Filter_StaticNoiseGray_wx(clip c, int "start", int "end") {
    c.ConvertToYV12().ApplyRange(start,end,"StaticNoiseC",60,0,True,False).ConvertToRGB32()
}

Function Filter_StaticNoiseColor_wx(clip c, int "start", int "end") {
    c.ConvertToYV12().ApplyRange(start,end,"StaticNoiseC",60,0,False,True).ConvertToRGB32()
}

Function Filter_OilPainting_wx(clip c, int "start", int "end") {
    c
}

Function Overlay_wx(clip c, string overlay, int "start", int "end") {
    start = Default(start, 0)
	end = Default(end, 200000000)

	start=start*FrameRate/20
	end = end*FrameRate/20
    end = Min(end, c.framecount - 1)
    end > start ? c.ApplyRange(start, end, "Layer", Image_wx(overlay)) : c
}

Function Overlay_Video_wx(clip c, string overlay, int "start", int "end", string "mask_frag") {
    start = Default(start, 0)
	end = Default(end, 200000000)
	mask_frag = Default(mask_frag, "")

	start=start*FrameRate/20
	end = end*FrameRate/20
    end = Min(end, c.framecount - 1)

    end > start ? c.Private_Overlay_Video_wx(overlay, start, end, mask_frag) : c
}

Function Private_Overlay_Video_wx(clip c, string overlay, int start, int end, string mask_frag) {
    lay = FFVideoSource(overlay).LoopFrames(round(end - start)).BilinearResize(c.width, c.height).ConvertToRGB32()

    startclip = c.Trim1(0, round(start)-1)
    endclip = c.Trim1(round(start), 0)

    startclip + endclip.GlLayer(lay, 0, round(end - start), shader = mask_frag)
}



function FFImage_Move(clip base_clip,clip over_clip,int "x",int "y")
{
	#crop_x=x<0?-x:0
	#crop_y=y<0?-y:0
	#crop_w=(x<0 ? over_clip.width+x : x>base_clip.width-over_clip.width ? base_clip.width-x:over_clip.width)
	#crop_h=(y<0 ? over_clip.height+y : y>base_clip.height-over_clip.height ? base_clip.height-y:over_clip.height)
	
	#over_clip=((crop_w<=0||crop_h<=0||(crop_w==over_clip.width&&crop_h==over_clip.height)) ? over_clip : over_clip.Crop(crop_x,crop_y,crop_w,crop_h))
	#((crop_w!=over_clip.width||crop_h!=over_clip.height) ? over_clip.Crop(crop_x,crop_y,crop_w,crop_h) : over_clip)
	
	#crop_w<=0||crop_h<=0?base_clip:Layer(base_clip,over_clip,x=x<0?0:x,y=y<0?0:y)
	Layer(base_clip,over_clip,x=x,y=y)
}

function FFImage_Rotate(clip base_clip,clip over_clip,int "deg",int "x",int "y")
{
  over_clip=over_clip.Rotate(deg)
  Layer(base_clip,over_clip,x=x,y=y)
}


function FFImage_Zoom_tiny(clip base_clip,clip over_clip,int "x",int "y", int "width", int "height")
	{
		over_clip=over_clip.BilinearResize(width,height)
		Layer(base_clip,over_clip,x=x,y=y)
	}


function FFImage_ZoomIn(clip base_clip,clip over_clip,float "ZoomFactor",int "x",int "y")
{
  over_clip=over_clip.ZoomBox(zoomFactor=zoomFactor)
  Layer(base_clip,over_clip,x=x,y=y)
}
function FFImage_Zoom(clip base_clip,clip over_clip,float "ZoomFactor",int "w",int "h",int "x",int "y")
{
  over_clip=over_clip.ZoomBox(width=w,height=h,zoomFactor=zoomFactor)
  #over_clip=over_clip.LanczosResize(w,h)
  Layer(base_clip,over_clip,x=x,y=y)
}

function FFImage_Roll(clip base_clip,clip over_clip,int "x",int "y",int "crop_x",int "crop_y",int "crop_w",int "crop_h" )#,int "crop_x",int "crop_y",int "crop_w",int "crop_h" 
{
	over_clip=((crop_w==0||crop_h==0||(crop_w==over_clip.width&&crop_h==over_clip.height)) ? over_clip : over_clip.Crop(crop_x,crop_y,crop_w,crop_h))	
	crop_w==0||crop_h==0?base_clip:Layer(base_clip,over_clip,x=x,y=y)
}

function FFImage_Sin(clip base_clip,clip over_clip,float "angle",int "x",int "height")
{
	y=sin(angle)
	y=Int(height/2*(1-absf(y)))
	#y=Int(height/2*(1-y))
	Layer(base_clip,over_clip,x=x,y=y)
}

Function wm_image_motion_wx2(string path, int "left", int "top", int "width", int "height", int "start", int "end", string "motion", int "motion_frames", string "format") {


    left = Default(left, 0)
    top = Default(top, 0)
    width = Default(width, FrameWidth)
	height = Default(height, FrameHeight)
    start = Default(start, 0)
    end = Default(end, 200000000)
    motion = Default(motion, "None")
    motion_frames = Default(motion_frames, 2 * FrameRate)
	format = Default(format, "")

    start=start*FrameRate/20
    end=  end*FrameRate/20
    
    overclip=Image_wx(path, 1, width, height, format)
    baseclip = overclip.PointResize(FrameWidth,FrameHeight).ConvertToYV12.AverageBlur(10,10).ConvertToRGB32(matrix="Rec601", interlaced=false).loop(end)
    Apply("WM_Image_" + motion + "_wx", baseclip, overclip, left, top, width, height, start, end, motion_frames)
}


Function WM_Image_wx1(clip c, string path, int "left", int "top", int "width", int "height", int "start", int "end", string "motion", int "motion_frames", string "format") {
    left = Default(left, 0)
    top = Default(top, 0)
    width = Default(width, FrameWidth)
	height = Default(height, FrameHeight)
    start = Default(start, 0)
    end = Default(end, 200000000)
    motion = Default(motion, "None")
    motion_frames = Default(motion_frames, 2 * FrameRate)
    format = Default(format, "")
   

    start=start*FrameRate/20
    end= end*FrameRate/20
    end = Min(end, c.framecount - 1)
    end > start ? Apply("WM_Image_" + motion + "_wx", c, Image_wx(path, 1, width, height, format), left, top, width, height, start, end, motion_frames) : c
}


Function WM_Image_wx(clip c, string path, int "left", int "top", int "width", int "height", int "start", int "end", string "motion", int "motion_frames", string "format") {
    left = Default(left, 0)
    top = Default(top, 0)
    width = Default(width, FrameWidth)
	height = Default(height, FrameHeight)
    start = Default(start, 0)
    end = Default(end, 200000000)
    motion = Default(motion, "None")
    motion_frames = Default(motion_frames, 2 * FrameRate)
    format = Default(format, "")
   

    start=start*FrameRate/20
    end= end*FrameRate/20
    end = Min(end, c.framecount - 1)
    end > start ? Apply("WM_Image_" + motion + "_wx", c, Image_wx(path, 1, width, height, format), left, top, width, height, start, end, motion_frames) : c
}

# MotionEffect.CreateImageMotionEffect

Function WM_Image_None_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    c.ApplyRange(start, end, "Layer", ci, "add", 257, left, top)
}
Function WM_Image_None_tiny_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    c.ApplyRange(start, end, "Layer", ci, "add", 257, left, top)
}

###########################################
Function Private_WM_Image_Move_wx(clip c, clip ci, int start_x, int start_y, int end_x, int end_y, int start, int end, int motion_frames) {
    end_motion = start + motion_frames - 1
    end_motion = end_motion < end ? end_motion : end
    c.ApplyRange(start, end, "Animate1", start, end_motion, "FFImage_Move", ci, start_x, start_y, ci, end_x, end_y)
}

Function WM_Image_MoveLeft_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_x = FrameWidth
    start_y = top
    end_x = left
    end_y = top
    Private_WM_Image_Move_wx(c, ci, start_x, start_y, end_x, end_y, start, end, motion_frames)
}
Function WM_Image_MoveLeft_tiny_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_x = left+FrameWidth/10
    start_y = top
    end_x = left
    end_y = top
    Private_WM_Image_Move_wx(c, ci, start_x, start_y, end_x, end_y, start, end, motion_frames)
}

Function WM_Image_MoveRight_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_x = -width
    start_y = top
    end_x = left
    end_y = top
    Private_WM_Image_Move_wx(c, ci, start_x, start_y, end_x, end_y, start, end, motion_frames)
}
Function WM_Image_MoveRight_tiny_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_x = left-width/10
    start_y = top
    end_x = left
    end_y = top
    Private_WM_Image_Move_wx(c, ci, start_x, start_y, end_x, end_y, start, end, motion_frames)
}

Function WM_Image_MoveUp_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_x = left
    start_y = FrameHeight
    end_x = left
    end_y = top
    Private_WM_Image_Move_wx(c, ci, start_x, start_y, end_x, end_y, start, end, motion_frames)
}


Function WM_Image_MoveUp_tiny_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_x = left
    start_y =top+ FrameHeight/10
    end_x = left
    end_y = top
    Private_WM_Image_Move_wx(c, ci, start_x, start_y, end_x, end_y, start, end, motion_frames)
}

Function WM_Image_MoveDown_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_x = left
    start_y = -height
    end_x = left
    end_y = top
    Private_WM_Image_Move_wx(c, ci, start_x, start_y, end_x, end_y, start, end, motion_frames)
}

Function WM_Image_MoveDown_tiny_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_x = left
    start_y = top -height/10
    end_x = left
    end_y = top
    Private_WM_Image_Move_wx(c, ci, start_x, start_y, end_x, end_y, start, end, motion_frames)
}

Function WM_Image_MoveLeftUp_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_x = FrameWidth
    start_y = FrameHeight
    end_x = left
    end_y = top
    Private_WM_Image_Move_wx(c, ci, start_x, start_y, end_x, end_y, start, end, motion_frames)
}

Function WM_Image_MoveLeftUp_tiny_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_x = left + FrameWidth/10
    start_y = top +FrameHeight/10
    end_x = left
    end_y = top
    Private_WM_Image_Move_wx(c, ci, start_x, start_y, end_x, end_y, start, end, motion_frames)
}

Function WM_Image_MoveLeftDown_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_x = FrameWidth
    start_y = -height
    end_x = left
    end_y = top
    Private_WM_Image_Move_wx(c, ci, start_x, start_y, end_x, end_y, start, end, motion_frames)
}

Function WM_Image_MoveLeftDown_tiny_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_x =left+ FrameWidth/10
    start_y = top-height/10
    end_x = left
    end_y = top
    Private_WM_Image_Move_wx(c, ci, start_x, start_y, end_x, end_y, start, end, motion_frames)
}

Function WM_Image_MoveRightUp_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_x = -width
    start_y = FrameHeight
    end_x = left
    end_y = top
    Private_WM_Image_Move_wx(c, ci, start_x, start_y, end_x, end_y, start, end, motion_frames)
}

Function WM_Image_MoveRightUp_tiny_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_x = left-width/10
    start_y = top+FrameHeight/10
    end_x = left
    end_y = top
    Private_WM_Image_Move_wx(c, ci, start_x, start_y, end_x, end_y, start, end, motion_frames)
}


Function WM_Image_MoveRightDown_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_x = -width
    start_y = -height
    end_x = left
    end_y = top
    Private_WM_Image_Move_wx(c, ci, start_x, start_y, end_x, end_y, start, end, motion_frames)
}

Function WM_Image_MoveRightDown_tiny_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_x = left-width/10
    start_y = top-height
    end_x = left
    end_y = top
    Private_WM_Image_Move_wx(c, ci, start_x, start_y, end_x, end_y, start, end, motion_frames)
}

###########################################

Function WM_Image_ZoomOut_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    #~ start_factor = 1.25
    #~ end_factor = 0.75
    #~ start_x = left + int((1.0 - start_factor) / 2 * width)
    #~ start_y = top + int((1.0 - start_factor) / 2 * height)
    #~ start_w = int(start_factor * width)
    #~ start_h = int(start_factor * height)

    #~ end_motion = start + motion_frames - 1
    #~ end_motion = end_motion < end ? end_motion : end
    #~ c.ApplyRange(start, end, "Animate1", start, end_motion, "FFImage_Zoom", ci, end_factor * 100.0, start_w, start_h, start_x, start_y, ci, 100.0 / start_factor, start_w, start_h, start_x, start_y)

    WM_Image_ZoomOut_tiny_wx(c, ci, left, top, width, height, start, end, motion_frames)
}

Function WM_Image_ZoomIn_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    #~ start_factor = 0.5 * 100.0
    #~ end_factor = 1 * 100.0

    #~ end_motion = start + motion_frames - 1
    #~ end_motion = end_motion < end ? end_motion : end
    #~ c.ApplyRange(start, end, "Animate1", start, end_motion, "FFImage_ZoomIn", ci, start_factor, left, top, ci, end_factor, left, top)

    start_x = left+width/10
    start_y = top +height/10
    start_w = width*8/10
    start_h = height*8/10

    end_motion = start + motion_frames - 1
    end_motion = end_motion < end ? end_motion : end
    c.ApplyRange(start, end, "Animate1", start, end_motion, "FFImage_Zoom_tiny", ci, start_x,start_y,start_w, start_h,  ci,left, top,width,height)
}


Function WM_Image_ZoomOut_tiny_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_x = left-width/10
    start_y = top -height/10
    start_w = width*12/10
    start_h = height*12/10

    end_motion = start + motion_frames - 1
    end_motion = end_motion < end ? end_motion : end
    c.ApplyRange(start, end, "Animate1", start, end_motion, "FFImage_Zoom_tiny", ci, start_x,start_y,start_w, start_h,  ci,left, top,width,height)
}

Function WM_Image_ZoomIn_tiny_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    end_x = left-width/10
    end_y = top -height/10
    end_w = width*12/10
    end_h = height*12/10

    end_motion = start + motion_frames - 1
    end_motion = end_motion < end ? end_motion : end
    c.ApplyRange(start, end, "Animate1", start, end_motion, "FFImage_Zoom_tiny",  ci,left, top,width,height , ci, end_x,end_y,end_w, end_h)
}


###########################################
Function Private_WM_Image_RotateClock_wx(clip c, clip ci, int left, int top, int width, int height, int start, int end, int motion_frames, int start_angle, int end_angle) {
    clip_width = int(Sqrt(width * width + height * height))
    clip_width = clip_width % 4 != 0 ? clip_width + 4 - clip_width % 4 : clip_width

    over_width = (clip_width - width) / 2
    over_height = (clip_width - height) / 2

    ci = ci.AddBorders(over_width, over_height, over_width, over_height, $00000000)

    end_motion = start + motion_frames - 1
    end_motion = end_motion < end ? end_motion : end
    c.ApplyRange(start, end, "Animate1", start, end_motion, "FFImage_Rotate", ci, start_angle, left - over_width, top - over_height, ci, end_angle, left - over_width, top - over_height)
}

Function WM_Image_RotateClock45_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_angle = -45
    end_angle = 0
    Private_WM_Image_RotateClock_wx(c, ci, left, top, width, height, start, end, motion_frames, start_angle, end_angle)
}

Function WM_Image_RotateCounterClock45_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_angle = 45
    end_angle = 0
    Private_WM_Image_RotateClock_wx(c, ci, left, top, width, height, start, end, motion_frames, start_angle, end_angle)
}


Function WM_Text_wx(clip c, string path, int "left", int "top", int "width", int "height", int "start", int "end", string "motion", float "motion_frames") {
    WM_Image_wx(c, path, left, top, width, height, start, end, motion, int(motion_frames*FrameRate))
}

Function WM_VideoText_wx(clip c, string path, int "left", int "top", int "width", int "height", int "start", int "end") {
    
    duration_f= (float(end-start))/20.0

    start=start*FrameRate/20
    end= end*FrameRate/20

    ci=TextVideo(path,duration_f).tinyresize(width,height).ConvertToRGB32
    
    mask= TextVideo(path+"mask.mp4",duration_f).tinyresize(width,height).ConvertToRGB32
    ci=ci.Mask(mask)
    
    #c.ApplyRange(start, end, "Layer", ci, "add", 257, left, top)

    startclip= c.Trim1(0,round(start)-1)
    endclip=c.Trim1(round(start),0)


    (start==0)?endclip.ApplyRange(0, round(end-start) ,"Layer",ci,"add",257,left,top): startclip+endclip.ApplyRange(0, round(end-start) ,"Layer",ci,"add",257,left,top)


}

# MotionEffect.CreateTextWarterMarkMoTionEffect

Function WM_Image_LeftRoll_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_x = FrameWidth
    start_y = top
    end_x = -width
    end_y = top
    motion_frames = end - start
    Private_WM_Image_Move_wx(c, ci, start_x, start_y, end_x, end_y, start, end, motion_frames)
}

Function WM_Image_RightRoll_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_x = -width
    start_y = top
    end_x = FrameWidth
    end_y = top
    motion_frames = end - start
    Private_WM_Image_Move_wx(c, ci, start_x, start_y, end_x, end_y, start, end, motion_frames)
}

Function WM_Image_UpRoll_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_x = left
    start_y = FrameHeight
    end_x = left
    end_y = -height
    motion_frames = end - start
    Private_WM_Image_Move_wx(c, ci, start_x, start_y, end_x, end_y, start, end, motion_frames)
}

Function WM_Image_DownRoll_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_x = left
    start_y = -height
    end_x = left
    end_y = FrameHeight
    motion_frames = end - start
    Private_WM_Image_Move_wx(c, ci, start_x, start_y, end_x, end_y, start, end, motion_frames)
}

###########################################

Function WM_Image_RotateClock360_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_angle = -360
    end_angle = 0
    Private_WM_Image_RotateClock_wx(c, ci, left, top, width, height, start, end, motion_frames, start_angle, end_angle)
}

Function WM_Image_RotateCounterClock360_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_angle = 360
    end_angle = 0
    Private_WM_Image_RotateClock_wx(c, ci, left, top, width, height, start, end, motion_frames, start_angle, end_angle)
}

###########################################
Function Private_WM_Image_PartToWhole_wx(clip c, clip ci, int start_x, int start_y, int start_crop_x, int start_crop_y, int start_crop_w, int start_crop_h, int end_x, int end_y, int end_crop_x, int end_crop_y, int end_crop_w, int end_crop_h, int start, int end, int motion_frames) {
    end_motion = start + motion_frames - 1
    end_motion = end_motion < end ? end_motion : end
    c.ApplyRange(start, end, "Animate1", start, end_motion, "FFImage_Roll", ci, start_x, start_y, start_crop_x, start_crop_y, start_crop_w, start_crop_h, ci, end_x, end_y, end_crop_x, end_crop_y, end_crop_w, end_crop_h)
}

Function WM_Image_LeftPartToWhole_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_x = left
    start_y = top
    start_crop_x = width
    start_crop_y = 0
    start_crop_w = 0
    start_crop_h = height
    end_x = left
    end_y = top
    end_crop_x = 0
    end_crop_y = 0
    end_crop_w = width
    end_crop_h = height
    Private_WM_Image_PartToWhole_wx(c, ci, start_x, start_y, start_crop_x, start_crop_y, start_crop_w, start_crop_h, end_x, end_y, end_crop_x, end_crop_y, end_crop_w, end_crop_h, start, end, motion_frames)
}

Function WM_Image_RightPartToWhole_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_x = left + width
    start_y = top
    start_crop_x = 0
    start_crop_y = 0
    start_crop_w = 0
    start_crop_h = height
    end_x = left
    end_y = top
    end_crop_x = 0
    end_crop_y = 0
    end_crop_w = width
    end_crop_h = height
    Private_WM_Image_PartToWhole_wx(c, ci, start_x, start_y, start_crop_x, start_crop_y, start_crop_w, start_crop_h, end_x, end_y, end_crop_x, end_crop_y, end_crop_w, end_crop_h, start, end, motion_frames)
}

Function WM_Image_UpPartToWhole_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_x = left
    start_y = top + height
    start_crop_x = 0
    start_crop_y = 0
    start_crop_w = width
    start_crop_h = 0
    end_x = left
    end_y = top
    end_crop_x = 0
    end_crop_y = 0
    end_crop_w = width
    end_crop_h = height
    Private_WM_Image_PartToWhole_wx(c, ci, start_x, start_y, start_crop_x, start_crop_y, start_crop_w, start_crop_h, end_x, end_y, end_crop_x, end_crop_y, end_crop_w, end_crop_h, start, end, motion_frames)
}

Function WM_Image_DownPartToWhole_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    start_x = left
    start_y = top
    start_crop_x = 0
    start_crop_y = height
    start_crop_w = width
    start_crop_h = 0
    end_x = left
    end_y = top
    end_crop_x = 0
    end_crop_y = 0
    end_crop_w = width
    end_crop_h = height
    Private_WM_Image_PartToWhole_wx(c, ci, start_x, start_y, start_crop_x, start_crop_y, start_crop_w, start_crop_h, end_x, end_y, end_crop_x, end_crop_y, end_crop_w, end_crop_h, start, end, motion_frames)
}

###########################################

Function WM_Image_FadeIO_wx(clip c, clip ci, int "left", int "top", int "width", int "height", int "start", int "end", int "motion_frames") {
    ci = ci.Loop(end - start).FadeIO(motion_frames, fps=FrameRate)
    
    co = (start == 0) ? c : c.Trim1(start, -0)
    co = co.ApplyRange(0, end - start, "Layer", ci, "add", 257, left, top)
    co = (start == 0) ? co : c.Trim1(0, -start) + co
    co
}


#SetMemoryMax(250)
#SetMemoryMax(round(SetMemoryMax(0)*2))

function Image_wx(string path, int "duration", int "width", int "height", string "format", int "angle", bool "hori", bool "vert", int "cache")
{	
    duration = Default(duration, 1)
    width = Default(width, FrameWidth)
	height = Default(height, FrameHeight)
	format = Default(format, "")
	duration = duration *FrameRate/20
   
	angle = Default(angle, 0)
	hori = Default(hori, false)
	vert = Default(vert, false)
    
    rotate = (angle==90) ? 1
        \: (angle==180) ? 2
        \: (angle==270) ? 3
        \: 0
    flip_h = hori ? 8 : 0
    flip_v = vert ? 10 : 0
    WICImage(path, width=width, height=height, fps = FrameRate, rotate_flip = rotate + flip_h + flip_v, cache = cache).Loop(round(duration))
}

function Video_wx(string path, float "srcFrames", float "srcFps",float "clipStart", float "clipDuration", bool "flag", float "speed", bool "isMuted")
{
    clipStart = Default(clipStart, 0)
    clipDuration = Default(clipDuration, 0)
    speed = Default(speed, 1.0)
    isMuted = Default(isMuted, False)
    flag = Default(flag, false)

    #~ pair = flag == true ? lsopen(path, srcfps) : dshowopen(path, srcfps)
    

    #~ video= LWLibavVideoSource(path).ChangeFPS1(FrameRate)
    #~ audio= isMuted? BlankAudioOfVideo_wx(video):LWLibavAudioSource(path)
    video= FFVideoSource(path).ChangeFPS1(FrameRate)
    audio= isMuted? BlankAudioOfVideo_wx(video) : TinyAudio(path)
    audio=audio.ConvertAudioToFloat().ResampleAudio(SampleRate)

    audio = (ChannelLayout == "mono") ? audio.ConvertToMono()
    \: (audio.audiochannels >= 2) ? audio.GetChannel(1, 2)
    \: MonoToStereo(audio, audio)

    Video_wx1(video, audio, clipStart, clipDuration, speed, isMuted)
}

function Video_wx1(clip cv, clip ca, float "clipStart", float "clipDuration", float "speed", bool "isMuted")
{
    clipStart = Default(clipStart, 0)
    clipDuration = Default(clipDuration, 0)
    speed =Default(speed, 1.0)
    isMuted = Default(isMuted, False)

    v1 = cv
    a1 = ca

    v1 = (v1.framecount > 0) ? v1.Trim1(round(clipStart*FrameRate), round(-clipDuration*FrameRate)) : BlankVideo_wx(clipDuration*20, FrameWidth, FrameHeight)
    a1 = (a1.audiolength > 0) ? a1.AudioTrim1(clipStart,-clipDuration) : BlankAudio_wx(clipDuration*20)

    v1 = (speed == 1.0) ? v1 : v1.AssumeFPS(speed*FrameRate).ChangeFPS1(FrameRate)
    a1 = (speed == 1.0) ? a1 : a1.TimeStretch(tempo=speed*100,pitch=100)
    
  
    a1 = isMuted ? BlankAudioOfVideo_wx(a1) : a1

    vd = v1.framecount / v1.framerate
    a1 = a1.audioduration > vd ? a1.AudioTrim1(0, -vd)
        \: a1.audioduration < vd ? (a1 ++ BlankAudio_wx((vd - a1.audioduration + 1) * 20)).AudioTrim1(0, -vd)
        \: a1

    AudioDub(v1, a1)
}

##################################
## @ atrack  - audio track number. Default auto. If -2, ignore audio.
## @ fpsnum, fpsden - framerate. Default auto.
## @ stacked - if true, return Stack16 format.
##    (note, stacked=true requires a 16-bit color format)
## @ format  - force specified output pixel format. Default auto.
##    (see documentation for valid color formats)
##    (if stacked = true, default "YUV420P16")
## @ cache - if true (the default), create an index file.
##
function LibavSource2(string path, int "atrack", 
\          int "fpsnum", int "fpsden",
\          string "format", bool "stacked", bool "cache") 
{
    atrack   = Default(atrack, -1)
    fpsnum   = Default(fpsnum, 0)
    fpsden   = Default(fpsden,  1)
    stacked  = Default(stacked, false)
    cache    = Default(cache, true)

    format   = Default(format, "")

    FFmpegSource2(path, fpsnum=fpsnum, fpsden=fpsden, cache=cache, atrack=atrack)
}

function LibavOpen(string path, bool ismuted)
{ 
    ismuted   = Default(ismuted, false)
    #~ video= LWLibavVideoSource(path).ChangeFPS1(FrameRate)
    #~ audio= ismuted? BlankAudioOfVideo_wx(video):LWLibavAudioSource(path)
    video= FFVideoSource(path).ChangeFPS1(FrameRate)
    audio= ismuted? BlankAudioOfVideo_wx(video) : TinyAudio(path)
    audio= audio.ConvertAudioToFloat().ResampleAudio(SampleRate)
    audio = (ChannelLayout == "mono") ? audio.ConvertToMono()
    \: (audio.audiochannels >= 2) ? audio.GetChannel(1, 2)
    \: MonoToStereo(audio, audio)

    AudioDub(video,audio)
}

function LibavOpenA(string path)
{
    #~ source = LWLibavAudioSource(path)
    source = TinyAudio(path)

    a1 = source.KillVideo().ConvertAudioToFloat().ResampleAudio(SampleRate)
    a1 = (ChannelLayout == "mono") ? a1.ConvertToMono()
        \: (a1.audiochannels >= 2) ? a1.GetChannel(1, 2)
        \: MonoToStereo(a1, a1)

    a1
}

function Layer_Wx(clip c, string overpath, float c_start, float c_duration,  int x,int y,int w,int h, float over_start, float over_duration,int angleI, bool hori, bool vert, float angle, int sourcewidth, int sourceheight, string "mask", float "mask_x", float "mask_y", float "mask_w", float "mask_h", bool "mask_alpha_or_grayscale")
{
    v1 = LibavOpen(overpath,true)

    Layer_Wx1(c, v1, c_start, c_duration, x, y, w, h, over_start, over_duration, angleI, hori, vert, angle, sourcewidth, sourceheight, mask, mask_x, mask_y, mask_w, mask_h, mask_alpha_or_grayscale)
}

function Layer_Wx1(clip c, clip over, float c_start, float c_duration,  int x,int y,int w,int h, float over_start, float over_duration,int angleI, bool hori, bool vert, float angle, int sourcewidth, int sourceheight, string "mask", float "mask_x", float "mask_y", float "mask_w", float "mask_h", bool "mask_alpha_or_grayscale")
{
    v1 = over
        \.Trim1(round(over_start*FrameRate), round(over_duration*FrameRate+over_start*FrameRate))
        \.Rotate_wx(angleI, hori, vert)
        \.BilinearResize(sourcewidth, sourceheight)
        \.ConvertToRGB32()
        \.ImageMask(mask, mask_x, mask_y, mask_w, mask_h, mask_alpha_or_grayscale)
        \.Rotate(angle, width=w, height=h)

    start= c.Trim1(0,round(c_start*FrameRate)-1)
    end=c.Trim1(round(c_start*FrameRate),0)
    
    (c_start==0) ? end.ApplyRange(0, round(over_duration*FrameRate) ,"Layer",v1,"add",257,x,y) : start+end.ApplyRange(0, round(over_duration*FrameRate) ,"Layer",v1,"add",257,x,y)
}


Function Audio_wx(string path, float srcDuration, float "clipStart", float "clipDuration", float "tempo", float "pitch", float "volume", float "fadeIn", float "fadeOut")
{
	srcDuration = Default(srcDuration, 0)
	clipStart = Default(clipStart, 0)
	clipDuration = Default(clipDuration, 0)
	tempo = Default(tempo, 1)
	pitch = Default(pitch, 1)
	volume = Default(volume, 100)
	fadeIn = Default(fadeIn, 0)
	fadeOut = Default(fadeOut, 0)

    #~ c = bassopen(path)
    try {
        #~ source = LWLibavAudioSource(path)
        source = TinyAudio(path)
    } catch(err_msg) {
        source = BlankAudio_wx(srcDuration)
    }
    a1 = source.KillVideo().ConvertAudioToFloat().ResampleAudio(SampleRate)
    a1 = (ChannelLayout == "mono") ? a1.ConvertToMono()
        \: (a1.audiochannels >= 2) ? a1.GetChannel(1, 2)
        \: MonoToStereo(a1, a1)
    Audio_wx1(a1, srcDuration, clipStart, clipDuration, tempo, pitch, volume, fadeIn, fadeOut)
}

Function Audio_wx1(clip ca, float srcDuration, float "clipStart", float "clipDuration", float "tempo", float "pitch", float "volume", float "fadeIn", float "fadeOut")
{
	srcDuration = Default(srcDuration, 0)
	clipStart = Default(clipStart, 0)
	clipDuration = Default(clipDuration, 0)
	tempo = Default(tempo, 1)
	pitch = Default(pitch, 1)
	volume = Default(volume, 100)
	fadeIn = Default(fadeIn, 0)
	fadeOut = Default(fadeOut, 0)

    c = ca
    c = (c.audioduration >= srcDuration) ? c.AudioTrim1(0, -srcDuration)
        \: c.AudioTrim1(0, -c.audioduration) ++ BlankAudio_wx((srcDuration - c.audioduration) * 20)
    c = (clipStart == 0 && (clipDuration == 0 || clipDuration == srcDuration)) ? c
        \: c.AudioTrim1(clipStart * tempo, -clipDuration)
    c = (tempo == 1 && pitch == 1) ? c
        \: c.TimeStretch(tempo=tempo * 100.0, pitch=pitch * 100.0)

    Adjust_Audio_wx(c, volume, fadeIn, fadeOut)
}

Function Adjust_Audio_wx(clip ca, float "volume", float "fadeIn", float "fadeOut")
{
	volume = Default(volume, 100)
	fadeIn = Default(fadeIn, 0)
	fadeOut = Default(fadeOut, 0)

    c = ca
    c = (volume == 100) ? c
        \: c.Amplify(volume / 100.0)
    c = (fadeIn == 0) ? c
        \: c.FadeIn0(round(fadeIn * FrameRate))
    c = (fadeOut == 0) ? c
        \: c.FadeOut0(round(fadeOut * FrameRate))
    c
}

Function Rotate_wx(clip c, int v1, bool hori, bool vert)
{
    c= (v1==90) ? c.TurnRight
        \: (v1==180) ? c.Turn180
        \: (v1==270) ? c.TurnLeft
        \: c
    
    c= hori == true ? c.FlipHorizontal : c
    c= vert == true ? c.FlipVertical : c
    c
}

function Noise_wx(clip c, int "start", int "end")
{
    start = Default(start, 0.0)
	end = Default(end, 0.0)
	start = start *FrameRate/20
	end =end*FrameRate/20
	
    _crop.ConvertToYV12().ApplyRange(start,end,"StaticNoiseC",60,0,True,False).ConvertToRGB()
}

Function Hue_wx(clip c, float _hue)
{
    c.ConvertToYV12().Tweak(hue=_hue).ConvertToRGB32()
}

Function BaseBlankVideo_wx()
{
    channels = (ChannelLayout == "mono") ? 1 : 2
    return BlankClip(length=1, width=FrameWidth, height=FrameHeight, channels=channels,fps=framerate,pixel_type="RGB32").KillAudio()
}


Function BlankVideo_wx(float duration, int "width", int "height", int "color")
{
    width = Default(width, FrameWidth)
	height = Default(height, FrameHeight)
	color = Default(color, $FF000000)
    duration = duration*FrameRate/20
    #return baseBlankClip.Loop(int(duration))
    channels = (ChannelLayout == "mono") ? 1 : 2
    return BlankClip(length=round(duration), width=width, height=height, channels=channels,fps=framerate,pixel_type="RGB32",color=color,sample_type ="float").KillAudio()
}

Function BlankAudio_wx(float duration)
{
	duration = duration*FrameRate/20
    channels = (ChannelLayout == "mono") ? 1 : 2
    return BlankClip(length=round(duration), audio_rate=SampleRate, channels=channels, sample_type="float",  fps=FrameRate).KillVideo()
}

Function BlankAudioOfVideo_wx(clip c)
{
    channels = (ChannelLayout == "mono") ? 1 : 2
    return BlankClip(c, audio_rate=SampleRate, channels=channels, sample_type="float").KillVideo()
}

Function videofit_wx(clip c)
{
    c.TinyResize(FrameWidth, FrameHeight)
    #c.zoombox(FrameWidth, FrameHeight)
}

Function MixAudio_wx(clip c1, clip c2, float c1start, float c2start)
{
    c1=BlankAudio_wx(c1start)++c1
    c2=BlankAudio_wx(c2start)++c2
    return MixAudio(c1, c2)
}

Function Trim_wx(clip c, int start, int end)
{
    return c.Trim1(round(start*FrameRate/20), round(end*FrameRate/20))
}

Function AudioTrim_wx(clip c, float start, float end)
{
    return c.AudioTrim1((round(start*FrameRate))/float(FrameRate), (round(end*FrameRate))/float(FrameRate))
}

Function Zoom_wx(Clip clip, int "_startX1",int "_startX2", int "_startY1", int "_startY2", int "_endX1", int "_endX2", int "_endY1", int "_endY2", int "_StartFrame", int "_EndFrame")
{
    clip.KenBurnsEffect(startX1=_startX1,startX2=_startX2,startY1=_startY1,startY2=_startY2,endX1=_endX1,endX2=_endX2,endY1=_endY1,endY2=_endY2,StartFrame=round(_StartFrame*FrameRate/20),EndFrame=int(_EndFrame*FrameRate/20))
}

Function Transition_wx(clip c1, clip c2, float "transition_left", float "transition_right", string "method", string "arg")
{
    transition_left = Default(transition_left, 0)
    transition_right = Default(transition_right, 0)
    method = Default(method, "")
    arg = Default(arg, "")

    durl = int(round(transition_left * FrameRate))
    durr = int(round(transition_right * FrameRate))

    ((durl <= 0 && durr <= 0) || !c1.HasVideo || c1.framecount <= 0 || !c2.HasVideo || c2.framecount <= 0)
    \ ? c1 ++ c2
    \ : (method == ""
    \ ? Dissolve(c1, c2, durl + durr)
    \ : Transition(c1, c2, durl, durr, method, arg, 50))
}

function TA_Dissolve(clip at, clip bt, int ft)
{
    Dissolve(at,bt,ft)
}

function TA_Scratch_Merge(clip at, clip bt, int ft)
{
    # TransAll avisynth plugin created by V.C.Mohan
    TransScratch(at, bt,ft,"merge")
}

    function DissolveBlack(at,bt,ft)
    {
        fth=int(ft/2)
        wt=BlankClip(fth,at.width(),at.height(),"YV12",at.framerate(),color=$000000).killaudio()
        aw=Dissolve(Trim1(at,1,fth),wt,fth-1)
        bw=Dissolve(wt,Trim1(bt,fth,ft),fth)
        ct=aw.ConvertToYV12()+bw.ConvertToYV12()
        return ct.framecount()>ft ? Trim1(ct,1,ct.framecount()):ct
    }

function TA_Dissolve_Black(clip at, clip bt, int ft)
{
    DissolveBlack(at.killaudio(),bt.killaudio(),ft)
}

    function DissolveWhite(at,bt,ft)
    {
        fth=int(ft/2)
        wt=BlankClip(fth,at.width(),at.height(),"YV12",at.framerate(),color=$FFFFFF).killaudio()
        aw=Dissolve(Trim1(at,1,fth),wt,fth-1)
        bw=Dissolve(wt,Trim1(bt,fth,ft),fth)
        ct=aw.ConvertToYV12()+bw.ConvertToYV12()
        return ct.framecount()>ft ? Trim1(ct,1,ct.framecount()):ct
    }

function TA_Dissolve_White(clip at, clip bt, int ft)
{
    DissolveWhite(at.killaudio(),bt.killaudio(),ft)
}

function TA_Push_Left(clip at, clip bt, int ft)
{
    # TransAll avisynth plugin created by V.C.Mohan
    TransPush(at, bt,ft,"left")
}

function TA_Push_Right(clip at, clip bt, int ft)
{
    # TransAll avisynth plugin created by V.C.Mohan
    TransPush(at, bt,ft,"right")
}

function TA_Push_Down(clip at, clip bt, int ft)
{
    # TransAll avisynth plugin created by V.C.Mohan
    TransPush(at, bt,ft,"down")
}

function TA_Push_Up(clip at, clip bt, int ft)
{
    # TransAll avisynth plugin created by V.C.Mohan
    TransPush(at, bt,ft,"up")
}

function TA_Scratch_Dust(clip at, clip bt, int ft)
{
    # TransAll avisynth plugin created by V.C.Mohan
    TransScratch(at, bt,ft,"dust")
}

function TA_Central_In(clip at, clip bt, int ft)
{
    # TransAll avisynth plugin created by V.C.Mohan
    TransCentral(at,bt,ft)
}

function TA_Dias_Down(clip at, clip bt, int ft)
{
    # TransAll avisynth plugin created by V.C.Mohan

    diasblack=at.levels(0,1,255,0,0,true)
    half=int(ft/2)
    TransPush(at.trim1(0,half), diasblack.trim1(0,half),half+1,"down")++diasblack.trim1(0,-1)++TransPush(diasblack.trim1(0,half), bt.trim1(0,half),half+1,"up")
}

function TA_Dias_Up(clip at, clip bt, int ft)
{
    # TransAll avisynth plugin created by V.C.Mohan

    diasblack=at.levels(0,1,255,0,0,true)
    half=int(ft/2)
    TransPush(at.trim1(0,half), diasblack.trim1(0,half),half+1,"up")++diasblack.trim1(0,-1)++TransPush(diasblack.trim1(0,half), bt.trim1(0,half),half+1,"down")
}

function TA_Shuffle_Down(clip at, clip bt, int ft)
{
    # TransAll avisynth plugin created by V.C.Mohan
    TransShuffle(at, bt,ft,"down")
}

function TA_Slant_Wipe_NE(clip at, clip bt, int ft)
{
    # TransAll avisynth plugin created by V.C.Mohan 05-03-2006
    TransSlantWipe(at, bt,ft,"ne")
}

function TA_SlantRoll_Out_NE(clip at, clip bt, int ft)
{
    # TransAll avisynth plugin created by V.C.Mohan
    TransSlantRollOut(at, bt,ft,"ne",Min(at.width/4, at.height/4),127)
}

function TA_SlantRoll_Out(clip at, clip bt, int ft)
{
    # TransAll avisynth plugin created by V.C.Mohan
    TransSlantRollOut(at, bt,ft,"se",Min(at.width/4, at.height/4),127)
}

    function smartcrop (clip clip, int x, int y, int w, int h, int xo, int yo) 
    {
        black=BlankClip(width=clip.width, height=clip.height, color=$00000000, length=clip.framecount, pixel_type="rgb32").killaudio()
        c=Clip.ConvertToRGB32().Crop(x, y, w, h).ResetMask()
        Layer(black, c, "Add", 257, xo, yo)
    }

function TA_Split_Horisontal(clip at, clip bt, int ft)
{
    at=at.ConvertToRGB32()
    bt=bt.ConvertToRGB32()
    atwidth=at.width
    atheight=at.height
    #crack=int(rand(at.height-60,seed=false)/2)*2+30
    crack=int(at.width/4)*2
    at1=smartcrop(at,0,0,at.width,int(crack),0,0)
    at2=smartcrop(at,0,int(crack),at.width,int(at.height-crack),0,crack)
    #bt3=smartcrop(bt,0,int(bt.height/3)*2,bt.width,int(bt.height/3),0,int(bt.height/3)*2)

    at=Animate2(0,int(at.framecount),"layer",bt,at1,"add",257,0,0,at,at1,"add",0,0,int((crack*-1)))
    at=Animate2(0,int(bt.framecount),"layer",at,at2,"add",257,0,0,at,at2,"add",0,0,int((at.height-crack)))
    #bt=Animate2(int(at.framecount/3)*2,at.framecount-1,"layer",bt,bt3,"add",257,atwidth*-1,0,bt,bt3,"add",257,0,0)
    bt=bt.ConvertToYV12()
    at=at.ConvertToYV12()
    at
}

function TA_Split_Vertical(clip at, clip bt, int ft)
{
    at=at.ConvertToRGB32()
    bt=bt.ConvertToRGB32()
    atwidth=at.width
    atheight=at.height
    #crack=int(rand(at.height-60,seed=false)/2)*2+30
    crack=int(at.width/4)*2
    at1=smartcrop(at,0,0,int(crack),at.height,0,0)
    at2=smartcrop(at,int(at.width-crack),0,int(at.width-crack),int(at.height),int(at.width-crack),0)
    #bt3=smartcrop(bt,0,int(bt.height/3)*2,bt.width,int(bt.height/3),0,int(bt.height/3)*2)

    at=Animate2(0,int(at.framecount),"layer",bt,at1,"add",257,0,0,at,at1,"add",0,int((crack*-1)),0)
    at=Animate2(0,int(bt.framecount),"layer",at,at2,"add",257,0,0,at,at2,"add",0,int((at.width-crack)),0)
    #bt=Animate2(int(at.framecount/3)*2,at.framecount-1,"layer",bt,bt3,"add",257,atwidth*-1,0,bt,bt3,"add",257,0,0)
    bt=bt.ConvertToYV12()
    at=at.ConvertToYV12()
    at
}

function TA_Wipe_Left(clip at, clip bt, int ft)
{
    # TransAll avisynth plugin created by V.C.Mohan
    TransWipe(at, bt,ft,"left")
}

function TA_Wipe_Right(clip at, clip bt, int ft)
{
    # TransAll avisynth plugin created by V.C.Mohan
    TransWipe(at, bt,ft,"right")
}

function TA_Ripples_Center(clip at, clip bt, int ft)
{
    # TransAll avisynth plugin created by V.C.Mohan 05-03-2006
    ft < 9 ? Dissolve(at,bt,ft) : TransRipples(at,bt,ft,origin = "center",amp = 8, lambda =16)
}