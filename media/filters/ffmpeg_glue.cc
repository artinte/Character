//
// Created by wangrl2016 on 2023/2/28.
//

#include <glog/logging.h>
#include "media/filters/ffmpeg_glue.h"

namespace media {
    enum {
        kBufferSize = 32 * 1024
    };

    static int AVIOReadOperation(void* opaque, uint8_t* buf, int buf_size) {
        return reinterpret_cast<FFmpegURLProtocol*>(opaque)->Read(buf_size, buf);
    }

    static int64_t AVIOSeekOperation(void* opaque, int64_t offset, int whence) {
        auto* protocol = reinterpret_cast<FFmpegURLProtocol*>(opaque);
        int64_t new_offset = AVERROR(EIO);
        switch (whence) {
            case SEEK_SET:
                if (protocol->SetPosition(offset))
                    protocol->GetPosition(&new_offset);
                break;

            case SEEK_CUR:
                int64_t pos;
                if (!protocol->GetPosition(&pos))
                    break;
                if (protocol->SetPosition(pos + offset))
                    protocol->GetPosition(&new_offset);
                break;

            case SEEK_END:
                int64_t size;
                if (!protocol->GetSize(&size))
                    break;
                if (protocol->SetPosition(size + offset))
                    protocol->GetPosition(&new_offset);
                break;

            case AVSEEK_SIZE:
                protocol->GetSize(&new_offset);
                break;

            default:
                assert(false);
        }
        return new_offset;
    }

    FFmpegGlue::FFmpegGlue(FFmpegURLProtocol* protocol) {
        // Initialize an AVIOContext using our custom read and seek operations.  Don't
        // keep pointers to the buffer since FFmpeg may reallocate it on the fly.  It
        // will be cleaned up
        format_context_ = avformat_alloc_context();
        io_context_.reset(avio_alloc_context(
                static_cast<unsigned char*>(av_malloc(kBufferSize)),
                kBufferSize,
                0,
                protocol,
                &AVIOReadOperation,
                nullptr,
                &AVIOSeekOperation));

        // Ensure FFmpeg only tries to seek on resources we know to be seekable.
        io_context_->seekable =
                protocol->IsStreaming() ? 0 : AVIO_SEEKABLE_NORMAL;

        // Ensure writing is disabled.
        io_context_->write_flag = 0;

        // Tell the format context about our custom IO context.  avformat_open_input()
        // will set the AVFMT_FLAG_CUSTOM_IO flag for us, but do so here to ensure an
        // early error state doesn't cause FFmpeg to free our resources in error.
        format_context_->flags |= AVFMT_FLAG_CUSTOM_IO;

        // Enable fast, but inaccurate seeks for MP3.
        format_context_->flags |= AVFMT_FLAG_FAST_SEEK;

        // Ensures format parsing errors will bail out. From an audit on 11/2017, all
        // instances were real failures. Solves bugs like http://crbug.com/710791.
        format_context_->error_recognition |= AV_EF_EXPLODE;

        format_context_->pb = io_context_.get();
    }

    FFmpegGlue::~FFmpegGlue() {
        // In the event of avformat_open_input() failure, FFmpeg may sometimes free
        // our AVFormatContext behind the scenes, but leave the buffer alive.  It will
        // helpfully set |format_context_| to nullptr in this case.
        if (!format_context_) {
            av_free(io_context_->buffer);
            return;
        }

        // If avformat_open_input() hasn't been called, we should simply free the
        // AVFormatContext and buffer instead of using avformat_close_input().
        if (!open_called_) {
            avformat_free_context(format_context_);
            av_free(io_context_->buffer);
            return;
        }

        avformat_close_input(&format_context_);
        av_free(io_context_->buffer);
    }

    bool FFmpegGlue::OpenContext(bool is_local_file) {
        DCHECK(!open_called_) << "OpenContext() shouldn't be called twice.";

        // If avformat_open_input() is called we have to take a slightly different
        // destruction path to avoid double frees.
        open_called_ = true;

        // By passing nullptr for the filename (second parameter) we are telling
        // FFmpeg to use the AVIOContext we set up from the AVFormatContext structure.
        const int ret = avformat_open_input(&format_context_, nullptr, nullptr, nullptr);

        if (ret == AVERROR_INVALIDDATA || ret < 0) {
            return false;
        }

        return true;
    }
}
