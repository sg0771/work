/***************************************************************************************************
This set of FastResizer functions resizes a source window to the size of the destination window. Nearest point 
method is used for fast work.YUY2 RGB 24 and 
RGB32  formats are supported. The src and dst  buffers must be of adequate sizes. The buffer pointers
point to start of image (left top). The image can also be flipped
***************************************************************************************************/
#ifndef FastResizer_H_V_C_MOHAN
#define FastResizer_H_V_C_MOHAN
bool HFastResizerRGB32(const unsigned char* sptr,unsigned char* dptr,int sht, int swd,int dht, int dwd,
					 int spitch, int dpitch,  bool hflip=false);

bool VFastResizerRGB32(const unsigned char* srcptr,unsigned char* dstptr,int srcht, int srcwd,int dstht, int dstwd,
					 int spitch, int dpitch,  bool vflip=false);
bool HFastResizerRGB24(const unsigned char* sptr,unsigned char* dptr,int sht, int swd,int dht, int dwd,
					 int spitch, int dpitch,  bool hflip=false);

bool VFastResizerRGB24(const unsigned char* srcptr,unsigned char* dstptr,int srcht, int srcwd,int dstht, int dstwd,
					 int spitch, int dpitch,  bool vflip=false);

bool HFastResizerYUY2(const unsigned char* sptr,unsigned char* dptr,int sht, int swd,int dht, int dwd,
					 int spitch, int dpitch,  bool hflip=false);

bool VFastResizerYUY2(const unsigned char* srcptr,unsigned char* dstptr,int srcht, int srcwd,int dstht, int dstwd,
					 int spitch, int dpitch,  bool vflip=false);
bool HFastResizerYV12(const unsigned char* sptr,unsigned char* dptr,int sht, int swd,int dht, int dwd,
					 int spitch, int dpitch,  bool hflip=false);

bool VFastResizerYV12(const unsigned char* srcptr,unsigned char* dstptr,int srcht, int srcwd,int dstht, int dstwd,
					 int spitch, int dpitch,  bool vflip=false);


bool VFastResizerPlanar(const unsigned char* srcptr, unsigned char* dstptr, int srcht, int srcwd, int dstht, int dstwd,
	int spitch, int dpitch, bool vflip);
bool HFastResizerPlanar(const unsigned char* srcptr, unsigned char* dstptr, int srcht, int srcwd, int dstht, int dstwd,
	int spitch, int dpitch, bool horflip);

#endif
/**************************************************************************************************************/