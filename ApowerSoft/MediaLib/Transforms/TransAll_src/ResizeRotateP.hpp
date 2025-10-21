/**************************************************************************
ResizeRotateP copies a circular window from source frame to destination frame
after rotating anticlockwise through an angle degree(clockwise if degree is negative).
frame pointers, the x,y coordinates of the center of disc in each of
the frames the radius, frame heights and frame widths, pitch sizes and 
the degree
***************************************************************************/
#ifndef ResizeRotateP_V_C_MOHAN
#define ResizeRotateP_V_C_MOHAN
#include "math.h"


//Function definitions
static void ResizeRotatePYV12(unsigned char*dstp,unsigned char*dstpU,unsigned char*dstpV,
					   int dsth,int dstw, int dpitch,int dpitchUV,
					  const unsigned char *srcp, const unsigned char *srcpU,const unsigned char *srcpV,
					  int srch,int srcw,int spitch,int spitchUV,
					  int degree, double zoom);
static void ResizeRotatePRGB24(unsigned char*dstp,int dsth,int dstw, int dpitch,
					  const unsigned char *srcp,int srch,int srcw,int spitch,
					  int degree, double zoom);

static void ResizeRotatePRGB32(unsigned char*dstp,int dsth,int dstw, int dpitch,
					  const unsigned char *srcp,int srch,int srcw,int spitch,
					  int degree, double zoom);

static void ResizeRotatePYUY2(unsigned char*dstp,int dsth,int dstw, int dpitch,
					  const unsigned char *srcp,int srch,int srcw,int spitch,
					  int degree, double zoom);

// for planar formats
static void ResizeRotatePlane(unsigned char*dstp, int dsth,int dstw, int dpitch,
					  const unsigned char *srcp,int srch,int srcw,int spitch,
					  int degree, double zoom);

	// All planes
static void ResizeRotatePYUV(unsigned char*dstp,unsigned char*dstpU,unsigned char*dstpV,
					   int dsth,int dstw, int dpitch,int dpitchUV,
					  const unsigned char *srcp, const unsigned char *srcpU,const unsigned char *srcpV,
					  int srch,int srcw,int spitch,int spitchUV,int subH, int subW,
					  int degree, double zoom);

//----------------------------------------------------------------------------------------------------------

//functions
static void ResizeRotatePRGB32(unsigned char*dstp,int dsth,int dstw, int dpitch,
					  const unsigned char *srcp,int srch,int srcw,int spitch,
					  int degree, double zoom)
{
	// rotates a given circular disc area in the src frame through degree angle 
	//and posts the resized frame on to output frame
/*********************************************************************************************/
	

	degree = degree%360;
	
	if (degree<0)
		degree=degree+360;
	double PI=3.1415;
	double alfa = (double)degree*PI/180;
	double sinalfa = sin(alfa);
	double cosalfa = cos(alfa);

		// Origin gets copied as it is
			
	*((int *)(dstp+(dsth/2*dpitch+4*dstw/2))) 
		=*((int *)(srcp+(srch/2*spitch+4*srcw/2)));

	int radmax = 1.0 / zoom * sqrt(srch * srch + srcw * srcw)/2;

	int hmax = radmax > dsth/2 ? dsth/2 : radmax;

	int wmax = radmax > dstw/2 ? dstw/2 : radmax;

	

	for(int h=-hmax; h<hmax;h++)
	{
		double hsinalfa = (h) * sinalfa;
		double hcosalfa = (h) * cosalfa;

		for(int w=-wmax; w<wmax; w++)
		{	
			double newx= zoom * ((w) * cosalfa - hsinalfa);

			double newy= zoom * ((w) * sinalfa + hcosalfa);

			int sx = newx + 0.5;
			int sy = newy + 0.5;

			if( sx >= srcw/2 || sx <= -srcw/2 || sy >= srch/2 || sy <= -srch/2)				

				continue;	// beyond input frame
	
				
				
						// symmetry 2 way
			*(int *)(dstp+(dsth/2+h)*dpitch+4*(dstw/2 + w))
					= *((int*)(srcp+(srch/2 + sy)*spitch+4*(srcw/2 + sx)));

				
			
		}
	}
		
}

//*********************************************************************************************
static void ResizeRotatePRGB24(unsigned char*dstp,int dsth,int dstw, int dpitch,
					  const unsigned char *srcp,int srch,int srcw,int spitch,
					  int degree, double zoom)
{
	// rotates a given circular disc area in the src frame through degree angle 
	//and posts the resized frame on to output frame
/*********************************************************************************************/
	

	degree = degree%360;
	
	if (degree<0)
		degree=degree+360;
	double PI=3.1415;
	double alfa = (double)degree*PI/180;
	double sinalfa = sin(alfa);
	double cosalfa = cos(alfa);

		// Origin gets copied as it is
	for(int k =0; k < 3; k ++)		
		*(dstp+dsth/2*dpitch+3*dstw/2+k) 
			=*(srcp+srch/2*spitch+3*srcw/2+k);

	int radmax = 1.0 / zoom * sqrt(srch * srch + srcw * srcw)/2;

	int hmax = radmax > dsth/2 ? dsth/2 : radmax;

	int wmax = radmax > dstw/2 ? dstw/2 : radmax;

	for(int h=-hmax; h<hmax;h++)
	{
		double hsinalfa = (h) * sinalfa;
		double hcosalfa = (h) * cosalfa;

		for(int w=-wmax; w<wmax; w++)
		{	
			double newx= zoom * ((w) * cosalfa - hsinalfa);

			double newy= zoom * ((w) * sinalfa + hcosalfa);

			int sx = newx + 0.5;
			int sy = newy + 0.5;

			if( sx > srcw/2 || sx <= -srcw/2 || sy > srch/2 || sy <= -srch/2)				

				continue;	// beyond input frame
	
				
			for(int k =0; k < 3; k ++)
			{
				*(dstp+(dsth/2+h)*dpitch+3*(dstw/2 + w)+k)
					= *(srcp+(srch/2 + sy)*spitch+3*(srcw/2 + sx)+k);

				
					
			}

				
			
			
		}
	}
		
}

/****************************************************/

static void ResizeRotatePYUY2(unsigned char*dstp,int dsth,int dstw, int dpitch,
					  const unsigned char *srcp,int srch,int srcw,int spitch,
					  int degree, double zoom)
{
	// rotates a given circular disc area in the src frame through degree angle 
	//and posts the resized frame on to output frame
/*********************************************************************************************/
	

	degree = degree%360;
	
	if (degree<0)
		degree=degree+360;
	double PI=3.1415;
	double alfa = (double)degree*PI/180;
	double sinalfa = sin(alfa);
	double cosalfa = cos(alfa);

		// Origin gets copied as it is
			
	*((int *)(dstp+(dsth/2*dpitch+4*dstw/2))) 
		=*((int *)(srcp+(srch/2*spitch+4*srcw/2)));

	int radmax = 1.0 / zoom * sqrt(srch * srch + srcw * srcw)/2;

	int hmax = radmax > dsth/2 ? dsth/2 : radmax;

	int wmax = (radmax > dstw/2 ? dstw/2 : radmax) & 0xfffffffe;

	for(int h=-hmax; h<hmax;h++)
	{
		double hsinalfa = (h) * sinalfa;
		double hcosalfa = (h) * cosalfa;

		for(int w=-wmax; w<wmax; w+=2)
		{	
			double newx= zoom * ((w) * cosalfa - hsinalfa);

			double newy= zoom * ((w) * sinalfa + hcosalfa);

			int sx = newx + 0.5;
			int sy = newy + 0.5;


			if( sx >= srcw/2 - 1 || sx <= -srcw/2 + 1 || sy >= srch/2 || sy <= -srch/2 + 1)			

				continue;	// beyond input frame
				

			dstp[(dsth/2+h)*dpitch+2*(dstw/2 + w)]
				= srcp[(srch/2 + sy)*spitch+2*(srcw/2 + sx)];

			
			dstp[(dsth/2+h)*dpitch+2*(dstw/2 + w)+2]
					= srcp[(srch/2 + sy)*spitch+2*(srcw/2 + sx)+2];

			if ( (sx & 1 ) == 0)
			{						

				dstp[(dsth/2+h)*dpitch+2*(dstw/2 + w)+1]
					= srcp[(srch/2 + sy)*spitch+2*(srcw/2 + sx)+1];	

				dstp[(dsth/2+h)*dpitch+2*(dstw/2 + w)+3]
					= srcp[(srch/2 + sy)*spitch+2*(srcw/2 + sx)+3];
			}

			else
			{						

				dstp[(dsth/2+h)*dpitch+2*(dstw/2 + w)+1]
					= srcp[(srch/2 + sy)*spitch+2*(srcw/2 + sx)-1];

				dstp[(dsth/2+h)*dpitch+2*(dstw/2 + w)+3]
					= srcp[(srch/2 + sy)*spitch+2*(srcw/2 + sx)+ 1];
			}

			

				

				
			
		}
	}
		
}

/****************************************************/

static void ResizeRotatePYV12(unsigned char*dstp,unsigned char*dstpU,unsigned char*dstpV,
					   int dsth,int dstw, int dpitch,int dpitchUV,
					  const unsigned char *srcp, const unsigned char *srcpU,const unsigned char *srcpV,
					  int srch,int srcw,int spitch,int spitchUV,
					  int degree, double zoom)
{
	// rotates a given circular disc area in the src frame through degree angle 
	//and posts the resized frame on to output frame
/*********************************************************************************************/
	

	degree = degree%360;
	
	if (degree<0)
		degree=degree+360;
	double PI=3.1415;
	double alfa = (double)degree*PI/180;
	double sinalfa = sin(alfa);
	double cosalfa = cos(alfa);

		// Origin gets copied as it is
			
	*(dstp+dsth/2*dpitch+dstw/2) 
		=*(srcp+srch/2*spitch+srcw/2);

	*(dstpU+dsth/4*dpitchUV+dstw/4) 
		=*(srcpU+srch/4*spitchUV+srcw/4);
	*(dstpV+dsth/4*dpitchUV+dstw/4) 
		=*(srcpV+srch/4*spitchUV+srcw/4);

	int radmax = 1.0 / zoom * sqrt(srch * srch + srcw * srcw)/2;

	//radmax &= 0xfffffffc;

	int hmax = (radmax > dsth/2 ? dsth/2 : radmax) & 0xfffffffc;

	int wmax = (radmax > dstw/2 ? dstw/2 : radmax) & 0xfffffffc;

	for(int h = - hmax; h < hmax; h++)
	{
		double hsinalfa = (h) * sinalfa;
		double hcosalfa = (h) * cosalfa;

		for(int w = - wmax; w < wmax; w++)
		{	
			double newx = zoom * ((w) * cosalfa - hsinalfa);

			double newy = zoom * ((w) * sinalfa + hcosalfa);

			int sx = newx + 0.5;
			int sy = newy + 0.5;

			if( sx >= srcw/2 || sx <= -srcw/2 || sy >= srch/2 || sy <= -srch/2)				

				continue;	// beyond input frame
			
						
			dstp[(dsth/2+h)*dpitch+(dstw/2 + w)]
				= srcp[(srch/2 + sy)*spitch+(srcw/2 + sx)];

			if ( ( h & 1) == 1 || (w & 1) == 1)

				continue;

			dstpU[(dsth/2+h)/2*dpitchUV+(dstw/2 + w)/2]
				= srcpU[(srch/2 + sy)/2*spitchUV+(srcw/2 + sx)/2];


			dstpV[(dsth/2+h)/2*dpitchUV+(dstw/2 + w)/2]
				= srcpV[(srch/2 + sy)/2*spitchUV+(srcw/2 + sx)/2];
				
			
		}
	}
		
}

/****************************************************/
// for planar formats
static void ResizeRotatePlane(unsigned char*dstp, int dsth,int dstw, int dpitch,
					  const unsigned char *srcp,int srch,int srcw,int spitch,
					  int degree, double zoom)
{
	// rotates a given circular disc area in the src plane through degree angle 
	//and posts the resized frame on to output frame
	

	degree = degree%360;
	
	if (degree<0)
		degree=degree+360;
	double PI=3.1415;
	double alfa = (double)degree*PI/180;
	double sinalfa = sin(alfa);
	double cosalfa = cos(alfa);

		// Origin gets copied as it is
			
	*(dstp+dsth/2*dpitch+dstw/2)
		
		=*(srcp+srch/2*spitch+srcw/2);

	int radmax = 1.0 / zoom * sqrt(srch * srch + srcw * srcw)/2;

	//radmax &= 0xfffffffc;

	int hmax = (radmax > dsth/2 ? dsth/2 : radmax) ;

	int wmax = (radmax > dstw/2 ? dstw/2 : radmax) ;

	for(int h = - hmax; h < hmax; h++)
	{
		double hsinalfa = (h) * sinalfa; 

		double hcosalfa = (h) * cosalfa;

		for(int w = - wmax; w < wmax; w++)
		{	
			double newx = zoom * ((w) * cosalfa - hsinalfa);

			double newy = zoom * ((w) * sinalfa + hcosalfa);

			int sx = newx + 0.5;

			int sy = newy + 0.5;

			if( sx >= srcw/2 || sx <= -srcw/2 || sy >= srch/2 || sy <= -srch/2)				

				continue;	// beyond input frame
			
						
			dstp[(dsth/2+h)*dpitch+(dstw/2 + w)]
				= srcp[(srch/2 + sy)*spitch+(srcw/2 + sx)];
			
		}
	}
		
}

//-----------------------------------------------------------------------------------------------------
static void ResizeRotatePYUV(unsigned char*dstp,unsigned char*dstpU,unsigned char*dstpV,
					   int dsth,int dstw, int dpitch,int dpitchUV,
					  const unsigned char *srcp, const unsigned char *srcpU,const unsigned char *srcpV,
					  int srch,int srcw,int spitch,int spitchUV,int subH, int subW,
					  int degree, double zoom)
{
	// rotates a given circular disc area in the src frame through degree angle 
	//and posts the resized frame on to output frame. All planar planes rotated
/*********************************************************************************************/
	
	int andH = (1 << subH) -1;

	int	andW = (1 << subW) -1;

	degree = degree%360;
	
	if (degree<0)
		degree=degree+360;
	double PI=3.1415;
	double alfa = (double)degree*PI/180;
	double sinalfa = sin(alfa);
	double cosalfa = cos(alfa);

		// Origin gets copied as it is
			
	*(dstp+dsth/2*dpitch+dstw/2) 
		=*(srcp+srch/2*spitch+srcw/2);

	*(dstpU+ (dsth >> subH)/2*dpitchUV+(dstw >> subW)/2) 
		=*(srcpU+(srch >> subH)/2*spitchUV+(srcw >> subW)/2);
	*(dstpV+(dsth >> subH)/2*dpitchUV+(dstw >> subW)/2) 
		=*(srcpV+(srch >> subH)/2*spitchUV+(srcw >> subW)/2);

	int radmax = 1.0 / zoom * sqrt(srch * srch + srcw * srcw)/2;

	//radmax &= 0xfffffffc;

	int hmax = (radmax > dsth/2 ? dsth/2 : radmax) & 0xfffffffc;

	int wmax = (radmax > dstw/2 ? dstw/2 : radmax) & 0xfffffffc;

	for(int h = - hmax; h < hmax; h++)
	{
		double hsinalfa = (h) * sinalfa;
		double hcosalfa = (h) * cosalfa;

		for(int w = - wmax; w < wmax; w++)
		{	
			double newx = zoom * ((w) * cosalfa - hsinalfa);

			double newy = zoom * ((w) * sinalfa + hcosalfa);

			int sx = newx + 0.5;
			int sy = newy + 0.5;

			if( sx >= srcw/2 || sx <= -srcw/2 || sy >= srch/2 || sy <= -srch/2)				

				continue;	// beyond input frame
			
						
			dstp[(dsth/2+h)*dpitch+(dstw/2 + w)]
				= srcp[(srch/2 + sy)*spitch+(srcw/2 + sx)];

			if ( ( h & andH) == 0 && (w & andW) == 0)
			{

				

				dstpU[((dsth/2+h) >> subH) * dpitchUV + ((dstw/2 + w) >> subW)]
						= srcpU[( (srch/2 + sy) >> subH) * spitchUV + ((srcw/2 + sx) >> subW)];


				dstpV[((dsth/2+h) >> subH) * dpitchUV + ((dstw/2 + w) >> subW)]
						= srcpV[( (srch/2 + sy) >> subH) * spitchUV + ((srcw/2 + sx) >> subW)];;
			}
				
			
		}
	}
		
}
#endif		