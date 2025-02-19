//
// Created by wangrl2016 on 2023/2/28.
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <glog/logging.h>
#include "media/filters/ffmpeg_glue.h"

namespace media {
    using testing::StrictMock;
    using testing::Return;
    class MockProtocol : public FFmpegURLProtocol {
    public:
        MockProtocol() = default;

        MockProtocol(const MockProtocol&) = delete;

        MockProtocol& operator=(const MockProtocol&) = delete;

        virtual ~MockProtocol() = default;

        MOCK_METHOD2(Read, int(int size, uint8_t * data));
        MOCK_METHOD1(GetPosition, bool(int64_t*position_out));
        MOCK_METHOD1(SetPosition, bool(int64_t position));
        MOCK_METHOD1(GetSize, bool(int64_t*size_out));
        MOCK_METHOD0(IsStreaming, bool());
    };

    class FFmpegGlueTest : public testing::Test {
    public:
        FFmpegGlueTest() :
                protocol_(new StrictMock<MockProtocol>()) {
            EXPECT_CALL(*protocol_.get(), IsStreaming()).WillOnce(Return(true));
            glue_ = std::make_unique<FFmpegGlue>(protocol_.get());
            CHECK(glue_->format_context());
            CHECK(glue_->format_context()->pb);
        }

    protected:
        std::unique_ptr<FFmpegGlue> glue_;
        std::unique_ptr<testing::StrictMock<MockProtocol>> protocol_;
    };

    // Ensure writing has been disabled.
    TEST_F(FFmpegGlueTest, Write) {
        ASSERT_FALSE(glue_->format_context()->pb->write_packet);
        ASSERT_FALSE(glue_->format_context()->pb->write_flag);
    }
}
