/**************************************************************************
FastDisco copies a circular window from source frame to destination frame.
frame pointers, the x,y coordinates of the center of disc in each of
the frames the radius, frame heights and frame widths, pitch sizes and 
 magnification(mag increase from 1 at edge to mag at center, drop magnification(mag increases from 1 at edge to mag at center
 proprtional to dist from center). 
***************************************************************************/
#ifndef FastDisco_V_C_MOHAN
#define FastDisco_V_C_MOHAN
#include "math.h"
#include "FastDisco.h"
//Function
bool FastDiscoRGB32(const unsigned char* srcp,  unsigned char* dstp, int radius, int srcx, int srcy, int srch, int srcw,
		int dstx, int dsty, int dsth, int dstw, int spitch, int dpitch, double mag, bool drop, int ringt)
{
		//ensure that the part to be copied is within frames
	if(srcx+radius<0 || srcx-radius>=srcw || srcy+radius<0 || srcy-radius >=srch 
		|| dstx+radius<0 || dstx-radius>=dstw || dsty+radius<0 || dsty-radius>=dsth || ringt>radius)
		return true;	// no work to be done as either dst or src circle is
						// outside frame area
	int rt=ringt<=0?radius:ringt;
	int sx=srcx-radius;
	if (sx<1)
		sx=1;
	if(dstx-(srcx-sx)<0)
		sx=sx+srcx-dstx;
	int sy=srcy-radius;
	if (sy<1)
		sy=1;
	if(dsty-(srcy-sy)<0)
		sy=sy+srcy-dsty;
	int ex=srcx+radius;
	if(ex>=srcw)
		ex=srcw-1;
	if(dstx+(ex-srcx)>dstw)
		ex=ex-(dstx-srcx);
	int ey=srcy+radius;
	if(ey>=srch)
		ey=srch-1;
	if(ey+dsty-srcy>dsth)
		ey=ey-(dsty-srcy);
	if(mag<=1 && mag>=-1)
	{
		// Being 4 byte pixel format, we can process 4 bytes at atime 
		for(int h=sy;h<ey;h++)
			for(int w=sx; w<ex;w++)
				if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)<=radius*radius)
					if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)>=(radius-rt)*(radius-rt))
						*((int*)(dstp+(dsth-1-(h-srcy+dsty))*dpitch+4*(w-srcx+dstx)))
							=*((int*)(srcp+(srch-1-h)*spitch+4*w));
		return true;
	}
	if(mag>1)
	{
								// magnification increases towards center
								// search area limited to external square including circle 
		for(int h=sy;h<ey;h++)		
			for(int w=sx; w<ex;w++)
				if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)>=(radius-rt)*(radius-rt))
				{
					if((srcx==w && srcy==h)||((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx))==radius*radius)
					{
								//if point is on periphery or at center then copy point
						*((int *)(dstp+(dsth-1-(h-srcy+dsty))*dpitch+4*(w-srcx+dstx)))
							=*((int *)(srcp+(srch-1-h)*spitch+4*w));
						continue;
					}
					if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)<radius*radius )
					{
					
						double dmag = mag;
						if(drop)
								//when point x,y is within circle area ie x*x +y*y <r*r
								//magnification dmag mag+ (1-mag)* sqrt(hy*hy+wx*wx)/radius
							 dmag=mag+(double)((1-mag)*sqrt((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx))/radius);
				
								//if wx is not on y axis wx not equal to zero
								//then value to be taken has x coord of wx/demag, Since this
								//is a real value we need to get nearest x coord
					
						double x0=(w-srcx)/dmag;
						int x1=(int)(x0+0.5);
						if(x0<0)
							x1=(int)(x0-0.5);
						
						int addressx1=srcx+x1;
									//If hy is not along x axis ie is non zero
									//then the y coord of value to be taken is h/dmag
								//We need to get nearest y coord							
						double y0=(h-srcy)/dmag;
						int y1=(int)(y0+0.5);
						if(y0<0)
							y1=(int)(y0-0.5);

						int addressy1=srch-1-(y1+srcy);
							
						*((int *)(dstp+(dsth-1-(h-srcy+dsty))*dpitch+4*(w-srcx+dstx)))
									=*((int*)(srcp+addressy1*spitch+4*addressx1));
													
					}
				
				}	
		
		return true;
	}

	if(mag<-1)
	{
								// magnification decreases towards center
								// search area limited to external square including circle 
		for(int h=sy;h<ey;h++)		
			for(int w=sx; w<ex;w++)
				if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)>=(radius-rt)*(radius-rt))
				{
					if(srcx==w && srcy==h)
					{
								//if  center then copy point
						*((int *)(dstp+(dsth-1-(h-srcy+dsty))*dpitch+4*(w-srcx+dstx)))
							=*((int *)(srcp+(srch-1-h)*spitch+4*w));
						continue;
					}
					if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)<=radius*radius )
					{
					
						double dmag = -mag;
						if(drop)
							dmag=1.0+(dmag-1)*sqrt((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx))/radius;
						double x0=(w-srcx)/dmag;
						int x1=(int)(x0+0.5);
						if(x0<0)
							x1=(int)(x0-0.5);
						
						int addressx1=srcx+x1;
									
									// the y coord of value to be taken is h/dmag
								//We need to get nearest y coord							
						double y0=(h-srcy)/dmag;
						int y1=(int)(y0+0.5);
						if(y0<0)
							y1=(int)(y0-0.5);

						int addressy1=srch-1-(y1+srcy);
							
						*((int *)(dstp+(dsth-1-(h-srcy+dsty))*dpitch+4*(w-srcx+dstx)))
									=*((int*)(srcp+addressy1*spitch+4*addressx1));
													
					}
				
				}	
		
		return true;
	}

	return true;

}


bool FastDiscoRGB24(const unsigned char* srcp,  unsigned char* dstp, int radius, int srcx, int srcy, int srch, int srcw,
		int dstx, int dsty, int dsth, int dstw, int spitch, int dpitch, double mag, bool drop, int ringt)
{
	if(srcx+radius<0 || srcx-radius>=srcw || srcy+radius<0 || srcy-radius >=srch 
		|| dstx+radius<0 || dstx-radius>=dstw || dsty+radius<0 || dsty-radius>=dsth || ringt>radius)
		return true;	// no work to be done as either dst or src circle is
						// outside frame area
	int rt=ringt<=0?radius:ringt;
	int sx=srcx-radius;
	if (sx<1)
		sx=1;
	if(dstx-(srcx-sx)<0)
		sx=sx+srcx-dstx;
	int sy=srcy-radius;
	if (sy<1)
		sy=1;
	if(dsty-(srcy-sy)<0)
		sy=sy+srcy-dsty;
	int ex=srcx+radius;
	if(ex>=srcw)
		ex=srcw-1;
	if(dstx+(ex-srcx)>dstw)
		ex=ex-(dstx-srcx);
	int ey=srcy+radius;
	if(ey>=srch)
		ey=srch-1;
	if(ey+dsty-srcy>dsth)
		ey=ey-(dsty-srcy);
	// we need to process each byte seperately as it is 3 byte per pixel format
	if(mag<=1)
	{
		for(int h=sy;h<ey;h++)
			for(int w=sx; w<ex;w++)
				if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)<=radius*radius)
					if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)>=(radius-rt)*(radius-rt))
						for(int byte=0; byte<3;byte++)
							*(dstp+(dsth-1-(h-srcy+dsty))*dpitch+3*(w-srcx+dstx)+byte)
								=*(srcp+(srch-1-h)*spitch+3*w+byte);

	return true;
	}

	if(mag>1)
	{
								// magnification increases towards center
								// search area limited to external square including circle 
		for(int h=sy;h<ey;h++)		
			for(int w=sx; w<ex;w++)
				if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)>=(radius-rt)*(radius-rt))
				{
					if((srcx==w && srcy==h)||((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx))==radius*radius)
					{
						for (int k=0;k<3;k++)
								//if point is on periphery or at center then copy point
							*(dstp+(dsth-1-(h-srcy+dsty))*dpitch+3*(w-srcx+dstx)+k)
								=*(srcp+(srch-1-h)*spitch+3*w+k);
						continue;
					}
					if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)<radius*radius )
					{
						double dmag=mag;
						if(drop)

								//when point x,y is within circle area ie x*x +y*y <r*r
								//magnification dmag mag+ (1-mag)* sqrt(hy*hy+wx*wx)/radius
						dmag=(mag)+(double)((1-mag)*sqrt((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx))/radius);
				
								//if wx is not on y axis wx not equal to zero
								//then value to be taken has x coord of wx/demag, Since this
								//is a real value we need to interpolate between x1 and x2
								//where x1 is integer value of x. X2 is x1+1 if x is +ve
								//and X2 is x1-1 if -ve.
					
					
						double x0=(w-srcx)/dmag;
						int x1=(int)(x0+0.5);
						if(x0<0)
							x1=(int)(x0-0.5);
						
						int addressx1=srcx+x1;
									//If hy is not along x axis ie is non zero
									//then the y coord of value to be taken is h/dmag
									//We need to get nearest y coord							
						double y0=(h-srcy)/dmag;
						int y1=(int)(y0+0.5);
						if(y0<0)
							y1=(int)(y0-0.5);

						int addressy1=srch-1-(y1+srcy);
						for (int k=0;k<3;k++)
						{
							
							*(dstp+(dsth-1-(h-srcy+dsty))*dpitch+3*(w-srcx+dstx)+k)
									=*(srcp+addressy1*spitch+3*addressx1+k);
						}

					
					}	
			}
		return true;
	}

	return true;
}
	
bool FastDiscoYUY2(const unsigned char* srcp,  unsigned char* dstp, int radius, int srcx, int srcy, int srch, int srcw,
		int dstx, int dsty, int dsth, int dstw, int spitch, int dpitch, double mag, bool drop, int ringt)
{
		//ensure that the part to be copied is within frames
	if(srcx+radius<0 || srcx-radius>=srcw || srcy+radius<0 || srcy-radius >=srch 
		|| dstx+radius<0 || dstx-radius>=dstw || dsty+radius<0 || dsty-radius>=dsth || ringt>radius)
		return true;	// no work to be done as either dst or src circle is
						// outside frame area
	int rt=ringt<=0?radius:ringt;
	int sx=srcx-radius;
	if (sx<1)
		sx=1;
	if(dstx-(srcx-sx)<0)
		sx=sx+srcx-dstx;
	int sy=srcy-radius;
	if (sy<1)
		sy=1;
	if(dsty-(srcy-sy)<0)
		sy=sy+srcy-dsty;
	int ex=srcx+radius;
	if(ex>=srcw)
		ex=srcw-1;
	if(dstx+(ex-srcx)>dstw)
		ex=ex-(dstx-srcx);
	int ey=srcy+radius;
	if(ey>=srch)
		ey=srch-1;
	if(ey+dsty-srcy>dsth)
		ey=ey-(dsty-srcy);

	if(mag<=1)
	{
		// Being 4 byte pixel format, we can process 4 bytes at atime 
		for(int h=sy;h<ey;h++)
			for(int w=sx; w<ex;w+=2)
				if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)<=radius*radius)
					if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)>=(radius-rt)*(radius-rt))
						*((int*)(dstp+(h-srcy+dsty)*dpitch+2*(w-srcx+dstx)))
							=*((int*)(srcp+h*spitch+2*w));
		return true;
	}
	if(mag>1)
	{
								// magnification increases towards center
								// search area limited to external square including circle 
		for(int h=sy;h<ey;h++)		
			for(int w=sx; w<ex;w+=2)
				if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)>=(radius-rt)*(radius-rt))
				{
					if((srcx==w && srcy==h)||((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx))==radius*radius)
					{
								//if point is on periphery or at center then copy point
						*((int *)(dstp+(h-srcy+dsty)*dpitch+2*(w-srcx+dstx)))
							=*((int *)(srcp+h*spitch+2*w));
						continue;
					}
					if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)<radius*radius )
					{
					
						double dmag = mag;
						if(drop)
								//when point x,y is within circle area ie x*x +y*y <r*r
								//magnification dmag mag+ (1-mag)* sqrt(hy*hy+wx*wx)/radius
							dmag=mag+(double)((1-mag)*sqrt((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx))/radius);
				
								//if wx is not on y axis wx not equal to zero
								//then value to be taken has x coord of wx/demag, Since this
								//is a real value we need to get nearest x coord
					
						double x0=(w-srcx)/dmag;
						int x1=(int)(x0+0.5);
						if(x0<0)
							x1=(int)(x0-0.5);
						x1=x1 & 0xfffffffe;
						
						int addressx1=srcx+x1;
									//If hy is not along x axis ie is non zero
									//then the y coord of value to be taken is h/dmag
								//We need to get nearest y coord							
						double y0=(h-srcy)/dmag;
						int y1=(int)(y0+0.5);
						if(y0<0)
							y1=(int)(y0-0.5);

						int addressy1=(y1+srcy);
							
						*((int *)(dstp+(h-srcy+dsty)*dpitch+2*(w-srcx+dstx)))
									=*((int*)(srcp+addressy1*spitch+2*addressx1));
													
					}
				
				}	
		
		return true;
	}
	return true;

}

/**************************************************************************************************************/
// just name change to be compatible with old 2.5version
bool FastDiscoYV12(const unsigned char* srcp,  unsigned char* dstp, int radius, int srcx, int srcy, int srch, int srcw,
		int dstx, int dsty, int dsth, int dstw, int spitch, int dpitch, double mag, bool drop,int ringt)
{
	return FastDiscoPlane(srcp,dstp, radius, srcx, srcy, srch, srcw,
		dstx, dsty, dsth, dstw, spitch, dpitch, mag, drop,ringt);
}
//-------------------------------------------------------------------------------------------------------------------

bool FastDiscoPlane(const unsigned char* srcp,  unsigned char* dstp, int radius, int srcx, int srcy, int srch, int srcw,
		int dstx, int dsty, int dsth, int dstw, int spitch, int dpitch, double mag, bool drop,int ringt)
{
	if(srcx+radius<0 || srcx-radius>=srcw || srcy+radius<0 || srcy-radius >=srch 
		|| dstx+radius<0 || dstx-radius>=dstw || dsty+radius<0 || dsty-radius>=dsth || ringt>radius)
		return true;	// no work to be done as either dst or src circle is
						// outside frame area or ring thickness is more than radius
	
	int rt=ringt<=0?radius:ringt;
	int sx=srcx-radius;
	if (sx<1)
		sx=1;
	if(dstx-(srcx-sx)<0)
		sx=sx+srcx-dstx;
	int sy=srcy-radius;
	if (sy<1)
		sy=1;
	if(dsty-(srcy-sy)<0)
		sy=sy+srcy-dsty;
	int ex=srcx+radius;
	if(ex>=srcw)
		ex=srcw-1;
	if(dstx+(ex-srcx)>dstw)
		ex=ex-(dstx-srcx);
	int ey=srcy+radius;
	if(ey>=srch)
		ey=srch-1;
	if(ey+dsty-srcy>dsth)
		ey=ey-(dsty-srcy);
	// we need to process each byte seperately as it is 3 byte per pixel format
	if(mag<=1)
	{
		for(int h=sy;h<ey;h++)
			for(int w=sx; w<ex;w++)
				if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)<=radius*radius)
					if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)>=(radius-rt)*(radius-rt))

					//	for(int byte=0; byte<3;byte++)
							*(dstp+(h-srcy+dsty)*dpitch+(w-srcx+dstx))
								=*(srcp+(h)*spitch+w);

	return true;
	}

	if(mag>1)
	{
								// magnification increases towards center
								// search area limited to external square including circle 
		for(int h=sy;h<ey;h++)		
			for(int w=sx; w<ex;w++)
				if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)>=(radius-rt)*(radius-rt))
				{
					if((srcx==w && srcy==h)||((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx))==radius*radius)
					{
					
								//if point is on periphery or at center then copy point
						*(dstp+(h-srcy+dsty)*dpitch+(w-srcx+dstx))
							=*(srcp+(h)*spitch+w);
						continue;
					}
					if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)<radius*radius )
					{
						double dmag=mag;
						if(drop)

								//when point x,y is within circle area ie x*x +y*y <r*r
								//magnification dmag mag+ (1-mag)* sqrt(hy*hy+wx*wx)/radius
						dmag=(mag)+(double)((1-mag)*sqrt((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx))/radius);
				
								//if wx is not on y axis wx not equal to zero
								//then value to be taken has x coord of wx/demag, Since this
								//is a real value we need to interpolate between x1 and x2
								//where x1 is integer value of x. X2 is x1+1 if x is +ve
								//and X2 is x1-1 if -ve.
					
					
						double x0=(w-srcx)/dmag;
						int x1=(int)(x0+0.5);
						if(x0<0)
							x1=(int)(x0-0.5);
						
						int addressx1=srcx+x1;
									//If hy is not along x axis ie is non zero
									//then the y coord of value to be taken is h/dmag
									//We need to get nearest y coord							
						double y0=(h-srcy)/dmag;
						int y1=(int)(y0+0.5);
						if(y0<0)
							y1=(int)(y0-0.5);
	
						*(dstp+(h-srcy+dsty)*dpitch+(w-srcx+dstx))
							=*(srcp+(y1+srcy)*spitch+addressx1);
				

					
					}	
				}
		return true;
	}

	return true;
}

//------------------------------------------------------------------------------------
bool FastDiscoYUVPlanes(const unsigned char* srcp,  unsigned char* dstp, 
						int radius, int srcx, int srcy, const int srch, const int srcw,
						int dstx, int dsty, const int dsth, const int dstw, const int spitch, const int dpitch,
						const unsigned char* srcpU,const unsigned char* srcpV,const int spitchUV,  
						unsigned char* dstpU,  unsigned char* dstpV,const int dpitchUV,
						const int subH, const int subW,
						double mag, bool drop ,int ringt )
{
	if(srcx+radius<0 || srcx-radius>=srcw || srcy+radius<0 || srcy-radius >=srch 
		|| dstx+radius<0 || dstx-radius>=dstw || dsty+radius<0 || dsty-radius>=dsth || ringt>radius)
		return true;	// no work to be done as either dst or src circle is
						// outside frame area or ring thickness is more than radius
	
	int rt=ringt<=0?radius:ringt;
	int sx=srcx-radius;
	if (sx<1)
		sx=1;
	if(dstx-(srcx-sx)<0)
		sx=sx+srcx-dstx;
	int sy=srcy-radius;
	if (sy<1)
		sy=1;
	if(dsty-(srcy-sy)<0)
		sy=sy+srcy-dsty;
	int ex=srcx+radius;
	if(ex>=srcw)
		ex=srcw-1;
	if(dstx+(ex-srcx)>dstw)
		ex=ex-(dstx-srcx);
	int ey=srcy+radius;
	if(ey>=srch)
		ey=srch-1;
	if(ey+dsty-srcy>dsth)
		ey=ey-(dsty-srcy);
	// we need to process each byte seperately as it is 3 byte per pixel format
	if(mag<=1)
	{
		for(int h=sy;h<ey;h++)
			for(int w=sx; w<ex;w++)
				if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)<=radius*radius)
					if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)>=(radius-rt)*(radius-rt))
					{
						*(dstp+(h-srcy+dsty)*dpitch+(w-srcx+dstx))							
							=*(srcp+(h)*spitch+w);

						if (  (h & (( 1 << subH ) - 1)) == 0 &&  (w & (( 1 << subW) - 1)) == 0)
						{

							*(dstpU + ((h-srcy+dsty) >> subH) * dpitchUV + ((w-srcx+dstx) >> subW))							
								= *(srcpU + (h >> subH) * spitchUV + (w >> subW));

							*(dstpV + ((h-srcy+dsty) >> subH) * dpitchUV + ((w-srcx+dstx) >> subW))							
								= *(srcpV + (h >> subH) * spitchUV + (w >> subW));
						}

					}

	return true;
	}

	if(mag>1)
	{
								// magnification increases towards center
								// search area limited to external square including circle 
		for(int h=sy;h<ey;h++)		
			for(int w=sx; w<ex;w++)
				if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)>=(radius-rt)*(radius-rt))
				{
					if((srcx==w && srcy==h)||((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx))==radius*radius)
					{
					
								//if point is on periphery or at center then copy point
						*(dstp+(h-srcy+dsty)*dpitch+(w-srcx+dstx))
							=*(srcp+(h)*spitch+w);

						*(dstpU + ((h-srcy+dsty) >> subH) * dpitchUV + ((w-srcx+dstx) >> subW))							
								= *(srcpU + (h >> subH) * spitchUV + (w >> subW));

						*(dstpV + ((h-srcy+dsty) >> subH) * dpitchUV + ((w-srcx+dstx) >> subW))							
								= *(srcpV + (h >> subH) * spitchUV + (w >> subW));

						continue;
					}
					if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)<radius*radius )
					{
						double dmag=mag;
						if(drop)

								//when point x,y is within circle area ie x*x +y*y <r*r
								//magnification dmag mag+ (1-mag)* sqrt(hy*hy+wx*wx)/radius
						dmag=(mag)+(double)((1-mag)*sqrt((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx))/radius);
				
								//if wx is not on y axis wx not equal to zero
								//then value to be taken has x coord of wx/demag, Since this
								//is a real value we need to interpolate between x1 and x2
								//where x1 is integer value of x. X2 is x1+1 if x is +ve
								//and X2 is x1-1 if -ve.
					
					
						double x0=(w-srcx)/dmag;
						int x1=(int)(x0+0.5);
						if(x0<0)
							x1=(int)(x0-0.5);
						
						int addressx1=srcx+x1;
									//If hy is not along x axis ie is non zero
									//then the y coord of value to be taken is h/dmag
									//We need to get nearest y coord							
						double y0=(h-srcy)/dmag;
						int y1=(int)(y0+0.5);
						if(y0<0)
							y1=(int)(y0-0.5);
	
						*(dstp+(h-srcy+dsty)*dpitch+(w-srcx+dstx))
							=*(srcp+(y1+srcy)*spitch+addressx1);

						if ( ( h & (( 1 << subH ) - 1) )== 0 &&  (w & (( 1 << subW) - 1)) == 0)
						{

							*(dstpU + ((h-srcy+dsty) >> subH) * dpitchUV + ((w-srcx+dstx) >> subW))							
								= *(srcpU + (h >> subH) * spitchUV + (w >> subW));

							*(dstpV + ((h-srcy+dsty) >> subH) * dpitchUV + ((w-srcx+dstx) >> subW))							
								= *(srcpV + (h >> subH) * spitchUV + (w >> subW));
						}
					
					}	
				}
		
	}

	return true;
}

// this can be used even if  x and y subsampling are different
bool FastDiscoPlaneXY(	// values for plane to be rotated
					  const unsigned char* srcp,  unsigned char* dstp,int spitch, int dpitch,
					  // radius and center coord on source and destination  y planes
					 int radius, int srcx, int srcy,int dstx,int dsty, 
					 int srch,int srcw,	// y plane height, width
					 	
					 // subsampling of h and w in U and V planes 
					const int subH, const int subW, 
					// magnification, drop effect and ring thickness on Y plane
					double mag, bool drop, int ringt)
{
	if(srcx+radius<0 || srcx-radius>=srcw || srcy+radius<0 || srcy-radius >=srch 
		|| dstx+radius<0 || dstx-radius>=srcw || dsty+radius<0 || dsty-radius>=srch || ringt>radius)
		return true;	// no work to be done as either dst or src circle is
						// outside frame area or ring thickness is more than radius
	
	int rt=ringt<=0?radius:ringt;

	
	int sx = srcx - radius;
	if (sx < 1)
		sx = 1;

	if(dstx - (srcx - sx)<0)
		sx=sx+srcx-dstx;
	int sy=srcy - radius;
	if (sy<1)
		sy=1;
	if(dsty-(srcy-sy)<0)
		sy=sy+srcy-dsty;
	int ex=srcx+radius;
	if(ex>=srcw)
		ex=srcw-1;
	if(dstx+(ex-srcx)>srcw)
		ex=ex-(dstx-srcx);
	int ey=srcy+radius;
	if(ey>=srch)
		ey=srch-1;
	if(ey+dsty-srcy>srch)
		ey=ey-(dsty-srcy);

	int andH = (1 << subH) -1;
	int andW = (1 << subW) -1;
	int radsq = radius * radius;
	int rtsq = (radius - rt) * (radius - rt);
	
	if(mag<=1)
	{
		for(int h = sy; h < ey; h ++)

			for(int w = sx; w < ex;w ++)
			{
				if( (h & andH) == 0 && (w & andW) == 0)
				{
					int xsqysq = ((h-srcy) << subH) * ((h-srcy) << subH) + ((w-srcx) << subW) * ((w-srcx) << subW);

					if( xsqysq <= radsq && xsqysq >= rtsq)

					//	if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)>=(radius-rt)*(radius-rt))
					{
						*(dstp + ((h - srcy + dsty) >> subH) * dpitch + ((w - srcx + dstx) >> subW))
							
							= * (srcp + (h >> subH) * spitch + (w >> subW));
					}
				}					
			}	
	}

	else if(mag > 1)
	{								// magnification increases towards center
								// search area limited to external square including circle 
		for(int h = sy; h < ey; h ++)

			for(int w = sx; w < ex;w ++)
			{
				if( (h & andH) == 0 && (w & andW) == 0)
				{
					int xsqysq = ((h-srcy) << subH) * ((h-srcy) << subH) + ((w-srcx) << subW) * ((w-srcx) << subW);

					if( xsqysq == rtsq || (srcx == w && srcy == h) )
					{					
								//if point is on periphery or at center then copy point
						*(dstp + ((h - srcy + dsty) >> subH) * dpitch + ((w - srcx + dstx) >> subW))
							
							= * (srcp + (h >> subH) * spitch + (w >> subW));
						
					}
					else if(xsqysq < radsq )
					{
						double dmag = mag;

						if(drop)
								//when point x,y is within circle area ie x*x +y*y <r*r
								//magnification dmag mag+ (1-mag)* sqrt(hy*hy+wx*wx)/radius
							dmag = (mag) + ((1.0 - mag) * sqrt(xsqysq) / radius);
				
								//if wx is not on y axis wx not equal to zero
								//then value to be taken has x coord of wx/demag, Since this
								//is a real value we need to interpolate between x1 and x2
								//where x1 is integer value of x. X2 is x1+1 if x is +ve
								//and X2 is x1-1 if -ve.
					
					
						double x0 = (w - srcx) / dmag;

						int x1 = x0 + 0.5;

						if(x0 < 0)
							
							x1 = x0 - 0.5;
						
					
									//If hy is not along x axis ie is non zero
									//then the y coord of value to be taken is h/dmag
									//We need to get nearest y coord							
						double y0 = (h - srcy) / dmag;

						int y1 = y0 + 0.5;

						if(y0 < 0)

							y1 = y0 - 0.5;

					
						*(dstp + ((h - srcy + dsty) >> subH) * dpitch + ((w - srcx + dstx) >> subW))
							
							= * (srcp + ((srcy + y1) >> subH) * spitch + ((srcx + x1) >> subW));
					
					}	// xsqysq < radius * radius	
				}	// if h &   w & 
			}	// for w =
	
		
	}	// else mag

	return true;
}


#endif
	