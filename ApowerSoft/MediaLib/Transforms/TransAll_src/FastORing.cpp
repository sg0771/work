/**************************************************************************
FastORing copies a circular window from source frame to destination frame.
frame pointers, the x,y coordinates of the center of disc in each of
the frames the Outer radius,thickness, frame heights and frame widths, pitch sizes 

 FastORingRotate copies a circular ring from source frame to destination frame
after rotating anticlockwise through an angle degree.
frame pointers, the x,y coordinates of the center of disc in each of
the frames the outer radius,thickness frame heights and frame widths, pitch sizes and 
the degree
***************************************************************************/
#ifndef FastORing_V_C_MOHAN
#define FastORing_V_C_MOHAN
#include "math.h"
#include "FastORing.h"



//Functions
bool FastORingRGB32(const unsigned char* srcp,  unsigned char* dstp, int radius,int thick, int srcx, int srcy, int srch, int srcw,
		int dstx, int dsty, int dsth, int dstw, int spitch, int dpitch)
{
		//ensure that the part to be copied is within frames
	if(srcx+radius<0 || srcx-radius>=srcw || srcy+radius<0 || srcy-radius >=srch 
		|| dstx+radius<0 || dstx-radius>=dstw || dsty+radius<0 || dsty-radius>=dsth ||thick>=radius || thick<1)
		return true;	// no work to be done as either dst or src circle is
						// outside frame area
	
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
		// Being 4 byte pixel format, we can process 4 bytes at atime 
	for(int h=sy;h<ey;h++)
		for(int w=sx; w<ex;w++)
			if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)<=radius*radius
					&& (h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)>=(radius-thick)*(radius-thick))
				*((int*)(dstp+(dsth-1-(h-srcy+dsty))*dpitch+4*(w-srcx+dstx)))
					=*((int*)(srcp+(srch-1-h)*spitch+4*w));
	return true;
}


bool FastORingRGB24(const unsigned char* srcp,  unsigned char* dstp, int radius,int thick, int srcx, int srcy, int srch, int srcw,
		int dstx, int dsty, int dsth, int dstw, int spitch, int dpitch)
{
	if(srcx+radius<0 || srcx-radius>=srcw || srcy+radius<0 || srcy-radius >=srch 
		|| dstx+radius<0 || dstx-radius>=dstw || dsty+radius<0 || dsty-radius>=dsth||thick>=radius || thick<1)
		return true;	// no work to be done as either dst or src circle is
						// outside frame area
	//ensure that the part to be copied is within frames
//	int rt=ringt<=0?radius:ringt;
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
	
	for(int h=sy;h<ey;h++)
		for(int w=sx; w<ex;w++)
			if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)<=radius*radius
				&& (h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)>=(radius-thick)*(radius-thick))
				for(int byte=0; byte<3;byte++)
					*(dstp+(dsth-1-(h-srcy+dsty))*dpitch+3*(w-srcx+dstx)+byte)
						=*(srcp+(srch-1-h)*spitch+3*w+byte);

	return true;
}
	

/**************************************************************************
FastORingRotate copies a circular ring from source frame to destination frame
after rotating anticlockwise through an angle degree.
frame pointers, the x,y coordinates of the center of disc in each of
the frames the outer radius,thickness frame heights and frame widths, pitch sizes and 
the degree
***************************************************************************/



bool FastORingRotateRGB32(const unsigned char *srcp,unsigned char*dstp,
					  int radius,int thick,int srcx,int srcy,int srch,int srcw,
					  int dstx,int dsty,int dsth,int dstw,int spitch, int dpitch,int degree)
{
	// rotates a given circular disc area in the src frame through degree angle in anti
	// clockwise direction.
/*********************************************************************************************/
	if(srcx+radius<0 || srcx-radius>=srcw || srcy+radius<0 || srcy-radius >=srch 
		|| dstx+radius<0 || dstx-radius>=dstw || dsty+radius<0 || dsty-radius>=dsth||thick>=radius || thick<1)
		return true;	// no work to be done as either dst or src circle is
						// outside frame area
	//ensure that the part to be copied is within frames
//	int rt=ringt<=0?radius:ringt;
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

	degree = degree%360;
		// no rotation
	if (degree<0)
		degree=degree+360;
	double PI=3.1415;
	double alfa = (double)degree*PI/180;
	

	for(int h=sy; h<ey;h++)
		for(int w=sx; w<ex; w++)
		{
			// for points inside circle only
			if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)<=radius*radius
				&& (h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)>=(radius-thick)*(radius-thick))
			{
				int x=w-srcx;
				int y=h-srcy;

				if(degree==0)
				{
					*((int *)(dstp+(dsth-1-(h-srcy+dsty))*dpitch+4*(w-srcx+dstx)))
					=*(int *)(srcp+(srch-1-h)*spitch+4*w);
					continue;
				}

				if(degree==90)
				{
					if(x+srcy>ey || -y+srcx >ex || x+srcy<sy || -y+srcx<sx)
						continue;
							//x=-y, y=x
					*((int *)(dstp+(dsth-1-(h-srcy+dsty))*dpitch+4*(w-srcx+dstx)))
					=*(int *)(srcp+(srch-1-x-srcy)*spitch+4*(-y+srcx));
					continue;
				}
				if(degree==180)
				{
							// x=-x, y=-y
					*((int *)(dstp+(dsth-1-(h-srcy+dsty))*dpitch+4*(w-srcx+dstx)))
					=*(int *)(srcp+(srch-1-(-y)-srcy)*dpitch+4*(-x+srcx));
					continue;
				}
				if(degree==270)
				{
					if(-x+srcy>ey || y+srcx >ex || -x+srcy<sy || y+srcx<sx)
						continue;
							// x=y, y=-x
					*((int *)(dstp+(dsth-1-(h-srcy+dsty))*dpitch+4*(w-srcx+dstx)))
					=*(int *)(srcp+(srch-1-(-x)-srcy)*dpitch+4*(y+srcx));
					continue;
				}
		
				
				double newx=x*cos(alfa)-y*sin(alfa);
	
				int newx1=(int)(newx+0.5);
				if(newx1+srcx < sx || newx1+srcx>= ex-1)
					continue;			// point outside circle
				
				double newy= x*sin(alfa)+y*cos(alfa);
				
				int newy1=(int)(newy+0.5);
				if(newy1+srcy < sy || newy1+srcy >= ey-1)
					continue;			// point outside circle

					*((int *)(dstp+(dsth-1-(h-srcy+dsty))*dpitch+4*(w-srcx+dstx)))
						= *((int*)(srcp+(srch-1-newy1-srcy)*spitch+4*(newx1+srcx)));

				
			
			}
		}

		return true;
}

//*********************************************************************************************
bool FastORingRotateRGB24(const unsigned char *srcp,unsigned char*dstp,
					  int radius,int thick,int srcx,int srcy,int srch,int srcw,
					  int dstx,int dsty,int dsth,int dstw,int spitch, int dpitch,int degree)
{
	if(srcx+radius<0 || srcx-radius>=srcw || srcy+radius<0 || srcy-radius >=srch 
		|| dstx+radius<0 || dstx-radius>=dstw || dsty+radius<0 || dsty-radius>=dsth||thick>=radius || thick<1)
		return true;	// no work to be done as either dst or src circle is
						// outside frame area
	//ensure that the part to be copied is within frames
//	int rt=ringt<=0?radius:ringt;
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

	degree = degree%360;
		// no rotation
	if (degree<0)
		degree=degree+360;
	double PI=3.1415;
	double alfa = (double)degree*PI/180;
	

	for(int h=sy; h<ey;h++)
		for(int w=sx; w<ex; w++)
		{
			
			// for points inside circle only
			if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)<=radius*radius &&
				(h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)>=(radius-thick)*(radius-thick))
			{
				int x=w-srcx;
				int y=h-srcy;

				if(degree==0)
				{
					for(int k=0;k<3;k++)
						*(dstp+(dsth-1-(h-srcy+dsty))*dpitch+3*(w-srcx+dstx)+k)
							=*(srcp+(srch-1-h)*spitch+3*w+k);
					continue;
				}

				if(degree==90)
				{
					if(x+srcy>ey || -y+srcx >ex || x+srcy<sy || -y+srcx<sx)
						continue;
					for(int k=0;k<3;k++)
							//x=-y, y=x
						*(dstp+(dsth-1-(h-srcy+dsty))*dpitch+3*(w-srcx+dstx)+k)
							=*(srcp+(srch-1-x-srcy)*spitch+3*(-y+srcx)+k);
					continue;
				}
				if(degree==180)
				{
					for(int k=0;k<3;k++)
							// x=-x, y=-y
						*(dstp+(dsth-1-(h-srcy+dsty))*dpitch+3*(w-srcx+dstx)+k)
							=*(srcp+(srch-1-(-y)-srcy)*dpitch+3*(-x+srcx)+k);
					continue;
				}
				if(degree==270)
				{
					if(-x+srcy>ey || y+srcx >ex || -x+srcy<sy || y+srcx<sx)
						continue;
					for(int k=0;k<3;k++)
							// x=y, y=-x
						*(dstp+(dsth-1-(h-srcy+dsty))*dpitch+3*(w-srcx+dstx)+k)
							=*(srcp+(srch-1-(-x)-srcy)*dpitch+3*(y+srcx)+k);
					continue;
				}
		
				
				double newx=(double)x*cos(alfa)-(double)y*sin(alfa);
	
				int newx1=(int)(newx+0.5);
				if(newx1+srcx < sx || newx1+srcx>= ex-1)
					continue;			// point outside circle
				
				double newy= (double)x*sin(alfa)+(double)y*cos(alfa);
				
				int newy1=(int)(newy+0.5);
				if(newy1+srcy < sy || newy1+srcy >= ey-1)
					continue;			// point outside circle
				
				for(int k=0;k<3;k++)
					
				{					
									//process all 3 bytes
					*(dstp+(dsth-1-(h-srcy+dsty))*dpitch+3*(w-srcx+dstx)+k)
						= *(srcp+(srch-1-newy1-srcy)*spitch+3*(newx1+srcx)+k);
				}
			
		}
	}
	return true;
}

bool FastORingYUY2(const unsigned char* srcp,  unsigned char* dstp, int radius,int thick, int srcx, int srcy, int srch, int srcw,
		int dstx, int dsty, int dsth, int dstw, int spitch, int dpitch)
{
		//ensure that the part to be copied is within frames
	if(srcx+radius<0 || srcx-radius>=srcw || srcy+radius<0 || srcy-radius >=srch 
		|| dstx+radius<0 || dstx-radius>=dstw || dsty+radius<0 || dsty-radius>=dsth||thick>=radius || thick<1)
		return true;	// no work to be done as either dst or src circle is
						// outside frame area
//	int rt=ringt<=0?radius:ringt;
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
		// Being 4 byte per two pixels format, we  process 4 bytes at atime 
	for(int h=sy;h<ey;h++)
		for(int w=sx; w<ex;w+=2)
			if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)<=radius*radius
					&& (h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)>=(radius-thick)*(radius-thick))
				*((int*)(dstp+(h-srcy+dsty)*dpitch+2*(w-srcx+dstx)))
					=*((int*)(srcp+h*spitch+2*w));
	return true;
}

/****************************************************/

bool FastORingRotateYUY2(const unsigned char *srcp,unsigned char*dstp,
					  int radius,int thick,int srcx,int srcy,int srch,int srcw,
					  int dstx,int dsty,int dsth,int dstw,int spitch, int dpitch,int degree)
{
	// rotates a given circular disc area in the src frame through degree angle in anti
	// clockwise direction.

	if(srcx+radius<0 || srcx-radius>=srcw || srcy+radius<0 || srcy-radius >=srch 
		|| dstx+radius<0 || dstx-radius>=dstw || dsty+radius<0 || dsty-radius>=dsth||thick>=radius || thick<1)
		return true;	// no work to be done as either dst or src circle is
						// outside frame area
	//ensure that the part to be copied is within frames
//	int rt=ringt<=0?radius:ringt;
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

	degree = degree%360;
		// no rotation
	if (degree<0)
		degree=degree+360;
	double PI=3.1415;
	double alfa = (double)degree*PI/180;	

	for(int h=sy; h<ey;h++)

		for(int w=sx; w<ex; w+=2)
		{
			// for points in the ring only
			if((h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)<=radius*radius
				&& (h-srcy)*(h-srcy)+(w-srcx)*(w-srcx)>=(radius-thick)*(radius-thick))
			{
				int x=w-srcx;
				int y=h-srcy;

				if(degree==0)
				{
					*((int*)(dstp+(h-srcy+dsty)*dpitch+2*(w-srcx+dstx)))
					=*(int *)(srcp+h*spitch+2*w);
					continue;
				}

				if(degree==90)
				{
					if(x+srcy>ey || -y+srcx >ex || x+srcy<sy || -y+srcx<sx)
						continue;
							//x=-y, y=x
					*((int*)(dstp+(h-srcy+dsty)*dpitch+2*(w-srcx+dstx)))
					=*(int *)(srcp+(x+srcy)*spitch+2*(-y+srcx));
					continue;
				}
				if(degree==180)
				{
							// x=-x, y=-y
					*((int*)(dstp+(h-srcy+dsty)*dpitch+2*(w-srcx+dstx)))
					=*(int *)(srcp+(-y+srcy)*dpitch+2*(-x+srcx));
					continue;
				}
				if(degree==270)
				{
					if(-x+srcy>ey || y+srcx >ex || -x+srcy<sy || y+srcx<sx)
						continue;
							// x=y, y=-x
					*((int*)(dstp+(h-srcy+dsty)*dpitch+2*(w-srcx+dstx)))
					=*(int *)(srcp+(-x+srcy)*dpitch+2*(y+srcx));
					continue;
				}
		
				
				double newx=x*cos(alfa)-y*sin(alfa);
	
				int newx1=((int)(newx+0.5)) & 0xfffffffe;
				if(newx1+srcx < sx || newx1+srcx>= ex-1)
					continue;			// point outside circle
				
				double newy= x*sin(alfa)+y*cos(alfa);
				
				int newy1=(int)(newy+0.5);
				if(newy1+srcy < sy || newy1+srcy >= ey-1)
					continue;			// point outside circle

					*((int*)(dstp+(h-srcy+dsty)*dpitch+2*(w-srcx+dstx)))
						= *((int*)(srcp+(newy1+srcy)*spitch+2*(newx1+srcx)));

				
			
			}
		}

		return true;
}

/*********************************************************************************************************/

bool FastORingYV12(const unsigned char* srcp,  unsigned char* dstp, int radius,int thick, int srcx, int srcy, int srch, int srcw,
		int dstx, int dsty, int dsth, int dstw, int spitch, int dpitch)
{
	FastORingPlane(srcp,  spitch,dstp, dpitch, radius,thick, srcx, srcy, srch, srcw,
					dstx, dsty);
		return true;
}
//------------------------------------------------------------------------------------------------------------
// this can be used for Y plane. For U and v if the subsamplings are equal can be used
bool FastORingPlane(const unsigned char* srcp, int spitch,  unsigned char* dstp, int dpitch, 
					int radius,int thick, int srcx, int srcy, int srch, int srcw,
					int dstx, int dsty)
{
		//ensure that the part to be copied is within frames
	if(srcx + radius < 0 || srcx - radius >= srcw || srcy + radius < 0 || srcy - radius >= srch 
		|| dstx + radius < 0 || dstx - radius >= srcw || dsty + radius < 0 || dsty - radius >= srch || thick >= radius || thick < 1)

		return true;	// no work to be done as either dst or src circle is
						// outside frame area

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
	if(dstx+(ex-srcx)>srcw)
		ex=ex-(dstx-srcx);
	int ey=srcy+radius;
	if(ey>=srch)
		ey=srch-1;
	if(ey+dsty-srcy>srch)
		ey=ey-(dsty-srcy);
		// 
	for(int h = sy;h < ey; h ++)

		for(int w = sx; w < ex;w ++)

			if((h - srcy) * (h - srcy) + (w - srcx) * (w - srcx) <= radius * radius
					&& (h - srcy) * (h - srcy) + (w - srcx) * (w - srcx) >= (radius - thick) * (radius - thick))

				*(dstp + (h - srcy + dsty) * dpitch + (w - srcx + dstx))
					=*(srcp +h * spitch + w);
	return true;
}
//--------------------------------------------------------------------------------------------------------------
// // this can be used for U and v even if the subsamplings are unequal 
bool FastORingUVPlanes(const unsigned char* srcpU,const unsigned char* srcpV,int spitchUV,
					   unsigned char* dstpU,  unsigned char* dstpV,int dpitchUV, 						 
						int subH, int subW, // subsampling of height and width
						// below give all values for Y plane
						int radius,int thick, int srch, int srcw,
						int srcx, int srcy, 
						int dstx, int dsty)
{
		//ensure that the part to be copied is within frames
	if(srcx + radius < 0 || srcx - radius >= srcw || srcy + radius < 0 || srcy - radius >= srch 
		|| dstx + radius < 0 || dstx - radius >= srcw || dsty + radius < 0 || dsty - radius >= srch || thick >= radius || thick < 1)

		return true;	// no work to be done as either dst or src circle is
						// outside frame area
	int andH = (1 << subH) -1;
	int andW = (1 << subW) -1;


	int sx = srcx - radius;
	if (sx < 1)
		sx = 1;

	if(dstx - (srcx - sx) < 0)
		sx = sx + srcx - dstx;
	int sy = srcy - radius;

	if (sy < 1)
		sy = 1;

	if(dsty - (srcy - sy) < 0)
		sy = sy + srcy - dsty;
	int ex = srcx + radius;

	if(ex >= srcw)
		ex = srcw - 1;

	if(dstx + (ex - srcx) > srcw)
		ex = ex - (dstx - srcx);

	int ey = srcy + radius;

	if(ey >= srch)
		ey = srch - 1;

	if(ey + dsty - srcy > srch)
		ey = ey - (dsty - srcy);
		// 
	for(int h = sy;h < ey; h ++)

		for(int w = sx; w < ex;w ++)

			if ( (h & andH) == 0 &&  (w & andW) == 0 )

				if((h - srcy) * (h - srcy) + (w - srcx) * (w - srcx) <= radius * radius
				&& (h - srcy) * (h - srcy) + (w - srcx) * (w - srcx) >= (radius - thick) * (radius - thick))
				{

					*(dstpU + ((h - srcy + dsty) >> subH) * dpitchUV + ((w - srcx + dstx) >> subW) )

					= *(srcpU + (h >> subH) * spitchUV + (w >> subW));

					*(dstpV + ((h - srcy + dsty) >> subH) * dpitchUV + ((w - srcx + dstx) >> subW) )

					= *(srcpV + (h >> subH) * spitchUV + (w >> subW));
				}
	return true;
}
/****************************************************/
// rotates a given circular disc area in the src frame through degree angle in anti
	// clockwise direction.
bool FastORingRotateYV12(const unsigned char *srcp,unsigned char*dstp,
					  int radius,int thick,int srcx,int srcy,int srch,int srcw,
					  int dstx,int dsty,int dsth,int dstw,int spitch, int dpitch,int degree)
{
	FastORingRotatePlane(srcp, spitch, dstp,dpitch,
					  radius,thick,srcx,srcy,srch,srcw,
					  dstx,dsty,degree);

	return true;
}

//-------------------------------------------------------------------------------------------
bool FastORingRotatePlane(const unsigned char *srcp,int spitch,unsigned char*dstp,int dpitch,
					  int radius,int thick,int srcx,int srcy,int srch,int srcw,
					  int dstx,int dsty,int degree)
	// rotates a given circular disc area in the srcp plane of frame through degree angle in anti
	// clockwise direction on to dstp plane of dst frame.

{		//ensure that the part to be copied is within frames
	if(srcx + radius < 0 || srcx - radius >= srcw || srcy + radius < 0 || srcy - radius >= srch 
		|| dstx + radius < 0 || dstx - radius >= srcw || dsty + radius < 0 || dsty - radius >= srch || thick >= radius || thick < 1)

		return true;	// no work to be done as either dst or src circle is
						// outside frame area

	int sx = srcx - radius;
	if (sx < 1)
		sx = 1;

	if(dstx - (srcx - sx) < 0)
		sx = sx + srcx - dstx;
	int sy = srcy - radius;

	if (sy < 1)
		sy = 1;

	if(dsty - (srcy - sy) < 0)
		sy = sy + srcy - dsty;
	int ex = srcx + radius;

	if(ex >= srcw)
		ex = srcw - 1;

	if(dstx + (ex - srcx) > srcw)
		ex = ex - (dstx - srcx);

	int ey = srcy + radius;

	if(ey >= srch)
		ey = srch - 1;

	if(ey + dsty - srcy > srch)
		ey = ey - (dsty - srcy);
	//  rotation
	
	degree = degree % 360;

	if (degree < 0)

		degree = degree + 360;

	double PI=3.1415;

	double alfa = (double)degree * PI / 180;
	
		
	for(int h = sy; h < ey; h ++)

		for(int w = sx; w < ex;w ++)	
		{
			// for points in the ring only
			if((h - srcy) * (h - srcy) + (w - srcx) * (w - srcx) <= radius * radius
				&& (h - srcy) * (h - srcy) + (w - srcx) * (w - srcx) >= (radius - thick) * (radius - thick))
			{
				int x = w - srcx;

				int y = h - srcy;
				
				double newx = x * cos(alfa) - y * sin(alfa);
	
				int newx1 = ((int)(newx + 0.5));

				if(newx1 + srcx < sx || newx1 + srcx >= ex-1)

					continue;			// point outside circle
				
				double newy = x * sin(alfa) + y * cos(alfa);
				
				int newy1 = (int)(newy + 0.5);

				if(newy1 + srcy < sy || newy1 + srcy >= ey - 1)

					continue;			// point outside circle

				*(dstp + (h - srcy + dsty) * dpitch + (w - srcx + dstx))
						= *(srcp + (newy1 + srcy) * spitch + (newx1 + srcx));

				
			
			}
		}
		return true;
}

//-------------------------------------------------------------------------------------------
bool FastORingRotateUVPlanes(const unsigned char *srcpU,const unsigned char *srcpV,int spitchUV,
							 unsigned char*dstpU, unsigned char*dstpV, int dpitchUV,
							 int subH, int subW,	// subsampling
							 // all following values are for Y plane
							int radius,int thick,int srcx,int srcy,int srch,int srcw,
							int dstx,int dsty,int degree)
	// rotates a given circular disc area in the srcp plane of frame through degree angle in anti
	// clockwise direction on to dstp plane of dst frame.
{
		//ensure that the part to be copied is within frames
	if(srcx + radius < 0 || srcx - radius >= srcw || srcy + radius < 0 || srcy - radius >= srch 
		|| dstx + radius < 0 || dstx - radius >= srcw || dsty + radius < 0 || dsty - radius >= srch || thick >= radius || thick < 1)

		return true;	// no work to be done as either dst or src circle is
						// outside frame area

	int sx = srcx - radius;
	if (sx < 1)
		sx = 1;

	if(dstx - (srcx - sx) < 0)
		sx = sx + srcx - dstx;
	int sy = srcy - radius;

	if (sy < 1)
		sy = 1;

	if(dsty - (srcy - sy) < 0)
		sy = sy + srcy - dsty;
	int ex = srcx + radius;

	if(ex >= srcw)
		ex = srcw - 1;

	if(dstx + (ex - srcx) > srcw)
		ex = ex - (dstx - srcx);

	int ey = srcy + radius;

	if(ey >= srch)
		ey = srch - 1;

	if(ey + dsty - srcy > srch)
		ey = ey - (dsty - srcy);
	//  rotation
	
	degree = degree % 360;

	if (degree < 0)

		degree = degree + 360;

	double PI=3.1415;

	double alfa = (double)degree * PI / 180;
	
		
	for(int h = sy; h < ey; h ++)
	{
		if ( (h & (( 1 << subH) - 1)) != 0 )

				continue;

		for(int w = sx; w < ex;w ++)
		{	
			if ((w & (( 1 << subW) - 1)) != 0)

				continue;
		
			// for points in the ring only
			if((h - srcy) * (h - srcy) + (w - srcx) * (w - srcx) <= radius * radius
				&& (h - srcy) * (h - srcy) + (w - srcx) * (w - srcx) >= (radius - thick) * (radius - thick))
			{
				int x = w - srcx;

				int y = h - srcy;
				
				double newx = x * cos(alfa) - y * sin(alfa);
	
				int newx1 = ((int)(newx + 0.5));

				if(newx1 + srcx < sx || newx1 + srcx >= ex-1)

					continue;			// point outside circle
				
				double newy = x * sin(alfa) + y * cos(alfa);
				
				int newy1 = (int)(newy + 0.5);

				if(newy1 + srcy < sy || newy1 + srcy >= ey - 1)

					continue;			// point outside circle

				*(dstpU + ( (h - srcy + dsty) >> subH) * dpitchUV + ((w - srcx + dstx) >> subW) )

						= *(srcpU + ( (newy1 + srcy) >> subH) * spitchUV + ((newx1 + srcx) >> subW) );

				*(dstpV + ( (h - srcy + dsty) >> subH) * dpitchUV + ((w - srcx + dstx) >> subW) )

						= *(srcpV + ( (newy1 + srcy) >> subH) * spitchUV + ((newx1 + srcx) >> subW) );				
			
			}
		}
	}
		return true;
}

//--------------------------------------------------------------------------------------------------------

bool FastORingRotateYUVPlanes(const unsigned char *srcp,int spitch, unsigned char *dstp,int dpitch,
							  const unsigned char *srcpU,const unsigned char *srcpV,int spitchUV,
							 unsigned char*dstpU, unsigned char*dstpV, int dpitchUV,
							 int subH, int subW,	// subsampling
							 // all following values are for Y plane
							int radius,int thick,int srcx,int srcy,int srch,int srcw,
							int dstx,int dsty,int degree)
	// rotates a given circular disc area in the srcp plane of frame through degree angle in anti
	// clockwise direction on to dstp plane of dst frame.
{
		//ensure that the part to be copied is within frames
	if(srcx + radius < 0 || srcx - radius >= srcw || srcy + radius < 0 || srcy - radius >= srch 
		|| dstx + radius < 0 || dstx - radius >= srcw || dsty + radius < 0 || dsty - radius >= srch || thick >= radius || thick < 1)

		return true;	// no work to be done as either dst or src circle is
						// outside frame area

	int sx = srcx - radius;
	if (sx < 1)
		sx = 1;

	if(dstx - (srcx - sx) < 0)
		sx = sx + srcx - dstx;
	int sy = srcy - radius;

	if (sy < 1)
		sy = 1;

	if(dsty - (srcy - sy) < 0)
		sy = sy + srcy - dsty;
	int ex = srcx + radius;

	if(ex >= srcw)
		ex = srcw - 1;

	if(dstx + (ex - srcx) > srcw)
		ex = ex - (dstx - srcx);

	int ey = srcy + radius;

	if(ey >= srch)
		ey = srch - 1;

	if(ey + dsty - srcy > srch)
		ey = ey - (dsty - srcy);
	//  rotation
	
	degree = degree % 360;

	if (degree < 0)

		degree = degree + 360;

	double PI=3.1415;

	double alfa = (double)degree * PI / 180;
	int rsq = radius * radius;
	int andH = ( 1 << subH) - 1;
	int andW = ( 1 << subW) - 1;
		
	for(int h = sy; h < ey; h ++)
	{
		int hsq = ( h- srcy) * (h - srcy);		

		for(int w = sx; w < ex;w ++)
		{		
			// for points in the ring only
			if((hsq) + (w - srcx) * (w - srcx) <= rsq

				&& (hsq) + (w - srcx) * (w - srcx) >= (radius - thick) * (radius - thick))
			{
				int x = w - srcx;

				int y = h - srcy;
				
				double newx = x * cos(alfa) - y * sin(alfa);
	
				int newx1 = ((int)(newx + 0.5));

				if(newx1 + srcx < sx || newx1 + srcx >= ex-1)

					continue;			// point outside circle
				
				double newy = x * sin(alfa) + y * cos(alfa);
				
				int newy1 = (int)(newy + 0.5);

				if(newy1 + srcy < sy || newy1 + srcy >= ey - 1)

					continue;			// point outside circle

				*(dstp + (h - srcy + dsty) * dpitch + (w - srcx + dstx) )

						= *(srcp + (newy1 + srcy)  * spitch + (newx1 + srcx) );


				if ( (h & andH) == 0 && ( w & andW) == 0 )
				{

					*(dstpU + ( (h - srcy + dsty) >> subH) * dpitchUV + ((w - srcx + dstx) >> subW) )

							= *(srcpU + ( (newy1 + srcy) >> subH) * spitchUV + ((newx1 + srcx) >> subW) );

					*(dstpV + ( (h - srcy + dsty) >> subH) * dpitchUV + ((w - srcx + dstx) >> subW) )

							= *(srcpV + ( (newy1 + srcy) >> subH) * spitchUV + ((newx1 + srcx) >> subW) );
				}
			
			}
		}
	}
		return true;
}
#endif