//
// Created by wangrl2016 on 2023/2/27.
//

#include <vector>
#include <memory>
#include <gtest/gtest.h>
#include "media/base/audio_bus.h"
#include "media/base/decoder_buffer.h"
#include "media/base/test_data_util.h"
#include "media/filters/audio_file_reader.h"
#include "media/filters/memory_protocol.h"


namespace media {
    class AudioFileReaderTest : public testing::Test {
    public:
        AudioFileReaderTest() : packet_verification_disabled_(false) {}

        AudioFileReaderTest(const AudioFileReaderTest&) = delete;
        AudioFileReaderTest& operator=(const AudioFileReaderTest&) = delete;

        ~AudioFileReaderTest() override = default;

        void Initialize(const char* filename) {
            data_ = ReadTestDataFile(filename);
            protocol_ = std::make_unique<MemoryProtocol>(
                    data_->data(), data_->data_size(), false);
            reader_ = std::make_unique<AudioFileReader>(protocol_.get());
        }

        void ReadAndVerify(const char* expected_audio_hash, int expected_frames) {
            std::vector<std::unique_ptr<AudioBus>> decoded_audio_packets;
            int actual_frames = reader_->Read(&decoded_audio_packets);
            std::unique_ptr<AudioBus> decoded_audio_data =
                    AudioBus::Create(reader_->channels(), actual_frames);
            int dest_start_frame = 0;
            for (size_t k = 0; k < decoded_audio_packets.size(); k++) {
                const AudioBus* packet = decoded_audio_packets[k].get();
                int frame_count = packet->frames();
                packet->CopyPartialFramesTo(0, frame_count, dest_start_frame,
                                            decoded_audio_data.get());
                dest_start_frame += frame_count;
            }

            ASSERT_LE(actual_frames, decoded_audio_data->frames());
        }

    protected:
        base::scoped_ref_ptr<DecoderBuffer> data_;
        std::unique_ptr<MemoryProtocol> protocol_;
        std::unique_ptr<AudioFileReader> reader_;
        bool packet_verification_disabled_;
    };

    TEST_F(AudioFileReaderTest, WithoutOpen) {
        Initialize("bunny.mp4");
    }
}