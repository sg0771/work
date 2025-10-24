/**************************************************************************
FastFastDiscoRotate copies a circular window from source frame to destination frame
after rotating anticlockwise through an angle degree.
frame pointers, the x,y coordinates of the center of disc in each of
the frames the radius, frame heights and frame widths, pitch sizes and 
the degree
***************************************************************************/
//Function definitions
#ifndef FastDiscoRotate_H_V_C_MOHAN
#define FastDiscoRotate_H_V_C_MOHAN
bool FastDiscoRotateRGB32(const unsigned char *srcp,unsigned char*dstp,
					  int radius,int srcx,int srcy,int srch,int srcw,
					  int dstx,int dsty,int dsth,int dstw,int spitch, int dpitch,int degree);
bool FastDiscoRotateRGB24(const unsigned char *srcp,unsigned char*dstp,
					  int radius,int srcx,int srcy,int srch,int srcw,
					  int dstx,int dsty,int dsth,int dstw,int spitch, int dpitch,int degree);
bool FastDiscoRotateYUY2(const unsigned char *srcp,unsigned char*dstp,
					  int radius,int srcx,int srcy,int srch,int srcw,
					  int dstx,int dsty,int dsth,int dstw,int spitch, int dpitch,int degree);
bool FastDiscoRotateYV12(const unsigned char *srcp,unsigned char*dstp,
					  int radius,int srcx,int srcy,int srch,int srcw,
					  int dstx,int dsty,int dsth,int dstw,int spitch, int dpitch,int degree);

bool FastDiscoRotatePlane(const unsigned char *srcp,unsigned char*dstp,
					  int radius,int srcx,int srcy,int srch,int srcw,
					  int dstx,int dsty,int dsth,int dstw,int spitch, int dpitch,int degree);

bool FastDiscoRotateYUVPlanes(const unsigned char *srcp,unsigned char*dstp,
					  int radius,int srcx,int srcy,const int srch,const int srcw,
					  int dstx,int dsty,const int dsth,const int dstw,
					  const int spitch, const int dpitch,int degree,
					  const unsigned char *srcpU,const unsigned char *srcpV,const int spitchUV, 
					  unsigned char*dstpU, unsigned char*dstpV, const int dpitchUV,
					  const int subH, const int subW);
#endif