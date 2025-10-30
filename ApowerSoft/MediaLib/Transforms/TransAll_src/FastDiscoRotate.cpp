/**************************************************************************
FastDiscoRotate copies a circular window from source frame to destination frame
after rotating anticlockwise through an angle degree(clockwise if degree is negative).
frame pointers, the x,y coordinates of the center of disc in each of
the frames the radius, frame heights and frame widths, pitch sizes and 
the degree
***************************************************************************/
#ifndef FastDiscoRotate_V_C_MOHAN
#define FastDiscoRotate_V_C_MOHAN
#include "math.h"
#include "FastDiscoRotate.h"

//Function


bool FastDiscoRotateRGB32(const unsigned char *srcp,unsigned char*dstp,
					  int radius,int srcx,int srcy,int srch,int srcw,
					  int dstx,int dsty,int dsth,int dstw,int spitch, int dpitch,int degree)
{
	// rotates a given circular disc area in the src frame through degree angle in anti
	// clockwise direction.
/*********************************************************************************************/
	if(srcx+radius<0 || srcx-radius>=srcw || srcy+radius<0 || srcy-radius >=srch 
		|| dstx+radius<0 || dstx-radius>=dstw || dsty+radius<0 || dsty-radius>=dsth)
		return true;	// no work to be done as either dst or src circle is
						// outside frame area
	//ensure that the part to be copied is within frames

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
	double sinalfa = sin(alfa);
	double cosalfa = cos(alfa);

	for(int h=sy; h<ey;h++)
		for(int w=sx; w<ex; w++)
		{
			int x=w-srcx;
			int y=h-srcy;
			if(h==srcy && w== srcx)	// Origin gets copied as it is
			{
				*((int *)(dstp+(dsth-1-(y+dsty))*dpitch+4*(x+dstx))) 
					=*((int*)(srcp+(srch-1-h)*spitch +4*w));
				continue;
			}
			// for points inside circle only
			if(y*y+x*x<=radius*radius )
			{
				

				if(degree==0)
				{
					*(int *)(dstp+(dsth-1-(y+dsty))*dpitch+4*(x+dstx))
					=*(int *)(srcp+(srch-1-h)*spitch+4*w);
					continue;
				}

				if(degree==90)
				{
					if(x+srcy>ey || -y+srcx >ex || x+srcy<sy || -y+srcx<sx)
						continue;
							//x=-y, y=x
					*(int *)(dstp+(dsth-1-(y+dsty))*dpitch+4*(x+dstx))
					=*(int *)(srcp+(srch-1-x-srcy)*spitch+4*(-y+srcx));
					continue;
				}
				if(degree==180)
				{
					if(-y+srcy<sy || -y+srcy>ey || -x+srcx<sx || -x+srcx>ex)
						continue;
							// x=-x, y=-y
					*(int *)(dstp+(dsth-1-(y+dsty))*dpitch+4*(x+dstx))
					=*(int *)(srcp+(srch-1-(-y+srcy))*dpitch+4*(-x+srcx));
					continue;
				}
				if(degree==270)
				{
					if(-x+srcy>ey || y+srcx >ex || -x+srcy<sy || y+srcx<sx)
						continue;
							// x=y, y=-x
					*(int *)(dstp+(dsth-1-(y+dsty))*dpitch+4*(x+dstx))
					=*(int *)(srcp+(srch-1-(-x+srcy))*dpitch+4*(y+srcx));
					continue;
				}
		
				
				double newx=x * cosalfa - y * sinalfa;
	
				int newx1=(int)(newx+0.5);
				if(newx1+srcx < sx || newx1+srcx>= ex-1)
					continue;			// point outside circle
				
				double newy= x * sinalfa + y * cosalfa;
				
				int newy1=(int)(newy+0.5);
				if(newy1+srcy < sy || newy1+srcy >= ey-1)
					continue;			// point outside circle

					*(int *)(dstp+(dsth-1-(y+dsty))*dpitch+4*(x+dstx))
						= *((int*)(srcp+(srch-1-newy1-srcy)*spitch+4*(newx1+srcx)));

				
			
			}
		}
		return true;
}

//*********************************************************************************************
bool FastDiscoRotateRGB24(const unsigned char *srcp,unsigned char*dstp,
					  int radius,int srcx,int srcy,int srch,int srcw,
					  int dstx,int dsty,int dsth,int dstw,int spitch, int dpitch,int degree)
{
	if(srcx+radius<0 || srcx-radius>=srcw || srcy+radius<0 || srcy-radius >=srch 
		|| dstx+radius<0 || dstx-radius>=dstw || dsty+radius<0 || dsty-radius>=dsth)
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
	double sinalfa = sin(alfa);
	double cosalfa = cos(alfa);

	for(int h=sy; h<ey;h++)
		for(int w=sx; w<ex; w++)
		{
			int x=w-srcx;
			int y=h-srcy;
			if(h==srcy && w== srcx)	// Origin gets copied as it is
			{
				for(int k=0;k<3;k++)
					*(dstp+(dsth-1-(y+dsty))*dpitch+3*(x+dstx)+k)
					=*(srcp+(srch-1-h)*spitch +3*w+k);
				continue;
			}
			// for points inside circle only
			if((y)*(y)+(x)*(x)<=radius*radius )
			{
				

				if(degree==0)
				{
					for(int k=0;k<3;k++)
						*(dstp+(dsth-1-(y+dsty))*dpitch+3*(x+dstx)+k)
							=*(srcp+(srch-1-h)*spitch+3*w+k);
					continue;
				}

				if(degree==90)
				{
					if(x+srcy>ey || -y+srcx >ex || x+srcy<sy || -y+srcx<sx)
						continue;
					for(int k=0;k<3;k++)
							//x=-y, y=x
						*(dstp+(dsth-1-(y+dsty))*dpitch+3*(x+dstx)+k)
							=*(srcp+(srch-1-x-srcy)*spitch+3*(-y+srcx)+k);
					continue;
				}
				if(degree==180)
				{
					if(-y+srcy<sy || -y+srcy>ey || -x+srcx<sx || -x+srcx>ex)
						continue;
					for(int k=0;k<3;k++)
							// x=-x, y=-y
						*(dstp+(dsth-1-(y+dsty))*dpitch+3*(x+dstx)+k)
							=*(srcp+(srch-1-(-y)-srcy)*dpitch+3*(-x+srcx)+k);
					continue;
				}
				if(degree==270)
				{
					if(-x+srcy>ey || y+srcx >ex || -x+srcy<sy || y+srcx<sx)
						continue;
					for(int k=0;k<3;k++)
							// x=y, y=-x
						*(dstp+(dsth-1-(y+dsty))*dpitch+3*(x+dstx)+k)
							=*(srcp+(srch-1-(-x)-srcy)*dpitch+3*(y+srcx)+k);
					continue;
				}
		
				
				double newx = (double)x * cosalfa - (double)y * sinalfa;
	
				int newx1=(int)(newx+0.5);
				if(newx1+srcx < sx || newx1+srcx>= ex-1)
					continue;			// point outside circle
				
				double newy = (double)x * sinalfa + (double)y * cosalfa;
				
				int newy1=(int)(newy+0.5);
				if(newy1+srcy < sy || newy1+srcy >= ey-1)
					continue;			// point outside circle
				
				for(int k=0;k<3;k++)
					
				{					
									//process all 3 bytes
					*(dstp+(dsth-1-(y+dsty))*dpitch+3*(x+dstx)+k)
						= *(srcp+(srch-1-newy1-srcy)*spitch+3*(newx1+srcx)+k);
				}
			
		}
	}

	return true;
}
/****************************************************/

bool FastDiscoRotateYUY2(const unsigned char *srcp,unsigned char*dstp,
					  int radius,int srcx,int srcy,int srch,int srcw,
					  int dstx,int dsty,int dsth,int dstw,int spitch, int dpitch,int degree)
{
	// rotates a given circular disc area in the src frame through degree angle in anti
	// clockwise direction.
/*********************************************************************************************/
	if(srcx+radius<0 || srcx-radius>=srcw || srcy+radius<0 || srcy-radius >=srch 
		|| dstx+radius<0 || dstx-radius>=dstw || dsty+radius<0 || dsty-radius>=dsth)
		return true;	// no work to be done as either dst or src circle is
						// outside frame area
	//ensure that the part to be copied is within frames
	srcx=srcx & 0xfffffffc;
	srcy= srcy & 0xfffffffc;
	dstx=dstx & 0xfffffffc;
	dsty= dsty & 0xfffffffc;
	radius=radius & 0xfffffffc;

	int sx=srcx-radius;
	if (sx<2)
		sx=2;
	if(dstx-(srcx-sx)<0)
		sx=sx+srcx-dstx;
	int sy=srcy-radius;
	if (sy<2)
		sy=2;
	if(dsty-(srcy-sy)<0)
		sy=sy+srcy-dsty;
	int ex=srcx+radius;
	if(ex>=srcw)
		ex=srcw-2;
	if(dstx+(ex-srcx)>dstw)
		ex=ex-(dstx-srcx);
	int ey=srcy+radius;
	if(ey>=srch)
		ey=srch-2;
	if(ey+dsty-srcy>dsth)
		ey=ey-(dsty-srcy);

	degree = degree%360;
		// no rotation
	if (degree<0)
		degree=degree+360;
	double PI=3.1415;
	double alfa = (double)degree*PI/180;
	double sinalfa = sin(alfa);
	double cosalfa = cos(alfa);

	for(int h=sy; h<ey;h++)
		for(int w=sx; w<ex; w+=2)
		{
			int x=w-srcx;
			int y=h-srcy;
			if(h==srcy && w== srcx)	// Origin gets copied as it is
			{
				*((int *)(dstp+(y+dsty)*dpitch+2*(x+dstx)))
					=*((int*)(srcp+h*spitch +2*w));
				continue;
			}
			// for points inside circle only
			if(y*y+x*x<=radius*radius )
			{

				if(degree==0)
				{
					*((int *)(dstp+(y+dsty)*dpitch+2*(x+dstx)))
					=*(int *)(srcp+h*spitch+2*w);
					continue;
				}

				
				if(degree==180)
				{
				if(-y+srcy<sy || -y+srcy>ey || -x+srcx<sx || -x+srcx>ex)
						continue;
							// x=-x, y=-y
					*((int *)(dstp+(y+dsty)*dpitch+2*(x+dstx)))
					=*(int *)(srcp+(-y+srcy)*dpitch+2*(-x+srcx));
					continue;
				}
				
				if(degree==90)
				{
					if(x+srcy>ey || -y+srcx >ex || x+srcy<sy || -y+srcx<sx )
						continue;
							//x=-y, y=x
					*((int *)(dstp+(y+dsty)*dpitch+2*(x+dstx)))
					=*(int *)(srcp+(x+srcy)*spitch+2*(-y+srcx));
					continue;
				}
				if(degree==270)
				{
					if(-x+srcy>ey || y+srcx >ex || -x+srcy<sy || y+srcx<sx )
						continue;
							// x=y, y=-x
					*((int *)(dstp+(y+dsty)*dpitch+2*(x+dstx)))
					=*(int *)(srcp+(-x+srcy)*dpitch+2*(y+srcx));
					continue;
				}
/**		
**/				
				double newx = x * cosalfa - y * sinalfa;
	
				int newx1=((int)(newx+0.5)) & 0xfffffffe;
				if(newx1+srcx < sx || newx1+srcx>= ex-1)
					continue;			// point outside circle
				
				double newy = x * sinalfa + y * cosalfa;
				
				int newy1=(int)(newy+0.5);
				if(newy1+srcy < sy || newy1+srcy >= ey-1)
					continue;			// point outside circle

					*((int *)(dstp+(y+dsty)*dpitch+2*(x+dstx)))
						= *((int*)(srcp+(newy1+srcy)*spitch+2*(newx1+srcx)));
				

				
			
			}
		}

	return true;
}
/****************************************************/
// just name change
bool FastDiscoRotateYV12(const unsigned char *srcp,unsigned char*dstp,
					  int radius,int srcx,int srcy,int srch,int srcw,
					  int dstx,int dsty,int dsth,int dstw,int spitch, int dpitch,int degree)
{
	return 	FastDiscoRotatePlane(srcp, dstp,
					  radius,srcx,srcy,srch,srcw,
					  dstx, dsty,dsth,dstw,spitch, dpitch,degree);
}
	// rotates a given circular disc area in the src frame through degree angle in anti
	// clockwise direction.
/*********************************************************************************************/
	// rotates a given circular disc area in the src frame through degree angle in anti
	// clockwise direction.
/*********************************************************************************************/
bool FastDiscoRotatePlane(const unsigned char *srcp,unsigned char*dstp,
					  int radius,int srcx,int srcy,int srch,int srcw,
					  int dstx,int dsty,int dsth,int dstw,int spitch, int dpitch,int degree)
{
	// rotates a given circular disc area in the src frame through degree angle in anti
	// clockwise direction.
/*********************************************************************************************/
	if(srcx+radius<0 || srcx-radius>=srcw || srcy+radius<0 || srcy-radius >=srch 
		|| dstx+radius<0 || dstx-radius>=dstw || dsty+radius<0 || dsty-radius>=dsth)
		return true;	// no work to be done as either dst or src circle is
						// outside frame area
	//ensure that the part to be copied is within frames
	srcx=srcx & 0xfffffffc;
	srcy= srcy & 0xfffffffc;
	dstx=dstx & 0xfffffffc;
	dsty= dsty & 0xfffffffc;
	radius=radius & 0xfffffffc;
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
	double sinalfa = sin(alfa);
	double cosalfa = cos(alfa);

	for(int h=sy; h<ey;h++)
		for(int w=sx; w<ex; w++)
		{
			int x=w-srcx;
			int y=h-srcy;
			if(h==srcy && w== srcx)	// Origin gets copied as it is
			{
				*(dstp+(y+dsty)*dpitch+(x+dstx))
					=*(srcp+h*spitch +w);
				continue;
			}
			// for points inside circle only
			if(y*y+x*x<=radius*radius )
			{
				

				if(degree==0)
				{
					*(dstp+(y+dsty)*dpitch+(x+dstx))
					=*(srcp+h*spitch+w);
					continue;
				}

				
				if(degree==180)
				{
					if(-y+srcy<sy || -y+srcy>ey || -x+srcx<sx || -x+srcx>ex)
						continue;
							// x=-x, y=-y
					*(dstp+(y+dsty)*dpitch+(x+dstx))
					=*(srcp+(-y+srcy)*dpitch+(-x+srcx));
					continue;
				}
				if(degree==90)
				{
					if(x+srcy>ey || -y+srcx >ex || x+srcy<sy || -y+srcx<sx )
						continue;
							//x=-y, y=x
					*(dstp+(y+dsty)*dpitch+(x+dstx))
					=*(srcp+(x+srcy)*spitch+(-y+srcx));
					continue;
				}
				if(degree==270)
				{
					if(-x+srcy>ey || y+srcx >ex || -x+srcy<sy || y+srcx<sx )
						continue;
							// x=y, y=-x
					*(dstp+(y+dsty)*dpitch+(x+dstx))
					=*(srcp+(-x+srcy)*dpitch+(y+srcx));
					continue;
				}
		
			
				double newx = x * cosalfa - y * sinalfa;
	
				int newx1=((int)(newx+0.5)) & 0xfffffffe;
				if(newx1+srcx < sx || newx1+srcx>= ex-1)
					continue;			// point outside circle
				
				double newy = x * sinalfa + y * cosalfa;
				
				int newy1=(int)(newy+0.5);
				if(newy1+srcy < sy || newy1+srcy >= ey-1)
					continue;			// point outside circle

					*(dstp+(y+dsty)*dpitch+(x+dstx))
						= *(srcp+(newy1+srcy)*spitch+(newx1+srcx));

				
			
			}
		}

	return true;
}

/*********************************************************************************************/
bool FastDiscoRotateYUVPlanes(const unsigned char *srcp,unsigned char*dstp,
					  int radius,int srcx,int srcy,const int srch,const int srcw,
					  int dstx,int dsty,const int dsth,const int dstw,
					  const int spitch, const int dpitch,int degree,
					  const unsigned char *srcpU,const unsigned char *srcpV,const int spitchUV, 
					  unsigned char*dstpU, unsigned char*dstpV, const int dpitchUV,
					  const int subH, const int subW)
{
	// rotates a given circular disc area in the src frame through degree angle in anti
	// clockwise direction.
/*********************************************************************************************/
	if(srcx+radius<0 || srcx-radius>=srcw || srcy+radius<0 || srcy-radius >=srch 
		|| dstx+radius<0 || dstx-radius>=dstw || dsty+radius<0 || dsty-radius>=dsth)
		return true;	// no work to be done as either dst or src circle is
						// outside frame area

	//ensure that the part to be copied is within frames
	srcx=srcx & 0xfffffffc;
	srcy= srcy & 0xfffffffc;
	dstx=dstx & 0xfffffffc;
	dsty= dsty & 0xfffffffc;
	radius=radius & 0xfffffffc;
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

	int andH = (1 << subH) - 1;
	int andW = (1 << subW) - 1;

	degree = degree%360;
		// no rotation
	if (degree<0)
		degree=degree+360;
	double PI=3.1415;
	double alfa = (double)degree*PI/180;
	double sinalfa = sin(alfa);
	double cosalfa = cos(alfa);

	for(int h = sy; h < ey;h ++)

		for(int w = sx; w < ex; w ++)
		{
			int x = w - srcx;
			int y = h - srcy;

			if(h == srcy && w == srcx)	// Origin gets copied as it is
			{
				*(dstp + (y + dsty) * dpitch + (x + dstx))
					=*(srcp + h * spitch + w);
				*(dstpU + ((y + dsty)>> subH) * dpitchUV + ((x + dstx)>> subW))
					=*(srcpU + (h >> subH) * spitchUV + (w >> subW));
				*(dstpV + ((y + dsty)>> subH) * dpitchUV + ((x + dstx)>> subW))
					=*(srcpV + (h >> subH) * spitchUV + (w >> subW));
				
			}
			// for points inside circle only
			else if(y*y+x*x<=radius*radius )
			{
				
			
				double newx = x * cosalfa - y * sinalfa;
	
				int newx1=((int)(newx+0.5)) & 0xfffffffe;

				if(newx1+srcx < sx || newx1+srcx>= ex-1)

					continue;			// point outside circle
				
				double newy = x * sinalfa + y * cosalfa;
				
				int newy1=(int)(newy+0.5);

				if(newy1+srcy < sy || newy1+srcy >= ey-1)

					continue;			// point outside circle

				*(dstp + (y + dsty) * dpitch + (x + dstx))
					=*(srcp + (newy1 + srcy) * spitch + (newx1 + srcx));

				if ( ((y + dsty) & andH) == 0 && ((x + dstx) & andW) == 0)
				{
					*(dstpU + ((y + dsty)>> subH) * dpitchUV + ((x + dstx)>> subW))
						=*(srcpU + ((newy1 + srcy) >> subH) * spitchUV + ((newx1 + srcx) >> subW));

					*(dstpV + ((y + dsty)>> subH) * dpitchUV + ((x + dstx)>> subW))
						=*(srcpV + ((newy1 + srcy) >> subH) * spitchUV + ((newx1 + srcx) >> subW));
				}

			
			}
		}

	return true;
}
#endif		