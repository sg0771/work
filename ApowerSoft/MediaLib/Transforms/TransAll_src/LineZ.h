/*********************************************************************************************
LineZ contain a set of functions that operate on a single line (vertical or Horizontal)
These are 1 Copy, 2 Flip and magnify linear or as a lens or constant value; Shading
can also be applied. These functions do not check the parameters and assume them to be correct.
Input and out pointers to the middle(except for copy and plain flip functions)
 of window, magnification, window winh( height). pitch for vertical functions  flip, shade are 
some of the parameters needed. 
**********************************************************************************************/
#ifndef Linez_H_V_C_MOHAN
#define Linez_H_V_C_MOHAN
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
// Definitions of Horizontal line functions
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
//**************************************************************************************************
//YUV2 Format Functions
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

  //***********************************************************************************
// Vertical Functions:-
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
//**************************************************************************************************
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
//------------------------------------------------------------------------------------
//**************************************************************************************************
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
#endif
