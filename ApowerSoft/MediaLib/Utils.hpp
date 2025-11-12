#ifndef __UTILS_HPP__
#define __UTILS_HPP__


//Windows
#include <Windows.h>
#include <io.h>
#include <objidl.h>
#include <direct.h>
#include <assert.h>
#include <mmsystem.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <Psapi.h>
#include <tchar.h>

//STL
#include <vector>
#include <locale>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <filesystem>
#include <future>
#include <queue>
#include <thread>
#include <mutex>
#include <memory>
#include <cassert>
#include <codecvt>
#include <any>
#include <fstream>
#include <utility>
#include <iostream>
#include <map>
#include <array>
#include <set>
#include <atomic>
#include <future>
#include <codecvt>
#include <functional>
#include <unordered_map>
#include <list>
#include <cstddef>
#include <xutility>
#include <type_traits>
#include <libyuv/libyuv.h>

#include <libass/ass.h>
#include <initializer_list>
#include <avisynth/avisynth.h>

#include <MediaLibAPI.h>

#include <WXBase.h>         //WXString 
#include <FfmpegIncludes.h> //FFMPEG
#include "LibInst.hpp" //第三方库加载
#include <wxlog.h>

EXTERN_C IScriptEnvironment* FFMS_GetEnv();

#define __SCOPEGUARD_CONCATENATE_IMPL(s1, s2) s1##s2
#define __SCOPEGUARD_CONCATENATE(s1, s2) __SCOPEGUARD_CONCATENATE_IMPL(s1, s2)

// Helper macro
#define ON_SCOPE_EXIT_OBJECT clover::detail::ScopeGuardOnExit() + [&]

#define ON_SCOPE_EXIT auto __SCOPEGUARD_CONCATENATE(ext_exitBlock_, __LINE__) = ON_SCOPE_EXIT_OBJECT

std::string  GetFullPathA(const char* filename);
std::wstring GetFullPathW(const wchar_t* filename);

class IScriptEnvironment;


static std::string WXFeadFileGB(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return "";
    }
    std::string retult((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    return retult;
}
static std::string WXFeadFileUTF8(const std::string& filenameUTF8) {
    std::wstring wstr = WXBase::UTF8ToUTF16(filenameUTF8);
    std::ifstream file(wstr);
    if (!file.is_open()) {
        return "";
    }
     std::string retult((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
     file.close();
     return retult;
}
static std::string WXFeadFileUTF16(const std::wstring& filenameUTF16) {
    std::ifstream file(filenameUTF16);
    if (!file.is_open()) {
        return "";
    }
    std::string retult((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    return retult;
}
class AssEngine {

public:

    ASS_Library* ass_library = nullptr;
    ASS_Renderer* ass_renderer = nullptr;

    AssEngine() {
        init();
    }
    ~AssEngine() {}

    static AssEngine& InstanceDX()
    {
        static AssEngine engine;
        return engine;
    }

    static AssEngine& InstanceML()
    {
        static AssEngine engine;
        return engine;
    }

    static void msg_callback(int level, const char* fmt, va_list va, void* data)
    {
        if (level > 6)
            return;
    }


    void init()
    {
        ass_library = ass_library_init();
        if (!ass_library) {
            printf("ass_library_init failed!\n");
            exit(1);
        }
        ass_set_message_cb(ass_library, msg_callback, NULL);
        ass_renderer = ass_renderer_init(ass_library);
        if (!ass_renderer) {
            printf("ass_renderer_init failed!\n");
            exit(1);
        }
        ass_set_fonts(ass_renderer, NULL, "arial", ASS_FONTPROVIDER_AUTODETECT, NULL, 1);
    }

    ASS_Track* Read(std::string strContext) {
        ASS_Track* track = nullptr;
        if (strContext.length() == 0)
            return track;
        track = ass_read_memory(this->ass_library, (char*)strContext.c_str(), strContext.length(), NULL);
        if (track != nullptr) { // 内存数据
            return track;
        }
        std::string assContext = WXFeadFileUTF8(strContext); //按UTF8读取
        if (assContext.length() == 0) {
            assContext = WXFeadFileGB(strContext);//按当前系统编码读取
        }
        if (assContext.length() == 0) { //空文本
            return nullptr;
        }
        track = ass_read_memory(this->ass_library, (char*)assContext.c_str(), assContext.length(), NULL);
        return track;
    }

};

typedef struct image_s {
    int width, height, stride;
    uint8_t* buffer;   // RGB24
} image_t;

static image_t* gen_image(int width, int height)
{
    image_t* img = (image_t*)av_malloc(sizeof(image_t));
    img->width = width;
    img->height = height;
    img->stride = width * 4;
    img->buffer = (uint8_t*)av_malloc(height * width * 4);
    return img;
}

class AVSValueArray {
    const std::initializer_list<AVSValue>& _values;

public:
    AVSValueArray(const std::initializer_list<AVSValue>& values) : _values{ values } { }

    operator AVSValue() const {
        return AVSValue(std::data(_values), (int)_values.size());
    }
};

namespace clover {
    template <typename Fun>
    class ScopeGuard {
    public:
        explicit ScopeGuard(Fun&& fn) : _fun{ std::forward<Fun>(fn) }, _active{ true } { }

        ~ScopeGuard() {
            if (_active) {
                _fun();
            }
        }

        ScopeGuard() = delete;
        ScopeGuard(const ScopeGuard&) = delete;
        ScopeGuard& operator=(const ScopeGuard&) = delete;

        ScopeGuard(ScopeGuard&& other) noexcept : _fun{ std::move(other._fun) },
            _active{ other._active } {
            other.dismiss();
        }

        ScopeGuard& operator=(ScopeGuard&& other) noexcept {
            if (this == &other)
                return *this;
            _fun = std::move(other._fun);
            _active = other._active;
            other.dismiss();
            return *this;
        }

        void dismiss() {
            _active = false;
        }

    private:
        Fun _fun;
        bool _active;
    };

    namespace detail {
        enum class ScopeGuardOnExit { };

        template <typename Fun>
        ScopeGuard<Fun> operator+(ScopeGuardOnExit, Fun&& fn) {
            return ScopeGuard<Fun>(std::forward<Fun>(fn));
        }
    }
}

template <typename T>
class blocking_queue {
    std::queue<T> _datas;
    std::mutex _mutex;
    std::condition_variable _cond_var;

    std::atomic_bool _stopped{ false };

public:
    blocking_queue(const blocking_queue& other) = delete;
    blocking_queue(blocking_queue&& other) noexcept = delete;
    blocking_queue& operator=(const blocking_queue& other) = delete;
    blocking_queue& operator=(blocking_queue&& other) noexcept = delete;

    blocking_queue() = default;
    ~blocking_queue() = default;

    void stop() {
        _stopped = true;
        _cond_var.notify_all();
    }

    bool is_ok() const {
        return !_stopped;
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _datas.empty();
    }

    void push(T new_data) {
        if (_stopped) {
            return;
        }

        {
            std::lock_guard<std::mutex> lock(_mutex);
            _datas.push(std::move(new_data));
        }
        _cond_var.notify_one();
    }

    bool try_pop(T& val) {
        if (_stopped) {
            return false;
        }

        std::lock_guard<std::mutex> lock(_mutex);
        if (_datas.empty()) {
            return false;
        }

        val = std::move(_datas.front());
        _datas.pop();
        return true;
    }

    bool wait_and_pop(T& val) {
        if (_stopped) {
            return false;
        }

        std::unique_lock<std::mutex> lock(_mutex);
        _cond_var.wait(lock, [this] {
            return _stopped || !_datas.empty();
            });

        if (_stopped) {
            return false;
        }

        val = std::move(_datas.front());
        _datas.pop();
        return true;
    }

    std::shared_ptr<T> wait_and_pop() {
        if (_stopped) {
            return false;
        }

        std::unique_lock<std::mutex> lock(_mutex);
        _cond_var.wait(lock, [this] {
            return _stopped || !_datas.empty();
            });

        if (_stopped) {
            return std::shared_ptr<T>();
        }

        std::shared_ptr<T> val(std::make_shared<T>(std::move(_datas.front())));
        _datas.pop();
        return val;
    }
};

// FilterFrame 当前帧是要作为2D纹理还是3D纹理
#define	FRAME_2D	0
#define FRAME_3D	1
// FilterFrame 当前帧需要的环绕方式
#define FRAME_MIRRORED_REPEAT	0
#define FRAME_CLAMP_TO_EDGE		1

struct FilterFrame {
    uint8_t* data;
    int width, height;
    std::string texturename;
    int pitch;
    bool Keepstay;
    int type = FRAME_2D;
    int deep = 0;
    int wrap = FRAME_MIRRORED_REPEAT;
};

namespace cache {

    template<typename key_t, typename value_t>
    class lru_cache {
    public:
        typedef typename std::pair<key_t, value_t> key_value_pair_t;
        typedef typename std::list<key_value_pair_t>::iterator list_iterator_t;

        lru_cache(size_t max_size = 2) :
            _max_size(max_size) {
        }

        void put(const key_t& key, const value_t& value) {
            auto it = _cache_items_map.find(key);
            _cache_items_list.push_front(key_value_pair_t(key, value));
            if (it != _cache_items_map.end()) {
                _cache_items_list.erase(it->second);
                _cache_items_map.erase(it);
            }
            _cache_items_map[key] = _cache_items_list.begin();

            if (_cache_items_map.size() > _max_size) {
                auto last = _cache_items_list.end();
                last--;
                _cache_items_map.erase(last->first);
                _cache_items_list.pop_back();
            }
        }

        const value_t& get(const key_t& key) {
            auto it = _cache_items_map.find(key);
            if (it == _cache_items_map.end()) {
                throw std::range_error("There is no such key in cache");
            }
            else {
                _cache_items_list.splice(_cache_items_list.begin(), _cache_items_list, it->second);
                return it->second->second;
            }
        }

        bool exists(const key_t& key) const {
            return _cache_items_map.find(key) != _cache_items_map.end();
        }

        size_t size() const {
            return _cache_items_map.size();
        }
        void clear()
        {
            _cache_items_map.clear();
            _cache_items_list.clear();
        }
    private:
        std::list<key_value_pair_t> _cache_items_list;
        std::unordered_map<key_t, list_iterator_t> _cache_items_map;
        size_t _max_size;
    };

    template<typename key_t, typename value_t>
    class lru_av_cache {
    public:
        typedef typename std::pair<float, value_t> weight_value_pair_t;
        typedef typename std::pair<key_t, weight_value_pair_t> key_value_pair_t;
        typedef typename std::list<key_value_pair_t>::iterator list_iterator_t;

        lru_av_cache(float maxweight, int maxsize = 0) :
            _weight(maxweight), _size(maxsize) {
        }
        void clear()
        {
            _cache_items_map.clear();
            _cache_items_list.clear();
        }
        void put(const key_t& key, const float weight, const value_t& value) {
            auto it = _cache_items_map.find(key);
            _cache_items_list.push_front(key_value_pair_t(key, weight_value_pair_t(weight, value)));
            if (it != _cache_items_map.end()) {
                _cache_items_list.erase(it->second);
                _cache_items_map.erase(it);
            }
            _cache_items_map[key] = _cache_items_list.begin();

            while (isfull()) {
                auto last = _cache_items_list.end();
                last--;
                _cache_items_map.erase(last->first);
                _cache_items_list.pop_back();
            }
        }

        const value_t& get(const key_t& key) {
            auto it = _cache_items_map.find(key);
            if (it == _cache_items_map.end()) {
                throw std::range_error("There is no such key in cache");
            }
            else {
                _cache_items_list.splice(_cache_items_list.begin(), _cache_items_list, it->second);
                return it->second->second.second;
            }
        }

        bool exists(const key_t& key) const {
            return _cache_items_map.find(key) != _cache_items_map.end();
        }

        size_t size() const {
            return _cache_items_map.size();
        }
        float weight() const {
            float totalweight = 0.0f;
            auto it = _cache_items_list.begin();
            while (it != _cache_items_list.end())
            {
                totalweight += (*it).second.first;
                it++;
            }
            return totalweight;
        }

    private:
        std::list<key_value_pair_t> _cache_items_list;
        std::unordered_map<key_t, list_iterator_t> _cache_items_map;
        float _weight;
        int _size;
        bool isfull()
        {
            if (_size > 0 && _cache_items_list.size() > _size)
                return true;

            float totalweight = 0.0f;
            auto it = _cache_items_list.begin();
            while (it != _cache_items_list.end())
            {
                totalweight += (*it).second.first;
                it++;
            }
            return totalweight > _weight;
        }
    };

} // namespace cache

namespace utils {

    template<class T>
    class mem_ops
    {
    public:
        static void copy(T* dst, const T* src, unsigned count) { memcpy(dst, src, count * sizeof(T)); }
        static void move(T* dst, const T* src, unsigned count) { memmove(dst, src, count * sizeof(T)); }
        static void set(T* dst, int val, unsigned count) { memset(dst, val, count * sizeof(T)); }
        static void setval(T* dst, T val, unsigned count) { for (; count; count--) *(dst++) = val; }
        static T* alloc(unsigned count) { return reinterpret_cast<T*>(malloc(count * sizeof(T))); }
        static T* alloc_zeromem(unsigned count)
        {
            T* ptr = alloc(count);
            if (ptr) set(ptr, 0, count);
            return ptr;
        }
        static T* realloc(T* ptr, unsigned count)
        {
            return ptr ? reinterpret_cast<T*>(::realloc(reinterpret_cast<void*>(ptr), count * sizeof(T))) : alloc(count);
        }

        static void free(T* ptr) { ::free(reinterpret_cast<void*>(ptr)); }
        inline static T make_null_item()
        {
            char item[sizeof(T)];
            memset(&item, 0, sizeof(T));
            return *reinterpret_cast<T*>(&item);
        }
        inline static void swap(T& item1, T& item2) { T temp; temp = item1; item1 = item2; item2 = temp; }
    };

    class mem_block
    {
    public:
        enum mem_logic_t { ALLOC_DEFAULT, ALLOC_FAST, ALLOC_FAST_DONTGODOWN };
    private:
        void* data = nullptr;
        size_t size = 0, used = 0;
        mem_logic_t mem_logic = ALLOC_DEFAULT;
    public:
        inline void set_mem_logic(mem_logic_t v) { mem_logic = v; }
        inline mem_logic_t get_mem_logic() const { return mem_logic; }

        inline mem_block() { data = 0; size = 0; used = 0; mem_logic = ALLOC_DEFAULT; }
        inline ~mem_block() { if (data) free(data); }

        inline size_t get_size() const { return used; }

        inline const void* get_ptr() const { return data; }
        inline void* get_ptr() { return data; }

        void* set_size(size_t new_used)
        {
            if (new_used == 0)
            {
                if (mem_logic != ALLOC_FAST_DONTGODOWN)
                {
                    if (data != 0) { free(data); data = 0; }
                    size = 0;
                }
            }
            else
            {
                size_t new_size;
                if (mem_logic == ALLOC_FAST || mem_logic == ALLOC_FAST_DONTGODOWN)
                {
                    new_size = size;
                    if (new_size < 1) new_size = 1;
                    while (new_size < new_used) new_size <<= 1;
                    if (mem_logic != ALLOC_FAST_DONTGODOWN) while (new_size >> 1 > new_used) new_size >>= 1;
                }
                else
                {
                    new_size = new_used;
                }

                if (new_size != size)
                {
                    if (data == 0)
                    {
                        data = malloc(new_size);
                    }
                    else
                    {
                        void* new_data;
                        new_data = realloc(data, new_size);
                        if (new_data == 0) free(data);
                        data = new_data;
                    }
                    size = new_size;
                }
            }
            used = new_used;
            return data;
        }

        inline void* check_size(size_t new_size) {
            if (used < new_size) return set_size(new_size);
            else return get_ptr();
        }
        inline operator const void* () const { return get_ptr(); }
        inline operator void* () { return get_ptr(); }
        inline void zeromemory() { memset(get_ptr(), 0, used); }
        inline void force_reset() { if (data) free(data); data = 0; size = 0; used = 0; }
    };

    template<class T>
    class mem_block_t //: public mem_block	
    {
        mem_block theBlock;//msvc7 sucks
    public:
        mem_block_t() {}
        mem_block_t(size_t s) { theBlock.set_size(s * sizeof(T)); }
        inline void set_mem_logic(mem_block::mem_logic_t v) { theBlock.set_mem_logic(v); }
        inline const T* get_ptr() const { return static_cast<const T*>(theBlock.get_ptr()); }
        inline T* get_ptr() { return static_cast<T*>(theBlock.get_ptr()); }
        inline T* set_size(size_t new_size) { return static_cast<T*>(theBlock.set_size(new_size * sizeof(T))); }
        inline T* check_size(size_t new_size) { return static_cast<T*>(theBlock.check_size(new_size * sizeof(T))); }
        inline operator const T* () const { return get_ptr(); }
        inline operator T* () { return get_ptr(); }
        inline void zeromemory() { theBlock.zeromemory(); }
    };

    class ML_Buffer
    {
    public:
        ML_Buffer() {

        }

        ML_Buffer(uint8_t* buf, int size) {
            init(buf, size);
        }

        ML_Buffer(int size) {
            init(size);
        }
        virtual ~ML_Buffer() {
            clear();
        }

    public://取值
        //地址
        template<typename T>
        T* ptr() {
            return (T*)m_p;
        }

        template<typename T>
        T operator [](size_t pos) { //重载[]
            T* arr = (T*)m_p;
            return arr[pos];
        }
        size_t size() { return m_iSize; }//总内存
    public:
        void init(size_t newsize) { //扩容
            if (newsize > m_iSize && newsize > 0) {
                uint8_t* tmp = (uint8_t*)av_malloc(newsize);
                if (tmp) {
                    if (m_p) {
                        memcpy(tmp, m_p, m_iSize); //复制原来内容
                        av_free(m_p);
                        m_p = nullptr;
                    }
                    else {
                        memset(tmp, 0, newsize);
                    }
                    m_p = tmp;
                    m_iSize = newsize;
                }
            }
        }
        void clear() { //置空
            if (m_p) {
                av_free(m_p);
                m_p = nullptr;
            }
        }
        void init(uint8_t* buf, int size) { //按内存初始化
            if (size > 0) {
                m_p = (uint8_t*)av_malloc(size);
                if (buf)
                    memcpy(m_p, buf, size);
                m_iSize = size;
            }
        }
    private:
        size_t m_iSize = 0;
        uint8_t* m_p = nullptr;
    };

    class ML_FileBuffer :public ML_Buffer {
    public:
        ML_FileBuffer() {}

        //读取文件到内存，maxsize为0 表示读取所有内容
        ML_FileBuffer(std::string strU8, size_t maxsize = 0) {
            read_file(strU8, maxsize);
        }

        ML_FileBuffer(std::wstring strU16, size_t maxsize = 0) {
            read_file(strU16, maxsize);
        }
    public:
        //读取文件内容到buffer
        void read_file(std::string strFileName, size_t maxsize = 0) {
            std::wstring wstrFileName = WXBase::UTF8ToUTF16(strFileName);
            read_file(wstrFileName, maxsize);
        }

        //读取文件内容到buffer
        void read_file(std::wstring strFileName, size_t maxsize = 0) {
            uint64_t filesize = WXBase::Filesize(strFileName);
            if (filesize == 0)
                return;
            size_t readsize = filesize;//读取长度
            if (maxsize != 0) {
                readsize = maxsize;
                if (filesize < maxsize) {
                    readsize = filesize;
                }
            }
            std::ifstream fin(strFileName, std::ios::binary);
            if (fin.is_open()) {
                init(readsize);
                fin.read(this->ptr<char>(), readsize);
                fin.close();
            }
        }
    };

    class ML_FileHandle {
        utils::ML_FileBuffer m_readBuf;//ReadFile
        bool m_bRead = false;
        bool m_bFile = false;

        utils::ML_FileBuffer m_writeBuf;//WriteToFile
        size_t m_nSize = 0;
        size_t m_pos = 0;

        std::wstring m_wstrFileName = L"";
        std::string m_strFileName = "";
    public:
        ML_FileHandle(const char* filename, const char* mode) {
            if (stricmp(mode, "rb") == 0) {
                m_bRead = true;
                m_readBuf.read_file(filename); //读取Buffer
            }
            else {
                m_strFileName = filename;//写入文件
                m_wstrFileName = WXBase::UTF8ToUTF16(filename);
                m_bRead = false;
                m_nSize = 65536;
                m_writeBuf.init(m_nSize);
            }
            m_bFile = true;
        }

        ML_FileHandle(const wchar_t* filename, const wchar_t* mode) {
            if (wcsicmp(mode, L"rb") == 0) {
                m_bRead = true;
                m_readBuf.read_file(filename); //读取Buffer
            }
            else {
                m_wstrFileName = filename;//写入文件
                m_bRead = false;
                m_nSize = 65536;
                m_writeBuf.init(m_nSize);
            }
            m_bFile = true;
        }

        ML_FileHandle(uint8_t* buf, int size) { //内存初始化
            m_bFile = false;
            m_bRead = true;
            m_readBuf.init(buf, size);
        }

        ML_FileHandle() { //可写入
            m_bFile = false;
            m_bRead = false;
            m_nSize = 65536;
            m_writeBuf.init(m_nSize);
        }

        virtual ~ML_FileHandle() {
            //释放
        }

        //读取一段数据
        void Read(void* data, size_t size) {
            if (m_bRead) {
                memcpy(data, m_readBuf.ptr<uint8_t>() + m_pos, size);
                m_pos += size;
            }
        }

        size_t Write(const void* data, size_t size) {
            if (!m_bRead) {
                size_t new_pos = m_pos + size;
                if (new_pos > m_nSize) { //扩充内存
                    m_nSize *= 2;
                    m_writeBuf.init(m_nSize);
                }
                memcpy(m_writeBuf.ptr<uint8_t>() + m_pos, data, size);
                m_pos += size;
            }
            return size;
        }

        void Finish() {
            if (m_bFile && !m_bRead) {
                //写入文件
                std::ofstream fout(m_wstrFileName, std::ios::binary);
                if (fout.is_open()) {
                    fout.write(m_writeBuf.ptr<char>(), m_pos);
                    fout.close();
                }
                else {
                    WXLogW(L"std::ofstream %ws error", m_wstrFileName.c_str());
                }
            }
        }

        size_t size() {
            return m_bRead ? m_readBuf.size() : m_pos;//读取模式
        }

        template<typename T>
        T* ptr() {
            return m_bRead ? m_readBuf.ptr<uint8_t>() : m_writeBuf.ptr<T>();
        }

        template<typename T>
        T Read() {
            T ret = T();
            Read(&ret, sizeof(T));
            return ret;
        }

        template<typename T>
        void Write(T const& value) {
            Write(&value, sizeof value);
        }
    };

    class StringExt {
    public:
        static std::string base64_s2s(const std::string& src) {
            std::vector<char> bytes;
            if (src.find("base64:") == 0) {
                bytes = base64_decode(src.substr(7));
                bytes.push_back(0);
            }
            else {
                bytes.assign(src.cbegin(), src.cend());
            }

            try {
                return std::string{ bytes.cbegin(), bytes.cend() };
            }
            catch (const std::exception&) {
                return std::string();
            }
        }

        static std::string ltrim(std::string s) {
            s.erase(s.cbegin(), std::find_if(s.cbegin(), s.cend(), [](int ch) {
                return !std::isspace(ch);
                }));
            return s;
        }

        static std::string rtrim(std::string s) {
            s.erase(std::find_if(s.crbegin(), s.crend(), [](int ch) {
                return !std::isspace(ch);
                }).base(), s.cend());
            return s;
        }

        static std::string trim(const std::string& s) {
            return ltrim(rtrim(s));
        }

        static std::vector<char> base64_decode(const std::string& in) {
            static std::vector<int> MAP(256, -1);
            static bool initMAP;
            if (!initMAP) {
                for (int i = 0; i < 64; i++) {
                    MAP["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;
                }
                initMAP = true;
            }

            std::vector<char> out;

            int val = 0, valb = -8;
            for (auto c : in) {
                if (MAP[c] == -1) {
                    break;
                }

                val = (val << 6) + MAP[c];
                valb += 6;
                if (valb >= 0) {
                    out.push_back(char(val >> valb & 0xFF));
                    valb -= 8;
                }
            }

            return out;
        }

        static std::wstring base64_s2ws(const std::string& src) {
            std::vector<char> bytes;
            if (src.find("base64:") == 0) {
                bytes = base64_decode(src.substr(7));
                bytes.push_back(0);
            }
            else {
                bytes.assign(src.cbegin(), src.cend());
            }

            try {
                std::string ss{ bytes.cbegin(), bytes.cend() };
                return WXBase::UTF8ToUTF16(ss.c_str());
            }
            catch (const std::exception&) {
                return std::wstring();
            }
        }

        enum split_empties_t { split_empties_ok, split_no_empties };

        template <typename Container>
        static Container& split(
            Container& result,
            const typename Container::value_type& s,
            const typename Container::value_type& delimiters,
            split_empties_t empties = split_empties_ok) {
            result.clear();
            size_t next = -1;
            do {
                if (empties == split_no_empties) {
                    next = s.find_first_not_of(delimiters, next + 1);
                    if (next == Container::value_type::npos)
                        break;
                    next -= 1;
                }
                size_t current = next + 1;
                next = s.find_first_of(delimiters, current);
                result.push_back(s.substr(current, next - current));
            } while (next != Container::value_type::npos);
            return result;
        }

        template <typename Double = double,
            size_t Precision = std::numeric_limits<Double>::digits10,
            typename TimePoint>
        //requires std::is_floating_point_v<Double>
        //&& Precision <= std::numeric_limits<Double>::digits10
        static std::string toString(const TimePoint& timePoint) {
            auto seconds = Double(timePoint.time_since_epoch().count())
                * TimePoint::period::num / TimePoint::period::den;
            auto const zeconds = std::modf(seconds, &seconds);
            time_t tt(seconds);
            tm tm{ };
            auto err = localtime_s(&tm, &tt);
            if (err) {
                char errs[500];
                strerror_s(errs, err);
                throw std::runtime_error(errs);
            }
            std::ostringstream oss;
            oss << std::put_time(&tm, "%Y-%b-%d %H:%M:")
                << std::setw(Precision + 3) << std::setfill('0')
                << std::fixed << std::setprecision(Precision)
                << tm.tm_sec + zeconds;
            if (!oss)
                throw std::runtime_error("timepoint-to-string");
            return oss.str();
        }
    };

    class ML_QueueTask {
    public:
        virtual void run() = 0;

        ML_QueueTask() : _signal(false) {}
        virtual ~ML_QueueTask() {}
        void signal() {
            _signal = true;
            _condition.notify_all();
        }
        void wait() {
            std::unique_lock <std::mutex> lock(_mutex);
            _condition.wait(lock, [this]() { return _signal; });
            _signal = false;
        }

        void reset() {
            _signal = false;
        }
    private:
        bool _signal;
        std::mutex _mutex;
        std::condition_variable _condition;
    };

    template <class T>
    class ML_ClosureTask : public ML_QueueTask {
    public:
        explicit ML_ClosureTask(const T& closure) : _closure(closure) {}
    private:
        void run() override {
            _closure();
        }
        T _closure;
    };

    class ML_DispatchQueue {
    public:
        ML_DispatchQueue(int thread_count) {}

        virtual ~ML_DispatchQueue() {}

        template <class T, typename std::enable_if<std::is_copy_constructible<T>::value>::type* = nullptr>
        void sync(const T& task) {
            sync(std::shared_ptr<ML_QueueTask>(new ML_ClosureTask<T>(task)));
        }

        void sync(std::shared_ptr<ML_QueueTask> task) {
            if (nullptr != task) {
                sync_imp(task);
            }
        }

        template <class T, typename std::enable_if<std::is_copy_constructible<T>::value>::type* = nullptr>
        int64_t async(const T& task) {
            return async(std::shared_ptr<ML_QueueTask>(new ML_ClosureTask<T>(task)));
        }

        int64_t async(std::shared_ptr<ML_QueueTask> task) {
            if (nullptr != task) {
                return async_imp(task);
            }
            return -1;
        }

    protected:
        virtual void sync_imp(std::shared_ptr<ML_QueueTask> task) = 0;

        virtual int64_t async_imp(std::shared_ptr<ML_QueueTask> task) = 0;
    };

    class MutexQueueImp : public ML_DispatchQueue {

    public:
        static MutexQueueImp* OpenglInstance();

        MutexQueueImp(int thread_count)
            : ML_DispatchQueue(thread_count),
            _cancel(false),
            _thread_count(thread_count) {
            for (int i = 0; i < thread_count; i++) {
                create_thread();
            }
        }
        virtual ~MutexQueueImp() {
            exit(-1);//无法正常退出
            //_cancel = true;
            //_condition.notify_all();
            //for (auto& future : _futures) {
            //    future.wait();
            //}
        }

        void sync_imp(std::shared_ptr<ML_QueueTask> task) override {
            if (_thread_count == 1 && _thread_id == std::this_thread::get_id()) {
                task->reset();
                task->run();
                task->signal();
            }
            else {
                async_imp(task);
                task->wait();
            }
        }

        int64_t async_imp(std::shared_ptr<ML_QueueTask> task) override {
            _mutex.lock();
            task->reset();
            _dispatch_tasks.push(task);
            _mutex.unlock();
            _condition.notify_one();
            return 0;
        }

    private:

        MutexQueueImp(const MutexQueueImp&);

        void create_thread() {
            _futures.emplace_back(std::async(std::launch::async, [&]() {
                _thread_id = std::this_thread::get_id();
                while (!_cancel) {

                    {
                        std::unique_lock <std::mutex> work_signal(_signal_mutex);
                        _condition.wait(work_signal, [this]() {
                            return _cancel || !_dispatch_tasks.empty();
                            });
                    }

                    while (!_dispatch_tasks.empty() && !_cancel) {
                        _mutex.lock();
                        if (_dispatch_tasks.empty()) {
                            _mutex.unlock();
                            break;
                        }
                        std::shared_ptr<ML_QueueTask> task(_dispatch_tasks.front());
                        _dispatch_tasks.pop();
                        _mutex.unlock();
                        if (nullptr != task) {
                            task->run();
                            task->signal();
                        }
                    }
                }
                }));
        }

    private:
        std::vector<std::future<void>> _futures;
        std::queue<std::shared_ptr<ML_QueueTask>> _dispatch_tasks;
        std::recursive_mutex _mutex;
        bool _cancel;
        std::mutex _signal_mutex;
        std::condition_variable _condition;
        int _thread_count;
        std::thread::id _thread_id;
    };

}




#endif /* __UTILS_HPP__ */
