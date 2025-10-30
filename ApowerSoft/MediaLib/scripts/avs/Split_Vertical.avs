
#ft=20
#at=ImageReader("a.JPG",0,ft).converttoyv12().limiter(0,255).converttorgb32()
#bt=ImageReader("b.JPG",0,ft).converttoyv12().limiter(0,255).converttorgb32()
at=at.converttorgb32()
bt=bt.converttorgb32()
atwidth=at.width
atheight=at.height
#crack=int(rand(at.height-60,seed=false)/2)*2+30
crack=int(at.width/4)*2
at1=smartcrop(at,0,0,int(crack),at.height,0,0)
at2=smartcrop(at,int(at.width-crack),0,int(at.width-crack),int(at.height),int(at.width-crack),0)
#bt3=smartcrop(bt,0,int(bt.height/3)*2,bt.width,int(bt.height/3),0,int(bt.height/3)*2)

at=animate(0,int(at.framecount),"layer",bt,at1,"add",257,0,0,at,at1,"add",0,int((crack*-1)),0)
at=animate(0,int(bt.framecount),"layer",at,at2,"add",257,0,0,at,at2,"add",0,int((at.width-crack)),0)
#bt=animate(int(at.framecount/3)*2,at.framecount-1,"layer",bt,bt3,"add",257,atwidth*-1,0,bt,bt3,"add",257,0,0)
bt=bt.converttoyv12()
at=at.converttoyv12()
at

function smartCrop (clip clip, int x, int y, int w, int h, int xo, int yo) 
{
  black=blankclip(width=clip.width, height=clip.height, color=$00000000, length=clip.framecount, pixel_type="rgb32").killaudio()
  c=Clip.ConvertToRGB32().Crop(x, y, w, h).ResetMask()
  Layer(black, c, "Add", 257, xo, yo)
}