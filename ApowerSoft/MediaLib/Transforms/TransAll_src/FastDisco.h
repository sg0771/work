/**************************************************************************
FastDisco copies a circular window from source frame to destination frame.
frame pointers, the x,y coordinates of the center of disc in each of
the frames the radius, frame heights and frame widths, pitch sizes and 
 magnification(mag increase from 1 at edge to mag at center, drop magnification
 (for RGB32 only mag increases from 1 at edge to mag at center if mag is positive, else decreases to 1 from mag
 proprtional to dist from center). Mag if between -1 & 1 is treated as unity
 ringt is thickness if ring. default full disc process. ringt must limited to radius
***************************************************************************/
//Function definitions
#ifndef FastDisco_h_V_C_MOHAN
#define FastDisco_h_V_C_MOHAN
bool FastDiscoRGB32(const unsigned char* srcp,  unsigned char* dstp, int radius, int srcx, int srcy, int srch, int srcw,
		int dstx, int dsty, int dsth, int dstw, int spitch, int dpitch, double mag=1, bool drop=true, int ringt=0);
bool FastDiscoRGB24(const unsigned char* srcp,  unsigned char* dstp, int radius, int srcx, int srcy, int srch, int srcw,
		int dstx, int dsty, int dsth, int dstw, int spitch, int dpitch, double mag=1, bool drop=true, int ringt=0);
bool FastDiscoYUY2(const unsigned char* srcp,  unsigned char* dstp, int radius, int srcx, int srcy, int srch, int srcw,
		int dstx, int dsty, int dsth, int dstw, int spitch, int dpitch, double mag=1, bool drop=true, int ringt=0);
bool FastDiscoYV12(const unsigned char* srcp,  unsigned char* dstp, int radius, int srcx, int srcy, int srch, int srcw,
		int dstx, int dsty, int dsth, int dstw, int spitch, int dpitch, double mag=1, bool drop=true, int ringt=0);
bool FastDiscoPlane(const unsigned char* srcp,  unsigned char* dstp, int radius, int srcx, int srcy, int srch, int srcw,
		int dstx, int dsty, int dsth, int dstw, int spitch, int dpitch, double mag=1, bool drop=true, int ringt=0);
bool FastDiscoPlaneXY(	// values for plane to be rotated
					  const unsigned char* srcp,  unsigned char* dstp,int spitch, int dpitch,
					  // radius and center coord on source and destination  y planes
					 int radius, int srcx, int srcy,int dstx,int dsty, 
					 int srch,int srcw,	// y plane height, width
					 	// all y plane values
					 // subsampling of h and w in U and V planes 
					const int subH, const int subW, 
					// magnification, drop effect and ring thickness on Y plane
					double mag, bool drop, int ringt);

bool FastDiscoYUVPlanes(const unsigned char* srcp,  unsigned char* dstp, 
						int radius, int srcx, int srcy, const int srch, const int srcw,
						int dstx, int dsty, const int dsth, const int dstw, const int spitch, const int dpitch,
						const unsigned char* srcpU,const unsigned char* srcpV,const int spitchUV,  
						unsigned char* dstpU,  unsigned char* dstpV,const int dpitchUV,
						const int subH, const int subW,
						double mag =1.0 , bool drop = true,int ringt = 0  );


#endif