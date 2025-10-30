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
//Function definitions
#ifndef FastORing_H_V_C_MOHAN
#define FastORing_H_V_C_MOHAN
bool FastORingRGB32(const unsigned char* srcp,  unsigned char* dstp, int radius,int thick, int srcx, int srcy, int srch, int srcw,
		int dstx, int dsty, int dsth, int dstw, int spitch, int dpitch);
bool FastORingRGB24(const unsigned char* srcp,  unsigned char* dstp, int radius,int thick, int srcx, int srcy, int srch, int srcw,
		int dstx, int dsty, int dsth, int dstw, int spitch, int dpitch);
bool FastORingYUY2(const unsigned char* srcp,  unsigned char* dstp, int radius,int thick, int srcx, int srcy, int srch, int srcw,
		int dstx, int dsty, int dsth, int dstw, int spitch, int dpitch);
bool FastORingYV12(const unsigned char* srcp,  unsigned char* dstp, int radius,int thick, int srcx, int srcy, int srch, int srcw,
		int dstx, int dsty, int dsth, int dstw, int spitch, int dpitch);
bool FastORingPlane(const unsigned char* srcp, int spitch,  unsigned char* dstp, int dpitch, 
					int radius,int thick, int srcx, int srcy, int srch, int srcw,
					int dstx, int dsty);

bool FastORingUVPlanes(const unsigned char* srcpU,const unsigned char* srcpV,int spitchUV,
					   unsigned char* dstpU,  unsigned char* dstpV,int dpitchUV, 						 
						int subH, int subW, // subsampling of height and width
						// below give all values for Y plane
						int radius,int thick, int srch, int srcw,
						int srcx, int srcy, 
						int dstx, int dsty);
bool FastORingRotateRGB32(const unsigned char *srcp,unsigned char*dstp,
					  int radius,int thick, int srcx,int srcy,int srch,int srcw,
					  int dstx,int dsty,int dsth,int dstw,int spitch, int dpitch,int degree);
bool FastORingRotateRGB24(const unsigned char *srcp,unsigned char*dstp,
					  int radius,int thick,int srcx,int srcy,int srch,int srcw,
					  int dstx,int dsty,int dsth,int dstw,int spitch, int dpitch,int degree);

bool FastORingRotateYUY2(const unsigned char *srcp,unsigned char*dstp,
					  int radius,int thick, int srcx,int srcy,int srch,int srcw,
					  int dstx,int dsty,int dsth,int dstw,int spitch, int dpitch,int degree);
bool FastORingRotateYV12(const unsigned char *srcp,unsigned char*dstp,
					  int radius,int thick, int srcx,int srcy,int srch,int srcw,
					  int dstx,int dsty,int dsth,int dstw,int spitch, int dpitch,int degree);
// rotates a given circular disc area in the srcp plane of frame through degree angle in anti
	// clockwise direction on to dstp plane of dst frame.

bool FastORingRotatePlane(const unsigned char *srcp,int spitch,unsigned char*dstp,int dpitch,
					  int radius,int thick,int srcx,int srcy,int srch,int srcw,
					  int dstx,int dsty,int degree);
// rotates a given circular disc area in the srcp U and V planes of frame through degree angle in anti
	// clockwise direction on to dstp U and V planes of dst frame.

bool FastORingRotateUVPlanes(const unsigned char *srcpU,const unsigned char *srcpV,int spitch,
							 unsigned char*dstpU, unsigned char*dstpV, int dpitch,
							 int subH, int subW,	// subsampling
							 // all following values are for Y plane
							int radius,int thick,int srcx,int srcy,int srch,int srcw,
							int dstx,int dsty,int degree);
//	all planes of planar formats
bool FastORingRotateYUVPlanes(const unsigned char *srcp,int spitch, unsigned char *dstp,int dpitch,
							  const unsigned char *srcpU,const unsigned char *srcpV,int spitchUV,
							 unsigned char*dstpU, unsigned char*dstpV, int dpitchUV,
							 int subH, int subW,	// subsampling
							 // all following values are for Y plane
							int radius,int thick,int srcx,int srcy,int srch,int srcw,
							int dstx,int dsty,int degree);
	

#endif