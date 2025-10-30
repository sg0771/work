#at=colorbars().Trim1(0,25).converttoyv12().killaudio()
#bt=colorbars().Trim1(0,25).invert().converttoyv12().killaudio()
#ft=5

DissolveBlack(at.killaudio(),bt.killaudio(),ft)

function DissolveBlack(at,bt,ft)
{
fth=int(ft/2)
wt=blankclip(length=fth,at.width(),at.height(),"yv12",at.framerate(),color=$000000).killaudio()
aw=dissolve(Trim1(at,1,fth),wt,fth-1)
bw=dissolve(wt,Trim1(bt,1,fth),fth)
ct=aw.converttoYV12()+bw.converttoYV12()
return ct.framecount()>ft ? Trim1(ct,1,ct.framecount()):ct
}
