//MIT License
//
//Copyright (c) 2017 Mindaugas Vinkelis
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.

#include <gmock/gmock.h>
#include "serialization_test_utils.h"
#include <bitsery/ext/growable.h>

using namespace testing;

using bitsery::ext::Growable;

struct DataV1 {
    int32_t v1;
};

struct DataV2 {
    int32_t v1;
    int32_t v2;
};

struct DataV3 {
    int32_t v1;
    int32_t v2;
    int32_t v3;
};


TEST(SerializeExtensionGrowable, WriteSessionsDataAtBufferEndAfterFlush) {
    BasicSerializationContext<SessionsEnabledConfig, void> ctx;
    auto& ser = ctx.createSerializer();
    //session cannot be empty
    ser.ext(int8_t{}, Growable{}, [&ser] (int8_t& v) {
        ser.value1b(v);
    });

    EXPECT_THAT(ctx.getBufferSize(), Eq(1));
    ctx.bw->flush();
    EXPECT_THAT(ctx.getBufferSize(), Gt(1));
}


TEST(SerializeExtensionGrowable, SessionDataConsistOfSessionsEndPosAnd4BytesSessionsDataOffset) {
    BasicSerializationContext<SessionsEnabledConfig, void> ctx;


    constexpr size_t DATA_SIZE = 4;
    int32_t data{};

    auto& ser = ctx.createSerializer();
    ser.ext(data, Growable{}, [&ser](int32_t & v) { ser.value4b(v);});
    ctx.createDeserializer();//to flush data and create buffer reader

    EXPECT_THAT(ctx.getBufferSize(), Eq(1+4 + DATA_SIZE));

    //read value back
    auto& br = *(ctx.br);
    br.readBytes<DATA_SIZE>(data);

    size_t sessionEnd{};
    //there should start session data with first size of session
    bitsery::details::readSize(br, sessionEnd, 1000000u);
    EXPECT_THAT(sessionEnd, Eq(DATA_SIZE));
    //this is the the offset from the end of buffer where actual data ends
    uint32_t sessionsOffset{};//bufferEnd - sessionsOffset = dataEnd
    br.readBytes<4>(sessionsOffset);
    EXPECT_THAT(sessionsOffset, Eq(1+4));//1byte for session info, 4 bytes for session offset variable
    auto writtenSize = ctx.bw->writtenBytesCount();
    auto dSize = writtenSize - sessionsOffset;
    EXPECT_THAT(dSize, Eq(DATA_SIZE));
}

TEST(SerializeExtensionGrowable, WhenNestedSessionsThenStoreEachDepthAndSize) {
    BasicSerializationContext<SessionsEnabledConfig, void> ctx;
    DataV3 data{19457,846, 498418};
    ctx.createSerializer();
    ctx.bw->beginSession();
        ctx.bw->writeBytes<4>(data.v1);
        ctx.bw->beginSession();
            ctx.bw->writeBytes<4>(data.v2);
        ctx.bw->endSession();
        ctx.bw->beginSession();
            ctx.bw->writeBytes<4>(data.v3);
        ctx.bw->endSession();
    ctx.bw->endSession();
    DataV3 res{};
    ctx.createDeserializer();//to flush data and create buffer reader
    ctx.br->readBytes<4>(res.v1);
    ctx.br->readBytes<4>(res.v2);
    ctx.br->readBytes<4>(res.v3);
    //read data correctly
    EXPECT_THAT(res.v1, Eq(data.v1));
    EXPECT_THAT(res.v2, Eq(data.v2));
    EXPECT_THAT(res.v3, Eq(data.v3));
    size_t sessionEnd[3];
    //read sessions sizes
    bitsery::details::readSize(*(ctx.br), sessionEnd[0],10000000u);
    bitsery::details::readSize(*(ctx.br), sessionEnd[1],10000000u);
    bitsery::details::readSize(*(ctx.br), sessionEnd[2],10000000u);
    EXPECT_THAT(sessionEnd[0], Eq(12));
    EXPECT_THAT(sessionEnd[1], Eq(8));
    EXPECT_THAT(sessionEnd[2], Eq(12));
}

TEST(SerializeExtensionGrowable, MultipleSessionsReadSameVersionData) {
    BasicSerializationContext<SessionsEnabledConfig, void> ctx;
    DataV2 data{8454,987451};
    ctx.createSerializer();
    auto& bw = (*ctx.bw);
    for (auto i = 0; i < 10; ++i) {
        bw.beginSession();
        bw.writeBytes<4>(data.v1);
        bw.writeBytes<4>(data.v2);
        bw.endSession();
    }
    //create more sessions that can fit in 2 bytes
    ctx.createDeserializer();//to flush data and create buffer reader
    DataV2 res{};
    auto& br = (*ctx.br);
    for (auto i = 0; i < 10; ++i) {
        br.beginSession();
        br.readBytes<4>(res.v1);
        br.readBytes<4>(res.v2);
        br.endSession();
        EXPECT_THAT(res.v1, Eq(data.v1));
        EXPECT_THAT(res.v2, Eq(data.v2));
    }
    EXPECT_THAT(ctx.br->isCompletedSuccessfully(), Eq(true));
}

TEST(SerializeExtensionGrowable, MultipleSessionsReadNewerVersionData) {
    BasicSerializationContext<SessionsEnabledConfig, void> ctx;
    DataV3 data{8454,987451,54};
    ctx.createSerializer();
    auto& bw = (*ctx.bw);
    for (auto i = 0; i < 10; ++i) {
        bw.beginSession();
        bw.writeBytes<4>(data.v1);
        bw.writeBytes<4>(data.v2);
        bw.writeBytes<4>(data.v3);
        bw.endSession();
    }
    //create more sessions that can fit in 2 bytes
    ctx.createDeserializer();//to flush data and create buffer reader
    DataV2 res{};
    auto& br = (*ctx.br);
    for (auto i = 0; i < 10; ++i) {
        br.beginSession();
        br.readBytes<4>(res.v1);
        br.readBytes<4>(res.v2);
        br.endSession();
        EXPECT_THAT(res.v1, Eq(data.v1));
        EXPECT_THAT(res.v2, Eq(data.v2));
    }
    EXPECT_THAT(br.isCompletedSuccessfully(), Eq(true));
}

TEST(SerializeExtensionGrowable, MultipleSessionsReadOlderVersionData) {
    BasicSerializationContext<SessionsEnabledConfig, void> ctx;
    DataV2 data{8454,987451};
    ctx.createSerializer();
    auto& bw = (*ctx.bw);
    for (auto i = 0; i < 10; ++i) {
        bw.beginSession();
        bw.writeBytes<4>(data.v1);
        bw.writeBytes<4>(data.v2);
        bw.endSession();
    }
    //create more sessions that can fit in 2 bytes
    ctx.createDeserializer();//to flush data and create buffer reader
    DataV3 res{4798,657891,985};
    auto& br = (*ctx.br);
    for (auto i = 0; i < 10; ++i) {
        br.beginSession();
        br.readBytes<4>(res.v1);
        br.readBytes<4>(res.v2);
        br.readBytes<4>(res.v3);
        br.endSession();
        EXPECT_THAT(res.v1, Eq(data.v1));
        EXPECT_THAT(res.v2, Eq(data.v2));
        EXPECT_THAT(res.v3, Eq(0));
    }
    EXPECT_THAT(ctx.br->isCompletedSuccessfully(), Eq(true));
}

TEST(SerializeExtensionGrowable, MultipleNestedSessionsReadSameVersionData) {
    BasicSerializationContext<SessionsEnabledConfig, void> ctx;
    DataV2 data{8454,987451};
    ctx.createSerializer();
    auto& bw = (*ctx.bw);
    for (auto i = 0; i < 2; ++i) {
        bw.beginSession();
            bw.writeBytes<4>(data.v1);
            bw.writeBytes<4>(data.v2);
            bw.beginSession();
                bw.writeBytes<4>(data.v1);
                bw.writeBytes<4>(data.v2);
            bw.endSession();
        bw.endSession();
    }
    //create more sessions that can fit in 2 bytes
    ctx.createDeserializer();//to flush data and create buffer reader
    DataV2 res{};
    auto& br = (*ctx.br);
    for (auto i = 0; i < 2; ++i) {
        br.beginSession();
            br.readBytes<4>(res.v1);
            br.readBytes<4>(res.v2);
            EXPECT_THAT(res.v1, Eq(data.v1));
            EXPECT_THAT(res.v2, Eq(data.v2));
            br.beginSession();
                br.readBytes<4>(res.v1);
                br.readBytes<4>(res.v2);
                EXPECT_THAT(res.v1, Eq(data.v1));
                EXPECT_THAT(res.v2, Eq(data.v2));
            br.endSession();
        br.endSession();
    }
    EXPECT_THAT(ctx.br->isCompletedSuccessfully(), Eq(true));
}

TEST(SerializeExtensionGrowable, MultipleNestedSessionsReadOlderVersionData) {
    BasicSerializationContext<SessionsEnabledConfig, void> ctx;
    DataV2 data{8454,987451};
    ctx.createSerializer();
    auto& bw = (*ctx.bw);
    for (auto i = 0; i < 5; ++i) {
        bw.beginSession();
            bw.writeBytes<4>(data.v1);
        bw.endSession();
        bw.writeBytes<4>(data.v2);
    }

    //create more sessions that can fit in 2 bytes
    ctx.createDeserializer();//to flush data and create buffer reader
    DataV3 res{};
    auto& br = (*ctx.br);
    for (auto i = 0; i < 5; ++i) {
        br.beginSession();
            br.readBytes<4>(res.v1);
            EXPECT_THAT(res.v1, Eq(data.v1));
            //new flow
            br.beginSession();
                br.readBytes<4>(res.v3);
                EXPECT_THAT(res.v3, Eq(0));
                br.beginSession();
                    br.readBytes<4>(res.v3);
                    EXPECT_THAT(res.v3, Eq(0));
                br.endSession();
            br.endSession();
            br.beginSession();
                br.readBytes<4>(res.v3);
                EXPECT_THAT(res.v3, Eq(0));
            br.endSession();
        br.endSession();
        br.readBytes<4>(res.v2);
        EXPECT_THAT(res.v2, Eq(data.v2));
    }
    EXPECT_THAT(ctx.br->isCompletedSuccessfully(), Eq(true));
}

TEST(SerializeExtensionGrowable, MultipleNestedSessionsReadNewerVersionData1) {
    BasicSerializationContext<SessionsEnabledConfig, void> ctx;
    DataV3 data{8454,987451,54};
    ctx.createSerializer();
    auto& bw = (*ctx.bw);
    for (auto i = 0; i < 2; ++i) {
        bw.beginSession();
        {
            bw.writeBytes<4>(data.v1);
            bw.writeBytes<4>(data.v2);
            //new flow
            bw.beginSession();
            {
                bw.writeBytes<4>(data.v3);
            }
            bw.endSession();
            bw.beginSession();
            {
                bw.writeBytes<4>(data.v3);
                bw.beginSession();
                {
                    bw.beginSession();
                    {
                        bw.writeBytes<4>(data.v3);
                        bw.writeBytes<4>(data.v3);
                    }
                    bw.endSession();
                    bw.writeBytes<4>(data.v3);
                }
                bw.endSession();
            }
            bw.endSession();
            bw.writeBytes<4>(data.v3);
        }
        bw.endSession();
        bw.writeBytes<4>(data.v2);
    }
    //create more sessions that can fit in 2 bytes
    ctx.createDeserializer();//to flush data and create buffer reader
    DataV2 res{};
    auto& br = (*ctx.br);
    for (auto i = 0; i < 2; ++i) {
        br.beginSession();
        {
            br.readBytes<4>(res.v1);
            br.readBytes<4>(res.v2);
            EXPECT_THAT(res.v1, Eq(data.v1));
            EXPECT_THAT(res.v2, Eq(data.v2));
        }
        br.endSession();
        br.readBytes<4>(res.v2);
        EXPECT_THAT(res.v2, Eq(data.v2));
    }
    EXPECT_THAT(ctx.br->isCompletedSuccessfully(), Eq(true));
}

TEST(SerializeExtensionGrowable, MultipleNestedSessionsReadNewerVersionData2) {
    BasicSerializationContext<SessionsEnabledConfig, void> ctx;
    DataV3 data{8454,987451,54};
    ctx.createSerializer();
    auto& bw = (*ctx.bw);
    for (auto i = 0; i < 2; ++i) {
        bw.beginSession();
            bw.writeBytes<4>(data.v1);
            bw.writeBytes<4>(data.v2);
            bw.beginSession();
                bw.writeBytes<4>(data.v1);
                bw.writeBytes<4>(data.v2);

                //new flow
                bw.beginSession();
                    bw.writeBytes<4>(data.v3);
                bw.endSession();
                bw.beginSession();
                    bw.writeBytes<4>(data.v3);
                    bw.beginSession();
                        bw.beginSession();
                            bw.writeBytes<4>(data.v3);
                            bw.writeBytes<4>(data.v3);
                        bw.endSession();
                        bw.writeBytes<4>(data.v3);
                    bw.endSession();
                bw.endSession();
                bw.writeBytes<4>(data.v3);
            bw.endSession();
            bw.writeBytes<4>(data.v2);

            //new flow
            bw.writeBytes<4>(data.v3);
            bw.beginSession();
                bw.writeBytes<4>(data.v3);
            bw.endSession();
            bw.writeBytes<4>(data.v3);
        bw.endSession();
    }
    //create more sessions that can fit in 2 bytes
    ctx.createDeserializer();//to flush data and create buffer reader
    DataV2 res{};
    auto& br = (*ctx.br);
    for (auto i = 0; i < 2; ++i) {
        br.beginSession();
            br.readBytes<4>(res.v1);
            br.readBytes<4>(res.v2);
            EXPECT_THAT(res.v1, Eq(data.v1));
            EXPECT_THAT(res.v2, Eq(data.v2));
            br.beginSession();
                br.readBytes<4>(res.v1);
                br.readBytes<4>(res.v2);
                EXPECT_THAT(res.v1, Eq(data.v1));
                EXPECT_THAT(res.v2, Eq(data.v2));
            br.endSession();
            br.readBytes<4>(res.v2);
            EXPECT_THAT(res.v2, Eq(data.v2));
        br.endSession();
    }
    EXPECT_THAT(ctx.br->isCompletedSuccessfully(), Eq(true));
}

TEST(SerializeExtensionGrowable, SessionsStartsAtEndOfSerialization) {
    BasicSerializationContext<SessionsEnabledConfig, void> ctx;
    DataV2 data{8454,987451};
    ctx.createSerializer();
    auto& bw = (*ctx.bw);
    for (auto i = 0; i < 100; ++i)
        bw.writeBytes<4>(data.v1);
    for (auto i = 0; i < 10; ++i) {
        bw.beginSession();
        bw.writeBytes<4>(data.v1);
        bw.writeBytes<4>(data.v2);
        bw.endSession();
    }
    //create more sessions that can fit in 2 bytes
    ctx.createDeserializer();//to flush data and create buffer reader
    DataV2 res{};
    auto& br = (*ctx.br);
    for (auto i = 0; i < 100; ++i)
        br.readBytes<4>(res.v1);
    for (auto i = 0; i < 10; ++i) {
        br.beginSession();
        br.readBytes<4>(res.v1);
        br.readBytes<4>(res.v2);
        br.endSession();
        EXPECT_THAT(res.v1, Eq(data.v1));
        EXPECT_THAT(res.v2, Eq(data.v2));
    }
    EXPECT_THAT(ctx.br->isCompletedSuccessfully(), Eq(true));
}
