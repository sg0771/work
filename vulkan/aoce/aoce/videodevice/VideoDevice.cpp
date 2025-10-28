#include "VideoDevice.hpp"

namespace aoce {
    
VideoDevice::VideoDevice(/* args */) {
    name.resize(AOCE_VIDEO_MAX_NAME, 0);
    id.resize(AOCE_VIDEO_MAX_NAME, 0);
}

VideoDevice::~VideoDevice() {}

void VideoDevice::onVideoFrameAction(VideoFrame frame) {
    if (observer) {
        observer->onVideoFrame(frame);
    }
}

void VideoDevice::onDepthFrameAction(VideoFrame colorFrame,
                                     VideoFrame depthFrame, void* alignParamt) {
    if (observer) {
        observer->onDepthVideoFrame(colorFrame, depthFrame, alignParamt);
    }
}

void VideoDevice::onDeviceAction(VideoHandleId id, int32_t codeId) {
    if (observer) {
        observer->onDeviceHandle(id, codeId);
    }
}

int32_t VideoDevice::findFormatIndex(int32_t width, int32_t height,
                                     int32_t fps) {
    int32_t index = 0;
    if (formats.size() < 0) {
        return -1;
    }
    bool bFind = false;
    int32_t first = -1;
    int32_t second = -1;
    for (const VideoFormat& format : formats) {
        if (format.width == width && format.height == height) {
            bFind = true;
            // 如果全满足,直接返回
            if (format.fps == fps && format.videoType != VideoType::mjpg) {
                return index;
            }
            // 尽量不选MJPG,多了解码的消耗
            if (format.videoType != VideoType::mjpg) {
                if (first < 0 || formats[first].fps > format.fps) {
                    first = index;
                }
            } else {
                if (second < 0 || formats[second].fps > format.fps) {
                    second = index;
                }
            }
        }
        index++;
    }
    if (bFind) {
        return first >= 0 ? first : second;
    }
    return -1;
}

int32_t VideoDevice::findFormatIndex(int32_t width, int32_t height,
                                     std::function<bool(VideoFormat)> filter) {
    int32_t index = 0;
    if (formats.size() < 0) {
        return -1;
    }
    for (const VideoFormat& format : formats) {
        if (format.width == width && format.height == height) {
            if (filter(format)) {
                return index;
            }
        }
        index++;
    }
    return -1;
}

}  // namespace aoce