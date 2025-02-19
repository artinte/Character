//
// Created by wangrl2016 on 2023/2/28.
//

#ifndef ONESTUDIO_MEMORY_PROTOCOL_H
#define ONESTUDIO_MEMORY_PROTOCOL_H

#include "media/filters/ffmpeg_glue.h"

namespace media {
    class MemoryProtocol : public FFmpegURLProtocol {
    public:
        MemoryProtocol() = delete;

        MemoryProtocol(const uint8_t* data, int64_t size, bool streaming);

        MemoryProtocol(const MemoryProtocol&) = delete;

        MemoryProtocol& operator=(const MemoryProtocol&) = delete;

        virtual ~MemoryProtocol();

        // FFmpegURLProtocol methods
        int Read(int size, uint8_t* data) override;

        bool GetPosition(int64_t* position_out) override;

        bool SetPosition(int64_t position) override;

        bool GetSize(int64_t* size_out) override;

        bool IsStreaming() override;

    private:
        const uint8_t* data_;
        int64_t size_;
        int64_t position_;
        bool streaming_;
    };
}

#endif //ONESTUDIO_MEMORY_PROTOCOL_H
