#include "ffms.h"
#include "avssources.h"
#include "./utils.h"
#include "Utils.hpp"
#include "avs_vf_ffmpeg.h"

static AVSValue __cdecl CreateFFIndex(AVSValue Args, void* UserData, IScriptEnvironment* Env) {
	if (!Args[0].Defined())
		Env->ThrowError("FFIndex: No source specified");

	const char* Source = Args[0].AsString();
	const char* CacheFile = Args[1].AsString("");
	int IndexMask = Args[2].AsInt(-1);
	int ErrorHandling = Args[3].AsInt(FFMS_IEH_IGNORE);
	bool OverWrite = Args[4].AsBool(false);

	std::string DefaultCache(Source);
	DefaultCache.append(".ffindex");
	if (!strcmp(CacheFile, ""))
		CacheFile = DefaultCache.c_str();

	ErrorInfo E;
	FFMS_Index* Index = FFMS_ReadIndex(CacheFile, &E); 
	if (OverWrite || !Index || (Index && FFMS_IndexBelongsToFile(Index, Source, 0) != FFMS_ERROR_SUCCESS)) {

		FFMS_Indexer* Indexer = FFMS_CreateIndexer(Source, &E);
		if (!Indexer)
			Env->ThrowError("FFIndex: %s", E.Buffer);

		// Treat -1 as meaning track numbers above sizeof(int) too
		if (IndexMask == -1)
			FFMS_TrackTypeIndexSettings(Indexer, FFMS_TYPE_AUDIO, 1); //�Ƿ�������Ƶ����

		// Apply attributes to remaining tracks (will set the attributes again on some tracks)
		for (int i = 0; i < sizeof(IndexMask) * 8; i++) {
			if ((IndexMask >> i) & 1)
				FFMS_TrackIndexSettings(Indexer, i, 1);
		}

		Index = FFMS_DoIndexing(Indexer, ErrorHandling, &E);
		if (!Index) { 
			Env->ThrowError("FFMS_DoIndexing Error: %s", E.Buffer);
		}

		if (FFMS_WriteIndex(CacheFile, Index, &E)) { // 
			FFMS_DestroyIndex(Index);
			Env->ThrowError("FFMS_WriteIndex Error: %s", E.Buffer);
		}

		FFMS_DestroyIndex(Index);//CreateFFIndex()
		if (!OverWrite)
			return AVSValue(1);
		else
			return AVSValue(2);
	}
	else {
		FFMS_DestroyIndex(Index); //CreateFFIndex()
		return AVSValue(0);
	}
}

EXTERN_C char* getmd5str(char* path);


struct FFMS_IndexItem {
	std::wstring m_path = L"";
	FFMS_Index* index = nullptr;
	int Track = 0;
	FFMS_IndexItem(std::wstring _path, FFMS_Index* _index, int _track) {
		m_path = _path;
		index = _index;
		Track = _track;
	}
	static std::list<FFMS_IndexItem>& GetList(){
		static std::list<FFMS_IndexItem> s_list_FFMS_Indexs; //���������б����Ѿ��򿪹���Index 
		return s_list_FFMS_Indexs;
	}
};

FFMS_API(FFMS_Index*)FFMS_ProcessIndex(const char* Source, int* Track){
	IScriptEnvironment* Env = (IScriptEnvironment*)FFMS_GetEnv();
	std::wstring wstr = WXBase::UTF8ToUTF16(Source);
	if (!WXBase::Exists(wstr)) {
		WXLogW(L"%ws %ws No Exist", __FUNCTIONW__, wstr.c_str());
		return nullptr; //��ý���ļ�������
	}

	//���������б����Ѿ��򿪹���Index
	FFMS_IndexItem* resultitem = NULL;
	for each (FFMS_IndexItem  item in FFMS_IndexItem::GetList()){
		if (wcsicmp(item.m_path.c_str(), wstr.c_str()) == 0) {
			*Track = item.Track;
			//WXLogW(L"%ws %ws Exist !!!!!!!!!!!!", __FUNCTIONW__, wstr.c_str());
			return item.index;
		}
	}

	ErrorInfo E;
	FFMS_Index* pIndex = nullptr;
	std::string CacheFile;
	char* base64str = getmd5str((char*)Source);
	CacheFile = Env->GetTimelineInfo()->m_IndexDir;//获取Index目录
	if (strlen(base64str) > 50) {
		base64str += (strlen(base64str) - 50);
	}
	CacheFile += std::string(base64str);
	CacheFile += std::string(".index");
	if (WXBase::Exists(CacheFile)) {
		std::wstring wsstrCache = WXBase::UTF8ToUTF16(CacheFile);
		utils::ML_FileBuffer myIndexData(wsstrCache);
		if (myIndexData.size() > 0) {
			pIndex = FFMS_ReadIndexFromBuffer(myIndexData.ptr<uint8_t>(), myIndexData.size(), &E);
			if (pIndex && FFMS_IndexBelongsToFile(pIndex, Source, 0) != FFMS_ERROR_SUCCESS) {
				//��ȡ�ɹ�����Ϣ��һ��
				WXLogW(L"++++ Read Index OK, But is not belong to file %ws", wstr.c_str());
				FFMS_DestroyIndex(pIndex);
				pIndex = nullptr;
			}
		}
	}

	if (pIndex == nullptr) {
		//�򿪶�ý���ļ�������Index
		//WXLogW(L"%ws Get Index from Mediafile", wstr.c_str());
		FFMS_Indexer* Indexer = FFMS_CreateIndexer(Source, &E);
		if (!Indexer){
			//WXLogA("FFMS_CreateIndexer: Error");
			return NULL;
		}/*else {
			WXLogA("FFMS_CreateIndexer: OK");
		}*/
		FFMS_TrackTypeIndexSettings(Indexer, FFMS_TYPE_AUDIO, 1);//�Ƿ�������Ƶ����
		pIndex = FFMS_DoIndexing(Indexer, FFMS_IEH_CLEAR_TRACK, &E);//��Indexer����Index

		if (pIndex == nullptr) { //����ʧ��
			WXLogA("FFMS_DoIndexing2: Error %s", E.Buffer);
			return NULL;
		}

		if (FFMS_WriteIndex(CacheFile.c_str(), pIndex, &E)) { //��Indexд�뵽�����ļ�
			FFMS_DestroyIndex(pIndex);
			WXLogA("FFMS_WriteIndex: Error %s", E.Buffer);
			return NULL;
		}
		//��������
		FFMS_DestroyIndex(pIndex);
		pIndex = nullptr;//
		pIndex = FFMS_ReadIndex(CacheFile.c_str(), &E);//
	}
	//else {
	//	WXLogW(L"%ws Get Index For CacheFile",wstr.c_str());
	//}

	*Track = FFMS_GetFirstIndexedTrackOfType(pIndex, FFMS_TYPE_VIDEO, NULL); //VideoTrack
	FFMS_IndexItem::GetList().emplace_back(wstr, pIndex, *Track);
	//WXLogA("FFMS_GetFirstIndexedTrackOfType: %d", *Track);
	return pIndex;
}

AVSValue __cdecl CreateFFVideoSource(AVSValue Args, void* UserData, IScriptEnvironment* Env) {

	if (!Args[0].Defined())
		Env->ThrowError("FFVideoSource: No source specified");

	const char* Source = _strdup(utils::StringExt::base64_s2s(Args[0].AsString()).c_str());
	int ID = Args[1].AsInt(0);//

	//Args[0].AsString();
	int Track = -1;
	bool Cache = true;
	const char* CacheFile_nouse = "";
	int FPSNum = -1;
	int FPSDen = 1;
	int Threads = -1;
	const char* Timecodes = "";
	int SeekMode = 1;
	int RFFMode = 0;
	int Width = 0;
	int Height = 0;
	const char* Resizer = "BICUBIC";
	const char* ColorSpace = "";
	const char* VarPrefix = "";

	std::string source1 = utils::StringExt::base64_s2s(Source);

	FFMS_Index* pIndex = FFMS_ProcessIndex(source1.c_str(), &Track);
	if(pIndex == nullptr)
		Env->ThrowError("FFMS_ProcessIndex Error");

	AvisynthVideoSource* Filter = nullptr;

	try {
		Filter = new AvisynthVideoSource(ID, _strdup(source1.c_str()), Track, pIndex, FPSNum, FPSDen, Threads, SeekMode, RFFMode, Width, Height, Resizer, ColorSpace, VarPrefix, Env);
	}
	catch (...) {
		FFMS_DestroyIndex(pIndex);
		throw;
	}

	//MediaInfomation* info = FFMS_MediaInformation(pIndex);
	//if (info->interlaced){
	//	VideoInfo vi = Filter->GetVideoInfo();
	//	AVSValue vals[1] = { Filter };
	//	return Env->Invoke("bob", AVSValue(vals, 1));
	//}
	return Filter;
}

AVSValue __cdecl CreateFFVideoSource1(AVSValue Args, void* UserData, IScriptEnvironment* Env) {

	if (!Args[0].Defined())
		Env->ThrowError("FFVideoSource: No source specified");

	const char* Source = _strdup(utils::StringExt::base64_s2s(Args[0].AsString()).c_str());
	int ID = Args[1].AsInt(0);//

	//Args[0].AsString();
	int Track = -1;
	bool Cache = true;
	const char* CacheFile_nouse = "";
	int FPSNum = -1;
	int FPSDen = 1;
	int Threads = -1;
	const char* Timecodes = "";
	int SeekMode = 1;
	int RFFMode = 0;
	int Width = 0;
	int Height = 0;
	const char* Resizer = "BICUBIC";
	const char* ColorSpace = "";
	const char* VarPrefix = "";

	std::string source1 = utils::StringExt::base64_s2s(Source);
	FFMS_Index* Index = FFMS_ProcessIndex(source1.c_str(),&Track);

	AvisynthVideoSource* Filter = NULL;

	try {
		Filter = new AvisynthVideoSource(ID, _strdup(source1.c_str()), Track, Index, FPSNum, FPSDen, Threads, SeekMode, RFFMode, Width, Height, Resizer, ColorSpace, VarPrefix, Env);
	}
	catch (...) {
		FFMS_DestroyIndex(Index);
		throw;
	}

	return Filter;
}

static AVSValue __cdecl CreateFFAudioSource(AVSValue Args, void* UserData, IScriptEnvironment* Env) {

	//FFMS_Init(0, 0);

	if (!Args[0].Defined())
		Env->ThrowError("FFAudioSource: No source specified");

	const char* Source = _strdup(utils::StringExt::base64_s2s(Args[0].AsString()).c_str());
	//WXLogW(L"CreateFFAudioSource:%ws",  WXBase::UTF8ToUTF16( Source).c_str());
	//const char *Source = Args[0].AsString();
	//int Track = -1;// Args[1].AsInt(-1);
	bool Cache = Args[2].AsBool(true);
	const char* CacheFile_nouse = Args[3].AsString("");
	int AdjustDelay = Args[4].AsInt(-1);
	const char* VarPrefix = Args[5].AsString("");

	int value = 0;
	FFMS_Index* Index = FFMS_ProcessIndex(Source,&value);

	int Track = FFMS_GetFirstIndexedTrackOfType(Index, FFMS_TYPE_AUDIO, NULL);

	if (Track < 0)
		return NULL;

	if (AdjustDelay < -3)
		Env->ThrowError("FFAudioSource: Invalid delay adjustment specified");
	if (AdjustDelay >= FFMS_GetNumTracks(Index))
		Env->ThrowError("FFAudioSource: Invalid track to calculate delay from specified");

	AvisynthAudioSource* Filter = NULL;

	//���������Ƶ���Track����0�Ǵ���ģ�����
	try {
		Filter = new AvisynthAudioSource(Source, Track, Index, AdjustDelay, VarPrefix, Env);
	}
	catch (...) {
		WXLogA("bad audio, just ignore");

		FFMS_DestroyIndex(Index);
		return 0;
		throw;
	}
	return Filter;
}

static AVSValue __cdecl FFVideoRevert(AVSValue Args, void* UserData, IScriptEnvironment* Env) {
	AvisynthVideoSource* source = (AvisynthVideoSource*)(Args[0].AsClip().p);

	source->SupportRevert(Args[1].AsBool());
	return 0;
}

static AVSValue __cdecl CreateTinyAudio(AVSValue Args, void* UserData, IScriptEnvironment* Env)
{
	const char* Source = Args[0].AsString();
	AVSValue samplerate = Env->GetTimelineInfo()->SampleRate;
	AVSValue args0[6];
	args0[0] = Source;
	AVSValue value = Env->Invoke("FFAudioSource", AVSValue(args0, 6), NULL);

	int channels = Env->GetTimelineInfo()->channels;
	if (value.IsClip())
	{
		AVSValue t1 = Env->Invoke("ConvertAudioToFloat", value, NULL);
		t1 = Env->Invoke("Cache", t1, NULL);
		t1 = Env->Invoke("ResampleAudio", AVSValue(new AVSValue[3]{ t1, AVSValue(samplerate),1 }, 3), NULL);
		t1 = Env->Invoke("Cache", t1, NULL);
		if (channels == 1)
		{
			t1 = Env->Invoke("ConvertToMono", t1, NULL);
		}
		else
		{
			if (value.AsClip()->GetVideoInfo().AudioChannels() >= 2)
			{
				t1 = Env->Invoke("GetChannel", AVSValue(new AVSValue[3]{ t1, 1,2 }, 3), NULL);
			}
			else
			{
				t1 = Env->Invoke("MonoToStereo", AVSValue(new AVSValue[2]{ t1, t1 }, 2), NULL);
			}
		}
		return t1;
	}

	AVSValue args1[15] = { Source };
	VideoInfo vi = CreateFFVideoSource(AVSValue(args1, 15), UserData, Env).AsClip()->GetVideoInfo();

	int framerate = Env->GetTimelineInfo()->FrameRate;
	int length = vi.num_frames * framerate / (vi.fps_numerator / vi.fps_denominator);


	AVSValue args[4] = { length,channels,framerate, samplerate };
	char* names[4] = { (char*)"length",(char*)"channels", (char*)"fps" ,(char*)"audio_rate" };
	AVSValue blankclip = Env->Invoke("BlankClip", AVSValue(args, 3), names);

	return Env->Invoke("KillVideo", blankclip, NULL);
}

EXTERN_C const char* ffms2_init(IScriptEnvironment* Env) {

	Env->AddFunction("FFIndex", "[source]s[cachefile]s[indexmask]i[errorhandling]i[overwrite]b", CreateFFIndex, nullptr);
	
	Env->AddFunction("TinyAudio", "[source]s", CreateTinyAudio, nullptr);
	
	Env->AddFunction("FFVideoSource", "[source]s[track]i", CreateFFVideoSource, nullptr);//ML_Timeline 783
	
	Env->AddFunction("FFVideoSource1", "[source]s[track]i", CreateFFVideoSource1, nullptr); //ML_Timeline 2033
	
	Env->AddFunction("FFAudioSource", "[source]s[track]i[cache]b[cachefile]s[adjustdelay]i[varprefix]s", CreateFFAudioSource, nullptr);//ML_Timeline 180

	Env->AddFunction("AVS_VF_Ffmpeg", "csii", AVS_VF_Ffmpeg::Create, nullptr); //ML_Timeline 1860

	Env->AddFunction("FFVideoRevert", "[video]c[revert]b", FFVideoRevert, nullptr);  //ML_Timeline 2036

	return "FFmpegSource - The Second Coming V2.0 Final";
}

