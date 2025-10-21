
/*
Copyright (C) <2006, 2007,2008,2009>  <V.C.Mohan>

  Auyhor V.C.Mohan

  created/ modified on
   Feb 2006, 2007,2008, 27 Aug 2009

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    A copy of the GNU General Public License is at
    http://www.gnu.org/licenses/.
	For details of how to contact author see <http://www.avisynth.org/vcmohan/vcmohan.html> 
*/
/*********************************************************************************************
LineZ contain a set of functions that operate on a single line (vertical or Horizontal)
These are 1 Copy, 2 Flip and magnify linear or as a lens or constant value; Shading
can also be applied. These functions do not check the parameters and assume them to be correct.
Input and out pointers to the middle(except for copy and plain flip functions)
 of window, magnification, window winh( height). pitch for vertical functions  flip, shade are 
some of the parameters needed. 
**********************************************************************************************
// Definitions of Horizontal line functions
// source pointer at start destination pointer at start,  Segment width in pixels
 void CopyHLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winw);
 void CopyHLineRGB24( const unsigned char* srcp, unsigned char* dstp, int winw);
 void FlipHLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winw);
 void FlipHLineRGB24( const unsigned char* srcp, unsigned char* dstp, int winw);
// ResizeHLineRGB32 requires the srcp and dstp to point to middle of window winw/2. The factor is frame width/winw
 void ResizeHLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winw, double factor);
// ResizeHLineRGB24 requires the srcp and dstp to point to middle of window winw/2. The factor is frame width/winw
 void ResizeHLineRGB24( const unsigned char* srcp, unsigned char* dstp, int winw, double factor);
// FlipResizeHLineRGB32 requires the srcp and dstp to point to middle of window winw/2. The factor is frame width/winw
 void FlipResizeHLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winw,double factor);
// FlipResizeHLineRGB24 requires the srcp and dstp to point to middle of window winw/2. The factor is frame width/winw
 void FlipResizeHLineRGB24( const unsigned char* srcp, unsigned char* dstp, int winw,double factor);
// PeelzHLineRGB32 requires the srcp and dstp to point to middle of source window and destination windows.
// the destw normally should be touching the srcwindow towards peel direction
//source window should be double that of destination window winw.
//The line is flipped horizontally.
 void PeelzHLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winw, int shade=31);
// PeelzHLineRGB24 requires the srcp and dstp to point to middle of source window and destination windows.
// the destw normally should be touching the srcwindow towards peel direction
//source window should be double that of destination window winw.
//The line is flipped horizontally.
 void PeelzHLineRGB24( const unsigned char* srcp, unsigned char* dstp, int winw, int shade=31);
//ShadeHLineRGB24 adds shading to a line. Center gets whiter compared to edges.
// the pointers point the center of window. 
 void ShadeHLineRGB24( const unsigned char* srcp, unsigned char* dstp, int winw,int shade);
//ShadeHLineRGB32 adds shading to a line. Center gets whiter compared to edges.
// the pointers point the center of window. 
 void ShadeHLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winw,int shade);
//*************************************************************************************************
***************************************************************************************************
// Definitions of Vertical line functions
// source pointer at end pixel, destination pointer at end pixel,  Segment width in pixels
 void CopyVLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch);
 void CopyVLineRGB24( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch);
 void FlipVLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch);
 void FlipVLineRGB24( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch);
// ResizeVLineRGB32 requires the srcp and dstp to point to middle of window winh/2. The factor is frame width/winh
 void ResizeVLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,double factor);
// ResizeVLineRGB24 requires the srcp and dstp to point to middle of window winh/2. The factor is frame width/winh
 void ResizeVLineRGB24( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,double factor);
// FlipResizeVLineRGB32 requires the srcp and dstp to point to middle of window winh/2. The factor is frame width/winh
 void FlipResizeVLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,double factor);
// FlipResizeVLineRGB24 requires the srcp and dstp to point to middle of window winh/2. The factor is frame width/winh
 void FlipResizeVLineRGB24( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,double factor);
// PeelzVLineRGB32 requires the srcp and dstp to point to middle of source window and destination windows.
// the destw normally should be touching the srcwindow towards peel direction
//source window should be double that of destination window winh.
//The line is flipped horizontally.
 void PeelzVLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch, int shade=31);
// PeelzVLineRGB24 requires the srcp and dstp to point to middle of source window and destination windows.
// the destw normally should be touching the srcwindow towards peel direction
//source window should be double that of destination window winh.
//The line is flipped horizontally.
 void PeelzVLineRGB24( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch, int shade=31);
//ShadeVLineRGB24 adds shading to a line. Center gets whiter compared to edges.
// the pointers point the center of window. 
 void ShadeVLineRGB24( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,int shade);
//ShadeVLineRGB32 adds shading to a line. Center gets whiter compared to edges.
// the pointers point the center of window. 
 void ShadeVLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winw,int spitch, int dpitch,int shade);

********************************************************************************************************************
//YV12 functions
 void FlipHLineYV12( const unsigned char* srcp, unsigned char* dstp, int winw);
 void CopyHLineYV12( const unsigned char* srcp, unsigned char* dstp, int winw);
// ResizeHLineYV12 requires the srcp and dstp to point to middle of window winw/2. The factor is frame width/winw
 void ResizeHLineYV12( const unsigned char* srcp, unsigned char* dstp, int winw,double factor);
// FlipResizeHLineYV12 requires the srcp and dstp to point to middle of window winw/2. The factor is frame width/winw
 void FlipResizeHLineYV12( const unsigned char* srcp, unsigned char* dstp, int winw,double factor);
// FlipLenzHLineYV12 requires the srcp and dstp to point to middle of source window and destination windows
// The factor is source window width/destination window width winw
 void FlipLenzHLineYV12( const unsigned char* srcp, unsigned char* dstp, int winw,double factor);
// LenzHLineYV12 requires the srcp and dstp to point to middle of source window and destination windows
// The factor is source window width/destination window width winw
 void LenzHLineYV12( const unsigned char* srcp, unsigned char* dstp, int winw,double factor);
// PeelzHLineYV12 requires the srcp and dstp to point to middle of source window and destination windows.
// the destw normally should be touching the srcwindow towards peel direction
//source window should be double that of destination window winw.
//The line is flipped horizontally. Shading can be done optionally
 void PeelzHLineYV12( const unsigned char* srcp, unsigned char* dstp, int winw,int shade);
//ShadeHLineYV12 adds shading to a line. Center gets whiter compared to edges.
// the pointers point the center of window. 
 void ShadeHLineYV12( const unsigned char* srcp, unsigned char* dstp, int winw,int shade);
// Vertical Functions:-
//FlipVLineYV12 expects the srcp and dstp to be at 32 bit boundary of the start point of the segment to be copied. At a time
// two adjacent vertical lines are flipped.  
 void FlipVLineYV12( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch);
// CopyVLineYV12 expects the srcp and dstp to be at the start point of the segment to be copied. Two adjacent
//pixells are copied at a time. So two vertical lines are copied 
 void CopyVLineYV12( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch);
// FlipResizeVLineYV12 requires the srcp and dstp to point to middle of window winh/2. The factor is frame width/winh
 void FlipResizeVLineYV12( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,double factor);
// ResizeVLineYV12 requires the srcp and dstp to point to middle of window winh/2. The factor is frame width/winh
 void ResizeVLineYV12( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,double factor);
// PeelzVLineYV122 requires the srcp and dstp to point to middle of source window and destination windows.
// the destw normally should be touching the srcwindow towards peel direction
//source window should be double that of destination window winh, and divisible by 4.
//The line is flipped vertically. Shading can be done optionally  shade 0 will have no shading, -255 to 255
 void PeelzVLineYV12( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,int shade);
//ShadeVLineYV12 adds shading to a line. -255 to 255 Center gets whiter compared to edges.
// the pointers point the center of window. Two lines are copied/shaded at a time 
 void ShadeVLineYV12( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,int shade);
//***********************************************************************************
********************************************************************************************************************
//Planar functions
 void FlipHLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winw);
 void CopyHLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winw);
// ResizeHLinePlanar requires the srcp and dstp to point to middle of window winw/2. The factor is frame width/winw
 void ResizeHLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winw,double factor);
// FlipResizeHLinePlanar requires the srcp and dstp to point to middle of window winw/2. The factor is frame width/winw
 void FlipResizeHLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winw,double factor);
// FlipLenzHLinePlanar requires the srcp and dstp to point to middle of source window and destination windows
// The factor is source window width/destination window width winw
 void FlipLenzHLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winw,double factor);
// LenzHLinePlanar requires the srcp and dstp to point to middle of source window and destination windows
// The factor is source window width/destination window width winw
 void LenzHLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winw,double factor);
// PeelzHLinePlanar requires the srcp and dstp to point to middle of source window and destination windows.
// the destw normally should be touching the srcwindow towards peel direction
//source window should be double that of destination window winw.
//The line is flipped horizontally. Shading can be done optionally
 void PeelzHLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winw,int shade);
//ShadeHLinePlanar adds shading to a line. Center gets whiter compared to edges.
// the pointers point the center of window. 
 void ShadeHLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winw,int shade);
// Vertical Functions:-
//FlipVLinePlanar expects the srcp and dstp to be at 32 bit boundary of the start point of the segment to be copied. At a time
// two adjacent vertical lines are flipped.  
 void FlipVLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch);
// CopyVLinePlanar expects the srcp and dstp to be at the start point of the segment to be copied. Two adjacent
//pixells are copied at a time. So two vertical lines are copied 
 void CopyVLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch);
// FlipResizeVLinePlanar requires the srcp and dstp to point to middle of window winh/2. The factor is frame width/winh
 void FlipResizeVLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,double factor);
// ResizeVLinePlanar requires the srcp and dstp to point to middle of window winh/2. The factor is frame width/winh
 void ResizeVLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,double factor);
// PeelzVLinePlanar2 requires the srcp and dstp to point to middle of source window and destination windows.
// the destw normally should be touching the srcwindow towards peel direction
//source window should be double that of destination window winh, and divisible by 4.
//The line is flipped vertically. Shading can be done optionally  shade 0 will have no shading, -255 to 255
 void PeelzVLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,int shade);
//ShadeVLinePlanar adds shading to a line. -255 to 255 Center gets whiter compared to edges.
// the pointers point the center of window. Two lines are copied/shaded at a time 
 void ShadeVLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,int shade);
//***********************************************************************************
//YUV2 Format Functions
// Vertical Line Functions:-
//FlipVLineYUY2 expects the srcp and dstp to be at 32 bit boundary of the start point of the segment to be copied. At a time
// two adjacent vertical lines are flipped.  
 void FlipVLineYUY2( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch);
// CopyVLineYUY2 expects the srcp and dstp to be at the start point of the segment to be copied. Two adjacent
//pixells are copied at a time. So two vertical lines are copied 
 void CopyVLineYUY2( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch);
// FlipResizeVLineYUY2 requires the srcp and dstp to point to middle of window winh/2. The factor is frame width/winh
 void FlipResizeVLineYUY2( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,double factor);
// ResizeVLineYUY2 requires the srcp and dstp to point to middle of window winh/2. The factor is frame width/winh
 void ResizeVLineYUY2( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,double factor);
// PeelzVLineYUY22 requires the srcp and dstp to point to middle of source window and destination windows.
// the destw normally should be touching the srcwindow towards peel direction
//source window should be double that of destination window winh, and divisible by 4.
//The line is flipped vertically. Shading can be done optionally  shade 0 will have no shading, -255 to 255
 void PeelzVLineYUY2( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,int shade);
//ShadeVLineYUY2 adds shading to a line. -255 to 255 Center gets whiter compared to edges.
// the pointers point the center of window. Two lines are copied/shaded at a time 
 void ShadeVLineYUY2( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,int shade);
************************************************************************************************************
//Horizontal line functions
 void FlipHLineYUY2( const unsigned char* srcp, unsigned char* dstp, int winw);
 void CopyHLineYUY2( const unsigned char* srcp, unsigned char* dstp, int winw);
// ResizeHLineYUY2 requires the srcp and dstp to point to middle of window winw/2. The factor is frame width/winw
 void ResizeHLineYUY2( const unsigned char* srcp, unsigned char* dstp, int winw,double factor);
// FlipResizeHLineYUY2 requires the srcp and dstp to point to middle of window winw/2. The factor is frame width/winw
 void FlipResizeHLineYUY2( const unsigned char* srcp, unsigned char* dstp, int winw,double factor);
// FlipLenzHLineYUY2 requires the srcp and dstp to point to middle of source window and destination windows
// The factor is source window width/destination window width winw
 void FlipLenzHLineYUY2( const unsigned char* srcp, unsigned char* dstp, int winw,double factor);
// LenzHLineYUY2 requires the srcp and dstp to point to middle of source window and destination windows
// The factor is source window width/destination window width winw
 void LenzHLineYUY2( const unsigned char* srcp, unsigned char* dstp, int winw,double factor);
// PeelzHLineYUY2 requires the srcp and dstp to point to middle of source window and destination windows.
// the destw normally should be touching the srcwindow towards peel direction
//source window should be double that of destination window winw.
//The line is flipped horizontally. Shading can be done optionally
 void PeelzHLineYUY2( const unsigned char* srcp, unsigned char* dstp, int winw,int shade);
//ShadeHLineYUY2 adds shading to a line. Center gets whiter compared to edges.
// the pointers point the center of window. 
 void ShadeHLineYUY2( const unsigned char* srcp, unsigned char* dstp, int winw,int shade);

//**************************************************************************************************/
#ifndef Linez_V_C_MOHAN
#define Linez_V_C_MOHAN
#include "LineZ.h"
//Functions for Horizontal lines 

 void CopyHLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winw)
{
	for(int w=0;w<winw;w++)
		*((int*)(dstp+4*w))=*((int *)(srcp+4*w));
}

 void CopyHLineRGB24( const unsigned char* srcp, unsigned char* dstp, int winw)
{
	for(int w=0;w<3*winw;w++)
		*((dstp+w))=*((srcp+w));
}

 void CopyHLineYUY2( const unsigned char* srcp, unsigned char* dstp, int winw)
{


	for(int w=0;w<winw;w++)
	{
		dstp[w <<1]= srcp[w << 1];


		dstp[1 + (w <<1)]= srcp[1 + (w <<1)];
	}
}

 void FlipHLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winw)
{
	for(int w=0; w<winw; w++)
		*((int*)(dstp+4*w))=*((int *)(srcp+4*(winw-1-w)));
}

 void FlipHLineRGB24( const unsigned char* srcp, unsigned char* dstp, int winw)
{
	for(int w=0; w<winw; w++)
		for(int k=0;k<3;k++)
			*(dstp+3*w+k)=*(srcp+3*(winw-1-w)+k);
}
 void FlipHLineYUY2( const unsigned char* srcp, unsigned char* dstp, int winw)
{
	for(int w=0; w<winw/2; w++)
		*((int*)(dstp+4*w))=(((*((int *)(srcp+4*(winw-1-w))))& 0xff000000)>>16) |
							(((*((int *)(srcp+4*(winw-1-w))))& 0xff00)<<16) |
							((*((int *)(srcp+4*(winw-1-w))))& 0xff00ff);
}

// ResizeHLineRGB32 requires the srcp and dstp to point to middle of window winw/2. The factor is frame width/winw
 void ResizeHLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winw,double factor)
{
						// nearest point interpolation	
	for(int w=0; w<winw/2; w++)
	{
		
		
		*((int *)(dstp+4*w))=*((int *)(srcp+4*((int)(factor*w))));
		*((int *)(dstp-4*w))=*((int *)(srcp-4*((int)(factor*w))));
		
	}
}

// ResizeHLineYUY2 requires the srcp and dstp to point to middle of window winw/2. The factor is frame width/winw
 void ResizeHLineYUY2( const unsigned char* srcp, unsigned char* dstp, int winw,double factor)
{
						// nearest point interpolation. 2 points at a time. So bad approx	
	for(int w=0; w<winw/2; w+=2)
	{
		
		
		*((int *)(dstp+2*w))=*((int *)(srcp+2*(((int)(factor*w))& 0xFFFFFFFE)));
		*((int *)(dstp-2*w))=*((int *)(srcp-2*(((int)(factor*w))& 0xFFFFFFFE)));
		
	}
}
// ResizeHLineRGB24 requires the srcp and dstp to point to middle of window winw/2. The factor is frame width/winw
 void ResizeHLineRGB24( const unsigned char* srcp, unsigned char* dstp, int winw,double factor)
{
						// nearest point interpolation	
	for(int w=0; w<winw/2; w++)
	{
		for(int k=0; k<3; k++)
		{
			*(dstp+3*w+k)=*(srcp+3*((int)(factor*w))+k);
			*(dstp-3*w+k)=*(srcp-3*((int)(factor*w))+k);
		}
	}
}

// FlipResizeHLineRGB32 requires the srcp and dstp to point to middle of window winw/2. The factor is frame width/winw
 void FlipResizeHLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winw,double factor)
{
						// nearest point interpolation	
	for(int w=0; w<winw/2; w++)
	{
		*((int *)(dstp+4*w))=*((int *)(srcp-4*((int)(factor*w))));
		*((int *)(dstp-4*w))=*((int *)(srcp+4*((int)(factor*w))));
	}
}

// FlipResizeHLineYUY2 requires the srcp and dstp to point to middle of window winw/2. The factor is frame width/winw
 void FlipResizeHLineYUY2( const unsigned char* srcp, unsigned char* dstp, int winw,double factor)
{
						// nearest point interpolation. two pointse. Bad approx	
	for(int w=0; w<winw/2; w++)
	{
		*((int *)(dstp+2*w))=*((int *)(srcp-2*(((int)(factor*w))& 0xFFFFFFFE)));
		*((int *)(dstp-2*w))=*((int *)(srcp+2*(((int)(factor*w))& 0xFFFFFFFE)));
	}
}
// FlipResizeHLineRGB24 requires the srcp and dstp to point to middle of window winw/2. The factor is frame width/winw
 void FlipResizeHLineRGB24( const unsigned char* srcp, unsigned char* dstp, int winw,double factor)
{
						// nearest point interpolation	
	for(int w=0; w<winw/2; w++)
	{
		for(int k=0; k<3; k++)
		{
			*(dstp+3*w+k)=*(srcp-3*((int)(factor*w))+k);
			*(dstp-3*w+k)=*(srcp+3*((int)(factor*w))+k);
		}
	}
}

//

// FlipLenzHLineRGB24 requires the srcp and dstp to point to middle of source window and destination windows
// The factor is source window width/destination window width winw
 void FlipLenzHLineRGB24( const unsigned char* srcp, unsigned char* dstp, int winw,double factor)
{
	for(int w=0; w<winw/2; w++)
	{
		for(int k=0; k<3; k++)
		{
			*(dstp+3*w+k)=*(srcp-3*((int)((factor+(1-factor)*w*2/winw)*w))+k);
			*(dstp-3*w+k)=*(srcp+3*((int)((factor+(1-factor)*w*2/winw)*w))+k);
		}
	}
}


// LenzHLineRGB24 requires the srcp and dstp to point to middle of source window and destination windows
// The factor is source window width/destination window width winw
 void LenzHLineRGB24( const unsigned char* srcp, unsigned char* dstp, int winw,double factor)
{
	for(int w=0; w<winw/2; w++)
	{
		for(int k=0; k<3; k++)
		{
			*(dstp+3*w+k)=*(srcp+3*((int)((factor+(1-factor)*w*2/winw)*w))+k);
			*(dstp-3*w+k)=*(srcp-3*((int)((factor+(1-factor)*w*2/winw)*w))+k);
		}
	}
}

// FlipLenzHLineRGB32 requires the srcp and dstp to point to middle of source window and destination windows
// The factor is source window width/destination window width winw
 void FlipLenzHLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winw,double factor)
{
	for(int w=0; w<winw/2; w++)
	{
		
		*((int *)(dstp+4*w))=*((int *)(srcp-4*((int)((factor+(1-factor)*w*2/winw)*w))));
		*((int *)(dstp-4*w))=*((int *)(srcp+4*((int)((factor+(1-factor)*w*2/winw)*w))));
		
	}
}


// LenzHLineRGB32 requires the srcp and dstp to point to middle of source window and destination windows
// The factor is source window width/destination window width winw
 void LenzHLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winw,double factor)
{
	for(int w=0; w<winw/2; w++)
	{
		
		*((int *)(dstp+4*w))=*((int *)(srcp+4*((int)((factor+(1-factor)*w*2/winw)*w))));
		*((int *)(dstp-4*w))=*((int *)(srcp-4*((int)((factor+(1-factor)*w*2/winw)*w))));
		
	}
}

// FlipLenzHLineYUY2 requires the srcp and dstp to point to middle of source window and destination windows
// The factor is source window width/destination window width winw
 void FlipLenzHLineYUY2( const unsigned char* srcp, unsigned char* dstp, int winw,double factor)
{
	for(int w=0; w<winw/2; w++)
	{
		
		*((int *)(dstp+2*w))=*((int *)(srcp-2*((int)((factor+(1-factor)*w*2/winw)*w))));
		*((int *)(dstp-2*w))=*((int *)(srcp+2*((int)((factor+(1-factor)*w*2/winw)*w))));
		
	}
}


// LenzHLineYUY2 requires the srcp and dstp to point to middle of source window and destination windows
// The factor is source window width/destination window width winw
 void LenzHLineYUY2( const unsigned char* srcp, unsigned char* dstp, int winw,double factor)
{
	for(int w=0; w<winw/2; w++)
	{
		
		*((int *)(dstp+2*w))=*((int *)(srcp+2*((int)((factor+(1-factor)*w*2/winw)*w))));
		*((int *)(dstp-2*w))=*((int *)(srcp-2*((int)((factor+(1-factor)*w*2/winw)*w))));
		
	}
}

// PeelzHLineRGB32 requires the srcp and dstp to point to middle of source window and destination windows.
// the destw normally should be touching the srcwindow towards peel direction
//source window should be double that of destination window winw.
//The line is flipped horizontally. Shading can be done optionally
 void PeelzHLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winw,int shade)
{
	int wwd = (shade>=0?winw:0);
	for(int w=0; w<winw/2; w++)
	{

		*((int *)(dstp+4*w))=(*((int *)(srcp-4 * (w + (2 * w * w)/winw))))
				| 0x01010101*((shade*(wwd-2 * w))/winw);
		*((int *)(dstp-4*w))=(*((int *)(srcp+4 * (w + (2 * w * w)/winw)))) 
				| 0x01010101*((shade*(wwd-2 * w))/winw);
		
	
		
	}
}
// PeelzHLineYUY2 requires the srcp and dstp to point to middle of source window and destination windows.
// the destw normally should be touching the srcwindow towards peel direction
//source window should be double that of destination window winw.
//The line is flipped horizontally. Shading can be done optionally
 void PeelzHLineYUY2( const unsigned char* srcp, unsigned char* dstp, int winw,int shade)
{
	int wwd = (shade >= 0 ? winw : 0);
	for(int w = 0; w < winw/2 ; w ++)
	{
		int sh = (shade * (wwd - 2 * w) ) / winw;
		int nw = 2 * (w + (2 * w * w)/winw);
		int nww = nw + nw;
		int ww = w + w;
		
		dstp[ww ] = srcp[-nww ] | sh;
		dstp[-ww] = srcp[nww] | sh;

		if ((w & 1) == (nw & 1))
		{
			dstp[ww + 1] = srcp[-nww + 1];
			dstp[-ww + 1] = srcp[nww + 1];
		}
		else if (( w & 1 ) == 0)
		{
			dstp[ww + 1] = srcp[-nww - 1];
			dstp[-ww + 1] = srcp[nww - 1];
		}
		else
		{
			dstp[ww + 1] = srcp[-nww + 3];
			dstp[-ww + 1] = srcp[nww + 3];
		}
		
	}
}

// PeelzHLineRGB24 requires the srcp and dstp to point to middle of source window and destination windows.
// the destw normally should be touching the srcwindow towards peel direction
//source window should be double that of destination window winw.
//The line is flipped horizontally.
 void PeelzHLineRGB24( const unsigned char* srcp, unsigned char* dstp, int winw, int shade )
{
	int wwd = (shade>=0?winw:0);
	for(int w=0; w<winw/2; w++)
	{
		int sh = (shade*(wwd-2*w))/winw;
		int nww = 3 * (w + (2 * w * w)/winw);

		for(int k=0; k<3; k++)
		{
			*(dstp+3*w+k)=(*(srcp - nww+k)) | sh;
			*(dstp-3*w+k)=(*(srcp + nww+k)) | sh;
		}
	}
}
//ShadeHLineRGB24 adds shading to a line. Center gets whiter compared to edges.
// the pointers point the center of window. 
 void ShadeHLineRGB24( const unsigned char* srcp, unsigned char* dstp, int winw,int shade)
{
	int wwd = (shade>=0?winw:0);
	for(int w=0; w<winw/2; w++)
	{
		for(int k=0; k<3; k++)
		{
			*(dstp+3*w+k)=(*(srcp+3*w+k)) | (unsigned char)((shade*(wwd-2*w))/winw);
			*(dstp-3*w+k)=(*(srcp-3*w+k)) | (unsigned char)((shade*(wwd-2*w))/winw);
		}
	}
}

//ShadeHLineRGB32 adds shading to a line. Center gets whiter compared to edges.
// the pointers point the center of window. 
 void ShadeHLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winw,int shade)
{
	int wwd = (shade>=0?winw:0);
	for(int w=0; w<winw/2; w++)
	{
		
				
			*((int*)(dstp+4*w))=(*((int*)(srcp+4*w))) | 0x01010101*((shade*(wwd-2*w))/winw);
			*((int*)(dstp-4*w))=(*((int*)(srcp-4*w))) | 0x01010101*((shade*(wwd-2*w))/winw);
		
		
	}
}


//ShadeHLineYUY2 adds shading to a line. Center gets whiter compared to edges.
// the pointers point the center of window. 
 void ShadeHLineYUY2( const unsigned char* srcp, unsigned char* dstp, int winw,int shade)
{
	int wwd = (shade>=0?winw:0);
	for(int w=0; w<winw/2; w++)
	{
		
				
			*(dstp+2*w)=(*(srcp+2*w)) | ((shade*(wwd-2*w))/winw);
			*(dstp-2*w)=(*(srcp-2*w)) | ((shade*(wwd-2*w))/winw);
		
		
	}
}
//***************************************************************************************************************
//***************************************************************************************************************
//Functions for Vertical lines 
// CopyVLineRGB32 expects the srcp and dstp to be at the end point of the segment to be copied 
 void CopyVLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch)
{
	for(int h=0;h<winh;h++)
		*((int*)(dstp+dpitch*h))=*((int *)(srcp+spitch*h));
}
// CopyVLineRGB24 expects the srcp and dstp to be at the end point of the segment to be copied 
 void CopyVLineRGB24( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch)
{
	for(int h=0;h<winh;h++)
		for(int k=0;k<3;k++)
			*(dstp+dpitch*h+k)=*(srcp+spitch*h+k);
}
// CopyVLineYUY2 expects the srcp and dstp to be at the start point of the segment to be copied. Two adjacent
//pixells are copied at a time. So two vertical lines are copied
 
 void CopyVLineYUY2( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch)
{
	for(int h=0;h<winh;h++)
		*((int*)(dstp+dpitch*h))=*((int *)(srcp+spitch*h));
}
//FlipVLineRGB32 expects the srcp and dstp to be at the start point of the segment to be copied 
 void FlipVLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch)
{
	for(int h=0; h<winh; h++)
		*((int*)(dstp+dpitch*h))=*((int *)(srcp+spitch*(winh-1-h)));
}
//FlipVLineRGB24 expects the srcp and dstp to be at the start point of the segment to be copied 
 void FlipVLineRGB24( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch)
{
	for(int h=0; h<winh; h++)
		for(int k=0; k<3; k++)
			*(dstp+dpitch*h+k)=*(srcp+spitch*(winh-1-h)+k);
}
//FlipVLineYUY2 expects the srcp and dstp to be at 32 bit boundary of the start point of the segment to be copied. At a time
// two adjacent vertical lines are flipped.  
 void FlipVLineYUY2( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch)
{
	for(int h=0; h<winh; h++)
		*((int*)(dstp+dpitch*h))=*((int *)(srcp+spitch*(winh-1-h)));
}

// ResizeVLineRGB32 requires the srcp and dstp to point to middle of window winh/2. The factor is frame width/winh
 void ResizeVLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,double factor)
{
						// nearest point interpolation	
	for(int h=0; h<winh/2; h++)
	{
	
		*((int *)(dstp+dpitch*h))=*((int *)(srcp+spitch*((int)(factor*h))));
		*((int *)(dstp-dpitch*h))=*((int *)(srcp-spitch*((int)(factor*h))));
		
	}
}

// ResizeVLineRGB24 requires the srcp and dstp to point to middle of window winh/2. The factor is frame width/winh
 void ResizeVLineRGB24( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,double factor)
{
						// nearest point interpolation	
	for(int h=0; h<winh/2; h++)
	{
		for(int k=0; k<3; k++)
		{
			*(dstp+dpitch*h+k)=*(srcp+spitch*((int)(factor*h))+k);
			*(dstp-dpitch*h+k)=*(srcp-spitch*((int)(factor*h))+k);
		}
	}
}
// ResizeVLineYUY2 requires the srcp and dstp to point to middle of window winh/2. The factor is frame width/winh
 void ResizeVLineYUY2( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,double factor)
{
						// nearest point interpolation	
	for(int h=0; h<winh/2; h++)
	{
	
		*((int *)(dstp+dpitch*h))=*((int *)(srcp+spitch*((int)(factor*h))));
		*((int *)(dstp-dpitch*h))=*((int *)(srcp-spitch*((int)(factor*h))));
		
	}
}

// FlipResizeVLineRGB32 requires the srcp and dstp to point to middle of window winh/2. The factor is frame width/winh
 void FlipResizeVLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,double factor)
{
						// nearest point interpolation	
	for(int h=0; h<winh/2; h++)
	{

		*((int *)(dstp+dpitch*h))=*((int *)(srcp-spitch*((int)(factor*h))));
		*((int *)(dstp-dpitch*h))=*((int *)(srcp+spitch*((int)(factor*h))));
		
	}
}
// FlipResizeVLineRGB24 requires the srcp and dstp to point to middle of window winh/2. The factor is frame width/winh
 void FlipResizeVLineRGB24( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,double factor)
{
						// nearest point interpolation	
	for(int h=0; h<winh/2; h++)
	{
		for(int k=0; k<3; k++)
		{
			*(dstp+dpitch*h+k)=*(srcp-spitch*((int)(factor*h))+k);
			*(dstp-dpitch*h+k)=*(srcp+spitch*((int)(factor*h))+k);
		}
	}
}
// FlipResizeVLineYUY2 requires the srcp and dstp to point to middle of window winh/2. The factor is frame width/winh
 void FlipResizeVLineYUY2( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,double factor)
{
						// nearest point interpolation	
	for(int h=0; h<winh/2; h++)
	{

		*((int *)(dstp+dpitch*h))=*((int *)(srcp-spitch*((int)(factor*h))));
		*((int *)(dstp-dpitch*h))=*((int *)(srcp+spitch*((int)(factor*h))));
		
	}
}

//

// FlipLenzVLineRGB24 requires the srcp and dstp to point to middle of source window and destination windows
// The factor is source window width/destination window width winh
 void FlipLenzVLineRGB24( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,double factor)
{
	for(int h=0; h<winh/2; h++)
	{
		for(int k=0; k<3; k++)
		{
			*(dstp+dpitch*h+k)=*(srcp-spitch*((int)((factor+(1-factor)*h*2/winh)*h))+k);
			*(dstp-dpitch*h+k)=*(srcp+spitch*((int)((factor+(1-factor)*h*2/winh)*h))+k);
		}
	}
}


// LenzVLineRGB24 requires the srcp and dstp to point to middle of source window and destination windows
// The factor is source window height/destination window height winh
 void LenzVLineRGB24( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,double factor)
{
	for(int h=0; h<winh/2; h++)
	{
		for(int k=0; k<3; k++)
		{
			*(dstp+dpitch*h+k)=*(srcp+spitch*((int)((factor+(1-factor)*h*2/winh)*h))+k);
			*(dstp-dpitch*h+k)=*(srcp-spitch*((int)((factor+(1-factor)*h*2/winh)*h))+k);
		}
	}
}

// FlipLenzVLineRGB32 requires the srcp and dstp to point to middle of source window and destination windows
// The factor is source window width/destination window width winh
 void FlipLenzVLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,double factor)
{
	for(int h=0; h<winh/2; h++)
	{
		
		*((int *)(dstp+dpitch*h))=*((int *)(srcp-spitch*((int)((factor+(1-factor)*h*2/winh)*h))));
		*((int *)(dstp-dpitch*h))=*((int *)(srcp+spitch*((int)((factor+(1-factor)*h*2/winh)*h))));
		
	}
}


// LenzVLineRGB32 requires the srcp and dstp to point to middle of source window and destination windows
// The factor is source window width/destination window width winh
 void LenzVLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,double factor)
{
	for(int h=0; h<winh/2; h++)
	{
		*((int *)(dstp+dpitch*h))=*((int *)(srcp+spitch*((int)((factor+(1-factor)*h*2/winh)*h))));
		*((int *)(dstp-dpitch*h))=*((int *)(srcp-spitch*((int)((factor+(1-factor)*h*2/winh)*h))));
	}
}


// PeelzVLineRGB32 requires the srcp and dstp to point to middle of source window and destination windows.
// the destw normally should be touching the srcwindow towards peel direction
//source window should be double that of destination window winh.
//The line is flipped vertically. Shading can be done optionally  shade= Zero will have no shading
 void PeelzVLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,int shade)
{
	int wwd = (shade>=0?winh:0);
	for(int h=0; h<winh/2; h++)
	{
			*((int*)(dstp+dpitch*h))=*((int *)(srcp-spitch*((int)(h*(1+(double)(2*h/winh))))))
				| 0x01010101*((shade*(wwd-2*h))/winh);
			*((int*)(dstp-dpitch*h))=*((int *)(srcp+spitch*((int)(h*(1+(double)(2*h/winh)))))) 
				| 0x01010101*((shade*(wwd-2*h))/winh);
	}
}


// PeelzVLineRGB24 requires the srcp and dstp to point to middle of source window and destination windows.
// the destw normally should be touching the srcwindow towards peel direction
//source window should be double that of destination window winh.
//The line is flipped vertically.shading can be done. shade= Zero will have no shading
 void PeelzVLineRGB24( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch, int shade )
{
	int wwd = (shade>=0?winh:0);
	for(int h=0; h<winh/2; h++)
	{
		for(int k=0; k<3; k++)
		{
			*(dstp+dpitch*h+k)=(*(srcp-spitch*((int)(h*(1+(double)(2*h/winh))))+k))
				| (unsigned char)((shade*(wwd-2*h))/winh);
			*(dstp-dpitch*h+k)=(*(srcp+spitch*((int)(h*(1+(double)(2*h/winh))))+k)) 
				| (unsigned char)((shade*(wwd-2*h))/winh);
		}
	}
}
// PeelzVLineYUY22 requires the srcp and dstp to point to middle of source window and destination windows.
// the destw normally should be touching the srcwindow towards peel direction
//source window should be double that of destination window winh, and divisible by 4.
//The line is flipped vertically. Shading can be done optionally  shade= Zero will have no shading
 void PeelzVLineYUY2( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,int shade)
{
	int wwd = (shade>=0?winh:0);
	for(int h=0; h<winh/2; h++)
	{
			*((int*)(dstp+dpitch*h))=*((int *)(srcp-spitch*((int)(h*(1+(double)(2.0*h/winh))))))
				| 0x01000100*((shade*(wwd-2*h))/winh);
			*((int*)(dstp-dpitch*h))=*((int *)(srcp+spitch*((int)(h*(1+(double)(2.0*h/winh)))))) 
				| 0x01000100*((shade*(wwd-2*h))/winh);
	}
}
//ShadeVLineRGB24 adds shading to a line. Center gets whiter compared to edges.
// the pointers point the center of window. 
 void ShadeVLineRGB24( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,int shade)
{
	int wwd = (shade>=0?winh:0);
	for(int h=0; h<winh/2; h++)
	{
		for(int k=0; k<3; k++)
		{
			*(dstp+dpitch*h+k)=(*(srcp+spitch*h+k)) | (unsigned char)((shade*(wwd-2*h))/winh);
			*(dstp-dpitch*h+k)=(*(srcp-spitch*h+k)) | (unsigned char)((shade*(wwd-2*h))/winh);
		}
	}
}


//ShadeVLineRGB32 adds shading to a line. -255 to 255 Center gets whiter compared to edges.
// the pointers point the center of window. 
 void ShadeVLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,int shade)
{
	int wwd = (shade>=0?winh:0);
	for(int h=0; h<winh/2; h++)
	{
		
		*((int*)(dstp+dpitch*h))=(*((int*)(srcp+spitch*h))) | 0x01010101*((shade*(wwd-2*h))/winh);
		*((int*)(dstp-dpitch*h))=(*((int*)(srcp-spitch*h))) | 0x01010101*((shade*(wwd-2*h))/winh);
		
	}
}
//ShadeVLineYUY2 adds shading to a line. -255 to 255 Center gets whiter compared to edges.
// the pointers point the center of window. Two lines are copied/shaded at a time 
 void ShadeVLineYUY2( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,int shade)
{
	int wwd = (shade>=0?winh:0);
	for(int h=0; h<winh/2; h++)
	{
		
		*((int*)(dstp+dpitch*h))=(*((int*)(srcp+spitch*h))) | 0x01010101*((shade*(wwd-2*h))/winh);
		*((int*)(dstp-dpitch*h))=(*((int*)(srcp-spitch*h))) | 0x01010101*((shade*(wwd-2*h))/winh);
		
	}
}

/*******************************************************************************************************************
YV12 functions


**********************************************************************************************************************/
 void CopyHLineYV12( const unsigned char* srcp, unsigned char* dstp, int winw)
{
	for(int w=0;w<winw;w++)
		*(dstp+w)=*(srcp+w);
}
 void FlipHLineYV12( const unsigned char* srcp, unsigned char* dstp, int winw)
{
	for(int w=0; w<winw; w++)
		*(dstp+w)=*(srcp+(winw-1-w));
}
// ResizeHLineYV12 requires the srcp and dstp to point to middle of window winw/2. The factor is input widow width/winw
 void ResizeHLineYV12( const unsigned char* srcp, unsigned char* dstp, int winw,double factor)
{
						// nearest point interpolation	
	for(int w=0; w<winw/2; w++)
	{
		
		
		*(dstp+w)=*(srcp+(int)(factor*w));
		*(dstp-w)=*(srcp-(int)(factor*w));
		
	}
}
// FlipResizeHLineYV12 requires the srcp and dstp to point to middle of window winw/2. The factor is frame width/winw
 void FlipResizeHLineYV12( const unsigned char* srcp, unsigned char* dstp, int winw,double factor)
{
						// nearest point interpolation	
	for(int w=0; w<winw/2; w++)
	{
		*(dstp+w)=*(srcp-(int)(factor*w));
		*(dstp-w)=*(srcp+(int)(factor*w));
	}
}
// FlipLenzHLineYV12 requires the srcp and dstp to point to middle of source window and destination windows
// The factor is source window width/destination window width winw
 void FlipLenzHLineYV12( const unsigned char* srcp, unsigned char* dstp, int winw,double factor)
{
	for(int w=0; w<winw/2; w++)
	{
		
			*(dstp+w)=*(srcp-((int)((factor+(1-factor)*w*2/winw)*w)));
			*(dstp-w)=*(srcp+((int)((factor+(1-factor)*w*2/winw)*w)));
		
	}
}
// LenzHLineYV12 requires the srcp and dstp to point to middle of source window and destination windows
// The factor is source window width/destination window width winw
 void LenzHLineYV12( const unsigned char* srcp, unsigned char* dstp, int winw,double factor)
{
	for(int w=0; w<winw/2; w++)
	{
		
			*(dstp+w)=*(srcp+((int)((factor+(1-factor)*w*2/winw)*w)));
			*(dstp-w)=*(srcp-((int)((factor+(1-factor)*w*2/winw)*w)));
		
	}
}
// PeelzHLineYV12 requires the srcp and dstp to point to middle of source window and destination windows.
// the destw normally should be touching the srcwindow towards peel direction
//source window should be double that of destination window winw.
//The line is flipped horizontally. Shading can be done optionally
 void PeelzHLineYV12( const unsigned char* srcp, unsigned char* dstp, int winw,int shade)
{
	int wwd = (shade>=0?winw:0);
	for(int w=0; w<winw/2; w++)
	{
		
		*(dstp+w)=*(srcp-((int)(w*(1+(double)(2*w/winw)))))
				| 0x01010101*((shade*(wwd-2*w))/winw);
		*(dstp-w)=*(srcp+((int)(w*(1+(double)(2*w/winw))))) 
				| 0x01010101*((shade*(wwd-2*w))/winw);
		
	}
}
//ShadeHLineYV12 adds shading to a line. Center gets whiter compared to edges.
// the pointers point the center of window.
 void ShadeHLineYV12( const unsigned char* srcp, unsigned char* dstp, int winw,int shade)
{
	int wwd = (shade>=0?winw:0);
	for(int w=0; w<winw/2; w++)
	{
		
			*(dstp+w)=(*(srcp+w)) | (unsigned char)((shade*(wwd-2*w))/winw);
			*(dstp-w)=(*(srcp-w)) | (unsigned char)((shade*(wwd-2*w))/winw);
		
	}
}
//***************************************************************************************************************
//Functions for Vertical lines 
// CopyVLineYV12 expects the srcp and dstp to be at the end point of the segment to be copied. 
 void CopyVLineYV12( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch)
{
	for(int h=0;h<winh;h++)
		*(dstp+dpitch*h)=*(srcp+spitch*h);
}
//FlipVLineYV12 expects the srcp and dstp to be at the start point of the segment to be copied 
 void FlipVLineYV12( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch)
{
	for(int h=0; h<winh; h++)
		*(dstp+dpitch*h)=*(srcp+spitch*(winh-1-h));
}
// ResizeVLineYV12 requires the srcp and dstp to point to middle of window winh/2. The factor is frame width/winh
 void ResizeVLineYV12( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,double factor)
{
						// nearest point interpolation	
	for(int h=0; h<winh/2; h++)
	{
	
		*(dstp+dpitch*h)=*(srcp+spitch*((int)(factor*h)));
		*(dstp-dpitch*h)=*(srcp-spitch*((int)(factor*h)));
		
	}
}
// FlipResizeVLineYV12 requires the srcp and dstp to point to middle of window winh/2. The factor is frame width/winh
 void FlipResizeVLineYV12( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,double factor)
{
						// nearest point interpolation	
	for(int h=0; h<winh/2; h++)
	{
	
		*(dstp+dpitch*h)=*(srcp-spitch*((int)(factor*h)));
		*(dstp-dpitch*h)=*(srcp+spitch*((int)(factor*h)));
		
	}
}
// FlipLenzVLineYV12 requires the srcp and dstp to point to middle of source window and destination windows
// The factor is source window width/destination window width winh
 void FlipLenzVLineYV12( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,double factor)
{
	for(int h=0; h<winh/2; h++)
	{
		
		*(dstp+dpitch*h)=*(srcp-spitch*((int)((factor+(1-factor)*h*2/winh)*h)));
		*(dstp-dpitch*h)=*(srcp+spitch*((int)((factor+(1-factor)*h*2/winh)*h)));
		
	}
}
// LenzVLineYV12 requires the srcp and dstp to point to middle of source window and destination windows
// The factor is source window width/destination window width winh
 void LenzVLineYV12( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,double factor)
{
	for(int h=0; h<winh/2; h++)
	{
		
		*(dstp+dpitch*h)=*(srcp+spitch*((int)((factor+(1-factor)*h*2/winh)*h)));
		*(dstp-dpitch*h)=*(srcp-spitch*((int)((factor+(1-factor)*h*2/winh)*h)));
		
	}
}
// PeelzVLineYV12 requires the srcp and dstp to point to middle of source window and destination windows.
// the destw normally should be touching the srcwindow towards peel direction
//source window should be double that of destination window winh.
//The line is flipped vertically. Shading can be done optionally  shade= Zero will have no shading
 void PeelzVLineYV12( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,int shade)
{
	int wwd = (shade>=0?winh:0);
	for(int h=0; h<winh/2; h++)
	{
			*(dstp+dpitch*h)=*(srcp-spitch*((int)(h*(1+(double)(2*h/winh)))))
				| 0x01010101*((shade*(wwd-2*h))/winh);
			*(dstp-dpitch*h)=*(srcp+spitch*((int)(h*(1+(double)(2*h/winh))))) 
				| 0x01010101*((shade*(wwd-2*h))/winh);
	}
}
//ShadeVLineYV12 adds shading to a line. Center gets whiter compared to edges.
// the pointers point the center of window. 
 void ShadeVLineYV12( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,int shade)
{
	int wwd = (shade>=0?winh:0);
	for(int h=0; h<winh/2; h++)
	{
		
			*(dstp+dpitch*h)=(*(srcp+spitch*h)) | (unsigned char)((shade*(wwd-2*h))/winh);
			*(dstp-dpitch*h)=(*(srcp-spitch*h)) | (unsigned char)((shade*(wwd-2*h))/winh);
		
	}
}

//----------------------------------------------------------------------------------------------------------------
/*******************************************************************************************************************
Planar functions


**********************************************************************************************************************/
 void CopyHLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winw)
{
	for(int w=0;w<winw;w++)
		*(dstp+w)=*(srcp+w);
}
 void FlipHLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winw)
{
	for(int w=0; w<winw; w++)
		*(dstp+w)=*(srcp+(winw-1-w));
}
// ResizeHLinePlanar requires the srcp and dstp to point to middle of window winw/2. The factor is input widow width/winw
 void ResizeHLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winw,double factor)
{
						// nearest point interpolation	
	for(int w=0; w<winw/2; w++)
	{
		
		
		*(dstp+w)=*(srcp+(int)(factor*w));
		*(dstp-w)=*(srcp-(int)(factor*w));
		
	}
}
// FlipResizeHLinePlanar requires the srcp and dstp to point to middle of window winw/2. The factor is frame width/winw
 void FlipResizeHLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winw,double factor)
{
						// nearest point interpolation	
	for(int w=0; w<winw/2; w++)
	{
		*(dstp+w)=*(srcp-(int)(factor*w));
		*(dstp-w)=*(srcp+(int)(factor*w));
	}
}
// FlipLenzHLinePlanar requires the srcp and dstp to point to middle of source window and destination windows
// The factor is source window width/destination window width winw
 void FlipLenzHLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winw,double factor)
{
	for(int w=0; w<winw/2; w++)
	{
		
			*(dstp+w)=*(srcp-((int)((factor+(1-factor)*w*2/winw)*w)));
			*(dstp-w)=*(srcp+((int)((factor+(1-factor)*w*2/winw)*w)));
		
	}
}
// LenzHLinePlanar requires the srcp and dstp to point to middle of source window and destination windows
// The factor is source window width/destination window width winw
 void LenzHLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winw,double factor)
{
	for(int w=0; w<winw/2; w++)
	{
		
			*(dstp+w)=*(srcp+((int)((factor+(1-factor)*w*2/winw)*w)));
			*(dstp-w)=*(srcp-((int)((factor+(1-factor)*w*2/winw)*w)));
		
	}
}
// PeelzHLinePlanar requires the srcp and dstp to point to middle of source window and destination windows.
// the destw normally should be touching the srcwindow towards peel direction
//source window should be double that of destination window winw.
//The line is flipped horizontally. Shading can be done optionally
 void PeelzHLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winw,int shade)
{
	int wwd = (shade>=0?winw:0);
	for(int w=0; w<winw/2; w++)
	{
		
		*(dstp+w)=*(srcp-((int)(w*(1+(double)(2*w/winw)))))
				| 0x01010101*((shade*(wwd-2*w))/winw);
		*(dstp-w)=*(srcp+((int)(w*(1+(double)(2*w/winw))))) 
				| 0x01010101*((shade*(wwd-2*w))/winw);
		
	}
}
//ShadeHLinePlanar adds shading to a line. Center gets whiter compared to edges.
// the pointers point the center of window.
 void ShadeHLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winw,int shade)
{
	int wwd = (shade>=0?winw:0);
	for(int w=0; w<winw/2; w++)
	{
		
			*(dstp+w)=(*(srcp+w)) | (unsigned char)((shade*(wwd-2*w))/winw);
			*(dstp-w)=(*(srcp-w)) | (unsigned char)((shade*(wwd-2*w))/winw);
		
	}
}
//***************************************************************************************************************
//Functions for Vertical lines 
// CopyVLinePlanar expects the srcp and dstp to be at the end point of the segment to be copied. 
 void CopyVLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch)
{
	for(int h=0;h<winh;h++)
		*(dstp+dpitch*h)=*(srcp+spitch*h);
}
//FlipVLinePlanar expects the srcp and dstp to be at the start point of the segment to be copied 
 void FlipVLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch)
{
	for(int h=0; h<winh; h++)
		*(dstp+dpitch*h)=*(srcp+spitch*(winh-1-h));
}
// ResizeVLinePlanar requires the srcp and dstp to point to middle of window winh/2. The factor is frame width/winh
 void ResizeVLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,double factor)
{
						// nearest point interpolation	
	for(int h=0; h<winh/2; h++)
	{
	
		*(dstp+dpitch*h)=*(srcp+spitch*((int)(factor*h)));
		*(dstp-dpitch*h)=*(srcp-spitch*((int)(factor*h)));
		
	}
}
// FlipResizeVLinePlanar requires the srcp and dstp to point to middle of window winh/2. The factor is frame width/winh
 void FlipResizeVLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,double factor)
{
						// nearest point interpolation	
	for(int h=0; h<winh/2; h++)
	{
	
		*(dstp+dpitch*h)=*(srcp-spitch*((int)(factor*h)));
		*(dstp-dpitch*h)=*(srcp+spitch*((int)(factor*h)));
		
	}
}
// FlipLenzVLinePlanar requires the srcp and dstp to point to middle of source window and destination windows
// The factor is source window width/destination window width winh
 void FlipLenzVLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,double factor)
{
	for(int h=0; h<winh/2; h++)
	{
		
		*(dstp+dpitch*h)=*(srcp-spitch*((int)((factor+(1-factor)*h*2/winh)*h)));
		*(dstp-dpitch*h)=*(srcp+spitch*((int)((factor+(1-factor)*h*2/winh)*h)));
		
	}
}
// LenzVLinePlanar requires the srcp and dstp to point to middle of source window and destination windows
// The factor is source window width/destination window width winh
 void LenzVLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,double factor)
{
	for(int h=0; h<winh/2; h++)
	{
		
		*(dstp+dpitch*h)=*(srcp+spitch*((int)((factor+(1-factor)*h*2/winh)*h)));
		*(dstp-dpitch*h)=*(srcp-spitch*((int)((factor+(1-factor)*h*2/winh)*h)));
		
	}
}
// PeelzVLinePlanar requires the srcp and dstp to point to middle of source window and destination windows.
// the destw normally should be touching the srcwindow towards peel direction
//source window should be double that of destination window winh.
//The line is flipped vertically. Shading can be done optionally  shade= Zero will have no shading
 void PeelzVLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,int shade)
{
	int wwd = (shade>=0?winh:0);
	for(int h=0; h<winh/2; h++)
	{
			*(dstp+dpitch*h)=*(srcp-spitch*((int)(h*(1+(double)(2*h/winh)))))
				| 0x01010101*((shade*(wwd-2*h))/winh);
			*(dstp-dpitch*h)=*(srcp+spitch*((int)(h*(1+(double)(2*h/winh))))) 
				| 0x01010101*((shade*(wwd-2*h))/winh);
	}
}
//ShadeVLinePlanar adds shading to a line. Center gets whiter compared to edges.
// the pointers point the center of window. 
 void ShadeVLinePlanar( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch,int shade)
{
	int wwd = (shade>=0?winh:0);
	for(int h=0; h<winh/2; h++)
	{
		
			*(dstp+dpitch*h)=(*(srcp+spitch*h)) | (unsigned char)((shade*(wwd-2*h))/winh);
			*(dstp-dpitch*h)=(*(srcp-spitch*h)) | (unsigned char)((shade*(wwd-2*h))/winh);
		
	}
}
#endif