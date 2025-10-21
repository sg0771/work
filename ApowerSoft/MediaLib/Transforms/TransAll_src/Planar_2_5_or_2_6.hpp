// include this in the class methods deinitions to 
// use IsY8(), GetPlaneWidthSubsampling(plane)
// and GetPlaneHeightSubsampling(plane) in 2.5 and 2.6 versions
// without raising compile errors. 

#define _WIN32 1
#ifdef _WIN32
	bool IsY8(){return vi.IsY8();}
	int GetPlaneWidthSubsampling(int pl){
		if(pl == 0 || vi.IsRGB())
			return 0;
		return  vi.IsYUY2() ? 1 : vi.GetPlaneWidthSubsampling(pl);}
	int GetPlaneHeightSubsampling(int pl){
		if ( pl == 0 || ! vi.IsPlanar())
			return 0;
		return vi.GetPlaneHeightSubsampling(pl); }
#else 

	bool IsY8(){return false; }
	int GetPlaneWidthSubsampling(int pl){
		if(pl == 0 || vi.IsRGB())
			return 0;
		else return 1;}
	int GetPlaneHeightSubsampling(int pl){
		if ( pl == 0 || ! vi.IsPlanar())
			return 0;
		return  vi.IsPlanar() ? 1 : 0;  }
		
#endif

	
