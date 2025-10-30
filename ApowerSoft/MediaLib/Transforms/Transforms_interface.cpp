#include "utils.hpp"

#include <ImageProcess/reflector_app.h>

#define FILTER_NAME(x)  AVSValue __cdecl Create_##x(AVSValue , void* , IScriptEnvironment* );

//Frei0r
FILTER_NAME(Frei0r)
FILTER_NAME(Frei0r1)
FILTER_NAME(FreMixer)

//Rotate.dll
FILTER_NAME(Rotate)
FILTER_NAME(HShear)
FILTER_NAME(VShear)

//TransAll.dll
FILTER_NAME(TransAccord)
FILTER_NAME(TransBubbles)
FILTER_NAME(TransCentral)
FILTER_NAME(TransCrumple)
FILTER_NAME(TransDisco)
FILTER_NAME(TransFlipPage)
FILTER_NAME(TransFlipTurn)
FILTER_NAME(TransFunnel)
FILTER_NAME(TransPaint)
FILTER_NAME(TransFlipTurn)
FILTER_NAME(TransPeel)

FILTER_NAME(TransPush)
FILTER_NAME(TransRipple)
FILTER_NAME(TransRipples)
FILTER_NAME(TransRoll)
FILTER_NAME(TransScratch)
FILTER_NAME(TransShuffle)
FILTER_NAME(TransSlantRollIn)
FILTER_NAME(TransSlantRollOut)
FILTER_NAME(TransSlantWipe)
FILTER_NAME(TransSlide)
FILTER_NAME(TransSlideIn)
FILTER_NAME(TransSlideOut)
FILTER_NAME(TransSprite)
FILTER_NAME(TransSwing)
FILTER_NAME(TransSwirl)
FILTER_NAME(TransTwinDoors)
FILTER_NAME(TransVenetianBlinds)
FILTER_NAME(TransWeave)
FILTER_NAME(TransWipe)
FILTER_NAME(TransDoor)

//Transforms.dll
FILTER_NAME(AssSubtitle)
FILTER_NAME(AudioFade)

//opengl filter
FILTER_NAME(GlMotion)
FILTER_NAME(SimpleGLFilter)
FILTER_NAME(Transition)
FILTER_NAME(GlFilter)
FILTER_NAME(GlLayer)
FILTER_NAME(lut_2d)
FILTER_NAME(Chromakey)
FILTER_NAME(TransMarbles)

FILTER_NAME(GLColorAdjust)
FILTER_NAME(FaceTransform)
FILTER_NAME(EdgeBlur)
FILTER_NAME(EdgeIncrease)
FILTER_NAME(AdaptHistEq)
FILTER_NAME(AdaptHistEqEx)
FILTER_NAME(dynamicZoom)
FILTER_NAME(PointMotion)
FILTER_NAME(lut)
FILTER_NAME(ChromakeyShell)
FILTER_NAME(ChromakeyEx)

FILTER_NAME(CommonMotion)
FILTER_NAME(Delogo_guass)
FILTER_NAME(ColorAdjust)
FILTER_NAME(WaveForm)
FILTER_NAME(WICImage)
FILTER_NAME(LoopFrames)
FILTER_NAME(FrameCache)
FILTER_NAME(ImageMask)
FILTER_NAME(AlphaMask)
FILTER_NAME(DeLogo)

//change hue
// 定义 RGB 到 HSV 的转换函数
static void RGBtoHSV(double r, double g, double b, double& h, double& s, double& v) {
    double minVal = std::min(r, std::min(g, b));
    double maxVal = std::max(r, std::max(g, b));
    double delta = maxVal - minVal;

    v = maxVal;

    if (delta == 0) {
        h = 0;
        s = 0;
    }
    else {
        s = delta / maxVal;

        if (r == maxVal) {
            h = (g - b) / delta;
        }
        else if (g == maxVal) {
            h = 2 + (b - r) / delta;
        }
        else {
            h = 4 + (r - g) / delta;
        }

        h *= 60;
        if (h < 0) h += 360;
    }
}

// 定义 HSV 到 RGB 的转换函数
static void HSVtoRGB(double h, double s, double v, double& r, double& g, double& b) {
    if (s == 0) {
        r = g = b = v;
    }
    else {
        int i = int(floor(h / 60)) % 6;
        double f = (h / 60) - floor(h / 60);
        double p = v * (1 - s);
        double q = v * (1 - s * f);
        double t = v * (1 - s * (1 - f));

        switch (i) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
        }
    }
}


// 定义 ChangeHUE 滤镜类
class ChangeHUE_Filter : public GenericVideoFilter {
    int hue_shift = 0;
    int m_picth = 4;
public:
    ChangeHUE_Filter(PClip _child, int _hue_shift, IScriptEnvironment* env)
        : GenericVideoFilter(_child, __FUNCTION__) {
        if (!vi.IsRGB24() && !vi.IsRGB32()) {
            env->ThrowError("ChangeHUE: only RGB24/RGB32 input is supported.");
        }
        hue_shift = _hue_shift * 30;
        if (vi.IsRGB24())
            m_picth = 3;
    }

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) {
        PVideoFrame src = child->GetFrame(n, env);
        PVideoFrame dst = env->NewVideoFrame(vi);

        const unsigned char* srcp = src->GetReadPtr();
        unsigned char* dstp = dst->GetWritePtr();
        int pitch = src->GetPitch();
        int width = vi.width;
        int height = vi.height;

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int index = y * pitch + x * m_picth;
                double r = srcp[index + 2] / 255.0;
                double g = srcp[index + 1] / 255.0;
                double b = srcp[index] / 255.0;

                double h, s, v;
                RGBtoHSV(r, g, b, h, s, v);

                h += hue_shift;
                if (h > 360) h -= 360;
                if (h < 0) h += 360;

              //  RGBtoHSV(r, g, b, h, s, v);
                HSVtoRGB(h, s, v, r, g, b);

                dstp[index + 2] = static_cast<unsigned char>(r * 255);
                dstp[index + 1] = static_cast<unsigned char>(g * 255);
                dstp[index] = static_cast<unsigned char>(b * 255);
            }
        }

        return dst;
    }
};

// 导出函数，用于创建 ChangeHUE 滤镜实例
AVSValue __cdecl Create_ChangeHUE(AVSValue args, void* user_data, IScriptEnvironment* env) {
    return new ChangeHUE_Filter(args[0].AsClip(), args[1].AsInt(0), env);
}
AVSValue __cdecl Create_ZoomsFilter(AVSValue args, void* user_data, IScriptEnvironment* env);

EXTERN_C const char* Transforms_init(IScriptEnvironment *env) {
	
#if _WIN32
    static App* s_app = NULL;
    utils::MutexQueueImp::OpenglInstance()->sync([] {s_app = new App(); });
#endif

    //transition.avs
    //色调调节
    env->AddFunction("ChangeHUE", "c[hange_type]i", Create_ChangeHUE, nullptr);//软件调节HUE  i=(0-11)

    //ML_Timeline.cpp
    //缩放
    env->AddFunction("Zooms", "c[ZoomContext]p", Create_ZoomsFilter, nullptr);//Zooms

    //Rotate.dll
    env->AddFunction("Rotate", "c[angle]f[color]i[start]i[end]i[endangle]f[width]i[height]i[aspect]f", Create_Rotate, 0);//ML_Timeline
    env->AddFunction("HShear", "c[angle]f[color]i[start]i[end]i[endangle]f[width]i[height]i[aspect]f", Create_HShear, 0);
    env->AddFunction("VShear", "c[angle]f[color]i[start]i[end]i[endangle]f[width]i[height]i[aspect]f", Create_VShear, 0);
    // The AddFunction has the following paramters:
    // AddFunction(Filtername , Arguments, Function to call,0);

    //TransAll.dll
    env->AddFunction("TransAccord", "ccis[twin]b[open]b", Create_TransAccord, 0);
    env->AddFunction("TransBubbles", "cci[static]b", Create_TransBubbles, 0);
    env->AddFunction("TransCentral", "cci[emerge]b[resize]b[revolve]i", Create_TransCentral, 0);//Central.avs
    env->AddFunction("TransCrumple", "cci[type]s[fold]b", Create_TransCrumple, 0);
    env->AddFunction("TransDisco", "cci[radius]i[nturns]i[emerge]b", Create_TransDisco, 0);//utils.avs
    env->AddFunction("TransFlipPage", "cci[dir]s", Create_TransFlipPage, 0);
    env->AddFunction("TransFlipTurn", "cci[vflip]i[hflip]i[nturns]i", Create_TransFlipTurn, 0);
    env->AddFunction("TransFunnel", "cci[dir]s", Create_TransFunnel, 0);
    env->AddFunction("TransMarbles", "cci[radius]i[mag]i[drop]b", Create_TransMarbles, 0);
    env->AddFunction("TransPaint", "cci[type]s", Create_TransPaint, 0);
    env->AddFunction("TransPeel", "cci[dir]s[rollDia]i[shade]i[pull]b", Create_TransPeel, 0);
    env->AddFunction("TransPush", "cci[dir]s", Create_TransPush, 0);//Push_Left.avs
    env->AddFunction("TransRipple", "cci[lambda]i[amp]i[origin]s", Create_TransRipple, 0);
    env->AddFunction("TransRipples", "cci[lambda]i[amp]i[origin]s", Create_TransRipples, 0); //Ripples_Center.avs
    env->AddFunction("TransRoll", "cci[dir]i[rollin]b", Create_TransRoll, 0);
    env->AddFunction("TransScratch", "cci[style]s", Create_TransScratch, 0);//#000008_Scratch_Merge.avs
    env->AddFunction("TransShuffle", "cci[dir]s", Create_TransShuffle, 0);//Shuffle_Down.avs
    env->AddFunction("TransSlantRollIn", "cci[dir]s[rolldia]i[shade]i", Create_TransSlantRollIn, 0);
    env->AddFunction("TransSlantRollOut", "cci[dir]s[rolldia]i[shade]i", Create_TransSlantRollOut, 0);//SlantRoll_Out.avs SlantRoll_Out_NE.avs
    env->AddFunction("TransSlantWipe", "cci[dir]s", Create_TransSlantWipe, 0);//Slant_Wipe_NE.avs
    env->AddFunction("TransSlide", "cci[dir]i[slidein]b", Create_TransSlide, 0);
    env->AddFunction("TransSlideIn", "cci[dir]s", Create_TransSlideIn, 0);
    env->AddFunction("TransSlideOut", "cci[dir]s", Create_TransSlideOut, 0);
    env->AddFunction("TransSprite", "cci[dir]s", Create_TransSprite, 0);
    env->AddFunction("TransSwing", "cci[out]b[ndoors]i[corner]i[dir]b", Create_TransSwing, 0);
    env->AddFunction("TransSwirl", "cci[dir]s[step]i", Create_TransSwirl, 0);
    env->AddFunction("TransTwinDoors", "cci[vert]b[open]b", Create_TransTwinDoors, 0);
    env->AddFunction("TransVenetianBlinds", "cci[width]i[type]s", Create_TransVenetianBlinds, 0);
    env->AddFunction("TransWeave", "cci[type]s", Create_TransWeave, 0);
    env->AddFunction("TransWipe", "cci[dir]i", Create_TransWipe, 0); 
    //Wipe_Left.avs   Wipe_Right.avs
    env->AddFunction("TransDoor", "cci[vert]b[open]b", Create_TransDoor, 0);

    
#if  _WIN32
    env->AddFunction("Transition", "cc[left_overlap]i[right_overlap]i[type]s[type_param]s[mask_range]i", Create_Transition, nullptr);//ML_Timeline.cpp 1286

    env->AddFunction("GlFilter", "c[start]i[end]i[width]i[height]i[shader]s[vertexshader]s*", Create_GlFilter, nullptr);// filter.avs

    env->AddFunction("GlLayer", "cc[start]i[end]i[width]i[height]i[shader]s", Create_GlLayer, nullptr);//ML_Timeline.cpp 1583

    env->AddFunction("lut_2d", "cc[start]i[end]i[width]i[height]i[shader]s", Create_lut_2d, nullptr);//ML_Timeline.cpp 1869

    env->AddFunction("GlMotion", "cc[start]i[end]i[width]i[height]i[shader]s[rect]f+", Create_GlMotion, nullptr);//ML_Timeline.cpp 1459

    env->AddFunction("Chromakey", "c[start]i[end]i[width]i[height]i[R]f[G]f[B]f[dR]f[dG]f[dB]f[_threshhold]f[_blurSize]f", Create_Chromakey, nullptr);//opengl_ChromakeyShell.cpp 110

    env->AddFunction("GLColorAdjust", "c[start]i[end]i[width]i[height]i[saturation]f[hue]f[brightness]f[contrast]f[brightness]f[highlights]f[shader]s", Create_GLColorAdjust, nullptr);//ML_Timeline.cpp 567

    env->AddFunction("FaceTransform", "c[start]i[end]i[width]i[height]i[shadows]s*", Create_FaceTransform, nullptr);// unused

    env->AddFunction("EdgeBlur", "c[start]i[end]i[width]i[height]i[edgeWidth]f", Create_EdgeBlur, nullptr);// unused

    env->AddFunction("EdgeIncrease", "c[start]i[end]i[width]i[height]i[blurRadius]f", Create_EdgeIncrease, nullptr);// unused

    env->AddFunction("AdaptHistEq", "c[start]i[end]i[width]i[height]i[threshold]i[degree]f", Create_AdaptHistEq, nullptr);// unused

    env->AddFunction("AdaptHistEqEx", "c[start]i[end]i[width]i[height]i[threshold]i[degree]f", Create_AdaptHistEqEx, nullptr);// unused

    env->AddFunction("dynamicZoom", "c[start]i[end]i[width]i[height]i[rects]f+", Create_dynamicZoom, nullptr);// unused

    env->AddFunction("lut", "c[start]i[end]i[width]i[height]i[lutFilePath]s", Create_lut, nullptr);

    env->AddFunction("ChromakeyShell", "c[start]i[end]i[width]i[height]i[Color]i[dcolor]i[_threshhold]f[_blurSize]f", Create_ChromakeyShell, nullptr);//ML_Timeline.cpp 583

    env->AddFunction("ChromakeyEx", "c[start]i[end]i[width]i[height]i[Color]i[edgeMin]f[edgeMax]f[edgeWidth]f[blurRadius]f", Create_ChromakeyEx, nullptr);//ML_Timeline.cpp 598

    env->AddFunction("DeLogo_gauss", "c[start]i[end]i[width]i[height]i[mod]i[alpha]f[degree]f[rects]f+", Create_Delogo_guass, nullptr);//ML_Timeline.cpp 552

    env->AddFunction("ColorAdjust", "c[start]i[end]i[width]i[height]i[saturation]f[hue]f[brightness]f[contrast]f[brightness]f[highlights]f[shader]s", Create_ColorAdjust, nullptr);//ML_Timeline.cpp 569
#endif

    //音频波形图
    env->AddFunction("WaveForm", "[count]i[height]i[color]i[fps]i", Create_WaveForm, nullptr);


    env->AddFunction("WICImage", "s[width]i[height]i[rotate_flip]i", Create_WICImage, nullptr);//ML_Timeline.cpp 683
	env->AddFunction("LoopFrames", "c[frames]i[start]i[end]i", Create_LoopFrames, nullptr);//ML_Timeline.cpp 850
    env->AddFunction("FrameCache", "c[indexes]s", Create_FrameCache, nullptr);//opengl_transition.cpp 253 260
    env->AddFunction("ImageMask", "c[mask]s[x]f[y]f[w]f[h]f[alpha_or_grayscale]b", Create_ImageMask, nullptr);//util.avs
    env->AddFunction("AlphaMask", "cc", Create_AlphaMask, nullptr);//ImageMask.cpp 78
    env->AddFunction("AudioFade", "c[ranges_str]s[amplify]f", Create_AudioFade, nullptr);//

    env->AddFunction("DeLogo", "c[start]i[end]i[rects]s[border]i", Create_DeLogo, nullptr);//ML_TImeline.cpp 2987
    env->AddFunction("frei0r",   "cssii", Create_Frei0r, nullptr);//filter.avs
    env->AddFunction("frei0r",   "css",   Create_Frei0r1, nullptr);//filter.avs
    env->AddFunction("fremixer", "cciss", Create_FreMixer, nullptr);// unused
    env->AddFunction("AssSubtitle", "c[assfile]s[font]s[offset]i[start]i[end]i", Create_AssSubtitle, nullptr);//ML_Timeline.cpp 1801
    
    return "apowersoft image plugin";
}

