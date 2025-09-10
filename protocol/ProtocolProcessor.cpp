#include "ProtocolProcessor.h"

#include <algorithm>
#include <cstring>

#include "messages/Backend2Master.h"
#include "messages/Master2Backend.h"
#include "messages/Master2Slave.h"
#include "messages/Slave2Backend.h"
#include "messages/Slave2Master.h"

namespace WhtsProtocol {

// ProtocolProcessor 实现
ProtocolProcessor::ProtocolProcessor() : mtu_(DEFAULT_MTU) {}
ProtocolProcessor::~ProtocolProcessor() {}

void ProtocolProcessor::writeUint16LE(std::vector<uint8_t> &buffer,
                                      uint16_t value) {
    buffer.push_back(value & 0xFF);
    buffer.push_back((value >> 8) & 0xFF);
}

void ProtocolProcessor::writeUint32LE(std::vector<uint8_t> &buffer,
                                      uint32_t value) {
    buffer.push_back(value & 0xFF);
    buffer.push_back((value >> 8) & 0xFF);
    buffer.push_back((value >> 16) & 0xFF);
    buffer.push_back((value >> 24) & 0xFF);
}

uint16_t ProtocolProcessor::readUint16LE(const std::vector<uint8_t> &buffer,
                                         size_t offset) {
    if (offset + 1 >= buffer.size()) return 0;
    return buffer[offset] | (buffer[offset + 1] << 8);
}

uint32_t ProtocolProcessor::readUint32LE(const std::vector<uint8_t> &buffer,
                                         size_t offset) {
    if (offset + 3 >= buffer.size()) return 0;
    return buffer[offset] | (buffer[offset + 1] << 8) |
           (buffer[offset + 2] << 16) | (buffer[offset + 3] << 24);
}

std::vector<uint8_t> ProtocolProcessor::packMaster2SlaveMessageSingle(
    uint32_t destinationId, const Message &message, uint8_t fragmentsSequence,
    uint8_t moreFragmentsFlag) {
    Frame frame;
    frame.packetId = static_cast<uint8_t>(PacketId::MASTER_TO_SLAVE);
    frame.fragmentsSequence = fragmentsSequence;
    frame.moreFragmentsFlag = moreFragmentsFlag;

    // 构建载荷
    std::vector<uint8_t> payload;
    payload.push_back(message.getMessageId());
    writeUint32LE(payload, destinationId);

    auto messageData = message.serialize();
    payload.insert(payload.end(), messageData.begin(), messageData.end());

    frame.payload = payload;
    frame.packetLength = static_cast<uint16_t>(payload.size());

    return frame.serialize();
}

std::vector<uint8_t> ProtocolProcessor::packSlave2MasterMessageSingle(
    uint32_t slaveId, const Message &message, uint8_t fragmentsSequence,
    uint8_t moreFragmentsFlag) {
    Frame frame;
    frame.packetId = static_cast<uint8_t>(PacketId::SLAVE_TO_MASTER);
    frame.fragmentsSequence = fragmentsSequence;
    frame.moreFragmentsFlag = moreFragmentsFlag;

    // 构建载荷
    std::vector<uint8_t> payload;
    payload.push_back(message.getMessageId());
    writeUint32LE(payload, slaveId);

    auto messageData = message.serialize();
    payload.insert(payload.end(), messageData.begin(), messageData.end());

    frame.payload = payload;
    frame.packetLength = static_cast<uint16_t>(payload.size());

    return frame.serialize();
}

std::vector<uint8_t> ProtocolProcessor::packSlave2BackendMessageSingle(
    uint32_t slaveId, const DeviceStatus &deviceStatus, const Message &message,
    uint8_t fragmentsSequence, uint8_t moreFragmentsFlag) {
    Frame frame;
    frame.packetId = static_cast<uint8_t>(PacketId::SLAVE_TO_BACKEND);
    frame.fragmentsSequence = fragmentsSequence;
    frame.moreFragmentsFlag = moreFragmentsFlag;

    // 构建载荷
    std::vector<uint8_t> payload;
    payload.push_back(message.getMessageId());
    writeUint32LE(payload, slaveId);
    writeUint16LE(payload, deviceStatus.toUint16());

    auto messageData = message.serialize();
    payload.insert(payload.end(), messageData.begin(), messageData.end());

    frame.payload = payload;
    frame.packetLength = static_cast<uint16_t>(payload.size());

    return frame.serialize();
}

std::vector<uint8_t> ProtocolProcessor::packBackend2MasterMessageSingle(
    const Message &message, uint8_t fragmentsSequence,
    uint8_t moreFragmentsFlag) {
    Frame frame;
    frame.packetId = static_cast<uint8_t>(PacketId::BACKEND_TO_MASTER);
    frame.fragmentsSequence = fragmentsSequence;
    frame.moreFragmentsFlag = moreFragmentsFlag;

    // 构建载荷
    std::vector<uint8_t> payload;
    payload.push_back(message.getMessageId());

    auto messageData = message.serialize();
    payload.insert(payload.end(), messageData.begin(), messageData.end());

    frame.payload = payload;
    frame.packetLength = static_cast<uint16_t>(payload.size());

    return frame.serialize();
}

std::vector<uint8_t> ProtocolProcessor::packMaster2BackendMessageSingle(
    const Message &message, uint8_t fragmentsSequence,
    uint8_t moreFragmentsFlag) {
    Frame frame;
    frame.packetId = static_cast<uint8_t>(PacketId::MASTER_TO_BACKEND);
    frame.fragmentsSequence = fragmentsSequence;
    frame.moreFragmentsFlag = moreFragmentsFlag;

    // 构建载荷
    std::vector<uint8_t> payload;
    payload.push_back(message.getMessageId());

    auto messageData = message.serialize();
    payload.insert(payload.end(), messageData.begin(), messageData.end());

    frame.payload = payload;
    frame.packetLength = static_cast<uint16_t>(payload.size());

    return frame.serialize();
}

bool ProtocolProcessor::parseFrame(const std::vector<uint8_t> &data,
                                   Frame &frame) {
    return Frame::deserialize(data, frame);
}

std::unique_ptr<Message> ProtocolProcessor::createMessage(PacketId packetId,
                                                          uint8_t messageId) {
    switch (packetId) {
        case PacketId::MASTER_TO_SLAVE:
            switch (static_cast<Master2SlaveMessageId>(messageId)) {
                case Master2SlaveMessageId::SYNC_MSG:
                    return std::make_unique<Master2Slave::SyncMessage>();
                case Master2SlaveMessageId::PING_REQ_MSG:
                    return std::make_unique<Master2Slave::PingReqMessage>();
                case Master2SlaveMessageId::SHORT_ID_ASSIGN_MSG:
                    return std::make_unique<
                        Master2Slave::ShortIdAssignMessage>();
                default:
                    break;
            }
            break;

        case PacketId::SLAVE_TO_MASTER:
            switch (static_cast<Slave2MasterMessageId>(messageId)) {
                case Slave2MasterMessageId::RST_RSP_MSG:
                    return std::make_unique<Slave2Master::RstResponseMessage>();
                case Slave2MasterMessageId::PING_RSP_MSG:
                    return std::make_unique<Slave2Master::PingRspMessage>();
                case Slave2MasterMessageId::ANNOUNCE_MSG:
                    return std::make_unique<Slave2Master::JoinRequestMessage>();
                case Slave2MasterMessageId::SHORT_ID_CONFIRM_MSG:
                    return std::make_unique<
                        Slave2Master::ShortIdConfirmMessage>();
                case Slave2MasterMessageId::HEARTBEAT_MSG:
                    return std::make_unique<Slave2Master::HeartbeatMessage>();
                default:
                    break;
            }
            break;

        case PacketId::SLAVE_TO_BACKEND:
            switch (static_cast<Slave2BackendMessageId>(messageId)) {
                case Slave2BackendMessageId::CONDUCTION_DATA_MSG:
                    return std::make_unique<
                        Slave2Backend::ConductionDataMessage>();
                case Slave2BackendMessageId::RESISTANCE_DATA_MSG:
                    return std::make_unique<
                        Slave2Backend::ResistanceDataMessage>();
                case Slave2BackendMessageId::CLIP_DATA_MSG:
                    return std::make_unique<Slave2Backend::ClipDataMessage>();
            }
            break;

        case PacketId::BACKEND_TO_MASTER:
            switch (static_cast<Backend2MasterMessageId>(messageId)) {
                case Backend2MasterMessageId::SLAVE_CFG_MSG:
                    return std::make_unique<
                        Backend2Master::SlaveConfigMessage>();
                case Backend2MasterMessageId::MODE_CFG_MSG:
                    return std::make_unique<
                        Backend2Master::ModeConfigMessage>();
                case Backend2MasterMessageId::SLAVE_RST_MSG:
                    return std::make_unique<Backend2Master::RstMessage>();
                case Backend2MasterMessageId::CTRL_MSG:
                    return std::make_unique<Backend2Master::CtrlMessage>();
                case Backend2MasterMessageId::PING_CTRL_MSG:
                    return std::make_unique<Backend2Master::PingCtrlMessage>();
                case Backend2MasterMessageId::DEVICE_LIST_REQ_MSG:
                    return std::make_unique<
                        Backend2Master::DeviceListReqMessage>();
                case Backend2MasterMessageId::INTERVAL_CFG_MSG:
                    return std::make_unique<
                        Backend2Master::IntervalConfigMessage>();
                case Backend2MasterMessageId::CLEAR_DEVICE_LIST_MSG:
                    return std::make_unique<
                        Backend2Master::ClearDeviceListMessage>();
                case Backend2MasterMessageId::SET_UWB_CHAN_MSG:
                    return std::make_unique<
                        Backend2Master::SetUwbChannelMessage>();
            }
            break;

        case PacketId::MASTER_TO_BACKEND:
            switch (static_cast<Master2BackendMessageId>(messageId)) {
                case Master2BackendMessageId::SLAVE_CFG_RSP_MSG:
                    return std::make_unique<
                        Master2Backend::SlaveConfigResponseMessage>();
                case Master2BackendMessageId::MODE_CFG_RSP_MSG:
                    return std::make_unique<
                        Master2Backend::ModeConfigResponseMessage>();
                case Master2BackendMessageId::RST_RSP_MSG:
                    return std::make_unique<
                        Master2Backend::RstResponseMessage>();
                case Master2BackendMessageId::CTRL_RSP_MSG:
                    return std::make_unique<
                        Master2Backend::CtrlResponseMessage>();
                case Master2BackendMessageId::PING_RES_MSG:
                    return std::make_unique<
                        Master2Backend::PingResponseMessage>();
                case Master2BackendMessageId::DEVICE_LIST_RSP_MSG:
                    return std::make_unique<
                        Master2Backend::DeviceListResponseMessage>();
                case Master2BackendMessageId::INTERVAL_CFG_RSP_MSG:
                    return std::make_unique<
                        Master2Backend::IntervalConfigResponseMessage>();
                case Master2BackendMessageId::SET_UWB_CHAN_RSP_MSG:
                    return std::make_unique<
                        Master2Backend::SetUwbChannelResponseMessage>();
            }
            break;

        default:
            break;
    }
    return nullptr;
}

bool ProtocolProcessor::parseMaster2SlavePacket(
    const std::vector<uint8_t> &payload, uint32_t &destinationId,
    std::unique_ptr<Message> &message) {
    if (payload.size() < 5) return false;

    uint8_t messageId = payload[0];
    destinationId = readUint32LE(payload, 1);

    message = createMessage(PacketId::MASTER_TO_SLAVE, messageId);
    if (!message) return false;

    std::vector<uint8_t> messageData(payload.begin() + 5, payload.end());
    return message->deserialize(messageData);
}

bool ProtocolProcessor::parseSlave2MasterPacket(
    const std::vector<uint8_t> &payload, uint32_t &slaveId,
    std::unique_ptr<Message> &message) {
    if (payload.size() < 5) return false;

    uint8_t messageId = payload[0];
    slaveId = readUint32LE(payload, 1);

    message = createMessage(PacketId::SLAVE_TO_MASTER, messageId);
    if (!message) return false;

    std::vector<uint8_t> messageData(payload.begin() + 5, payload.end());
    return message->deserialize(messageData);
}

bool ProtocolProcessor::parseSlave2BackendPacket(
    const std::vector<uint8_t> &payload, uint32_t &slaveId,
    DeviceStatus &deviceStatus, std::unique_ptr<Message> &message) {
    if (payload.size() < 7) return false;

    uint8_t messageId = payload[0];
    slaveId = readUint32LE(payload, 1);
    deviceStatus.fromUint16(readUint16LE(payload, 5));

    message = createMessage(PacketId::SLAVE_TO_BACKEND, messageId);
    if (!message) return false;

    std::vector<uint8_t> messageData(payload.begin() + 7, payload.end());
    return message->deserialize(messageData);
}

bool ProtocolProcessor::parseBackend2MasterPacket(
    const std::vector<uint8_t> &payload, std::unique_ptr<Message> &message) {
    if (payload.size() < 1) return false;

    uint8_t messageId = payload[0];

    message = createMessage(PacketId::BACKEND_TO_MASTER, messageId);
    if (!message) return false;

    std::vector<uint8_t> messageData(payload.begin() + 1, payload.end());
    return message->deserialize(messageData);
}

bool ProtocolProcessor::parseMaster2BackendPacket(
    const std::vector<uint8_t> &payload, std::unique_ptr<Message> &message) {
    if (payload.size() < 1) return false;

    uint8_t messageId = payload[0];

    message = createMessage(PacketId::MASTER_TO_BACKEND, messageId);
    if (!message) return false;

    std::vector<uint8_t> messageData(payload.begin() + 1, payload.end());
    return message->deserialize(messageData);
}

// 支持自动分片的打包函数
std::vector<std::vector<uint8_t>> ProtocolProcessor::packMaster2SlaveMessage(
    uint32_t destinationId, const Message &message) {
    // 首先生成单个完整帧
    auto completeFrame =
        packMaster2SlaveMessageSingle(destinationId, message, 0, 0);

    // 检查是否需要分片
    if (completeFrame.size() <= mtu_) {
        // 不需要分片，直接返回
        return {completeFrame};
    }

    // 需要分片
    return fragmentFrame(completeFrame);
}

std::vector<std::vector<uint8_t>> ProtocolProcessor::packSlave2MasterMessage(
    uint32_t slaveId, const Message &message) {
    // 首先生成单个完整帧
    auto completeFrame = packSlave2MasterMessageSingle(slaveId, message, 0, 0);

    // 检查是否需要分片
    if (completeFrame.size() <= mtu_) {
        // 不需要分片，直接返回
        return {completeFrame};
    }

    // 需要分片
    return fragmentFrame(completeFrame);
}

std::vector<std::vector<uint8_t>> ProtocolProcessor::packSlave2BackendMessage(
    uint32_t slaveId, const DeviceStatus &deviceStatus,
    const Message &message) {
    // 首先生成单个完整帧
    auto completeFrame =
        packSlave2BackendMessageSingle(slaveId, deviceStatus, message, 0, 0);

    // 检查是否需要分片
    if (completeFrame.size() <= mtu_) {
        // 不需要分片，直接返回
        return {completeFrame};
    }

    // 需要分片
    return fragmentFrame(completeFrame);
}

std::vector<std::vector<uint8_t>> ProtocolProcessor::packBackend2MasterMessage(
    const Message &message) {
    // 首先生成单个完整帧
    auto completeFrame = packBackend2MasterMessageSingle(message, 0, 0);

    // 检查是否需要分片
    if (completeFrame.size() <= mtu_) {
        // 不需要分片，直接返回
        return {completeFrame};
    }

    // 需要分片
    return fragmentFrame(completeFrame);
}

std::vector<std::vector<uint8_t>> ProtocolProcessor::packMaster2BackendMessage(
    const Message &message) {
    // 首先生成单个完整帧
    auto completeFrame = packMaster2BackendMessageSingle(message, 0, 0);

    // 检查是否需要分片
    if (completeFrame.size() <= mtu_) {
        // 不需要分片，直接返回
        return {completeFrame};
    }

    // 需要分片
    return fragmentFrame(completeFrame);
}

// 分片功能实现
std::vector<std::vector<uint8_t>> ProtocolProcessor::fragmentFrame(
    const std::vector<uint8_t> &frameData) {
    std::vector<std::vector<uint8_t>> fragments;



    if (frameData.size() < 7) {    // Minimum frame header is 7 bytes

        return {frameData};
    }

    // Parse original frame header
    uint8_t packetId = frameData[2];

    // Calculate effective payload size per fragment (MTU - 7 bytes frame
    // header)
    size_t fragmentPayloadSize = mtu_ - 7;

    // Get original payload (starting from 7th byte)
    std::vector<uint8_t> originalPayload(frameData.begin() + 7,
                                         frameData.end());

    // Calculate how many fragments are needed
    uint8_t totalFragments = static_cast<uint8_t>(
        (originalPayload.size() + fragmentPayloadSize - 1) /
        fragmentPayloadSize);

    // Generate each fragment
    for (uint8_t i = 0; i < totalFragments; ++i) {
        std::vector<uint8_t> fragment;

        // Add frame header
        fragment.push_back(FRAME_DELIMITER_1);
        fragment.push_back(FRAME_DELIMITER_2);
        fragment.push_back(packetId);
        fragment.push_back(i);    // Fragment sequence number
        fragment.push_back(
            (i == totalFragments - 1) ? 0 : 1);    // moreFragmentsFlag

        // Calculate payload for this fragment
        size_t startPos = i * fragmentPayloadSize;
        size_t endPos =
            std::min(startPos + fragmentPayloadSize, originalPayload.size());
        size_t fragmentSize = endPos - startPos;

        // Add length field
        fragment.push_back(fragmentSize & 0xFF);
        fragment.push_back((fragmentSize >> 8) & 0xFF);

        // Add fragment payload
        fragment.insert(fragment.end(), originalPayload.begin() + startPos,
                        originalPayload.begin() + endPos);

        fragments.push_back(fragment);
    }


    return fragments;
}

// Process received raw data (supports packet concatenation handling)
void ProtocolProcessor::processReceivedData(const std::vector<uint8_t> &data) {
    // elog_v("ProtocolProcessor",
    //        "Received new data, size: %d bytes, prefix: %s", data.size(),
    //        bytesToHexString(data, 8).c_str());

    // Prevent receive buffer from becoming too large
    if (receiveBuffer_.size() + data.size() > MAX_RECEIVE_BUFFER_SIZE) {
        receiveBuffer_.clear();
    }

    // Add new data to receive buffer
    receiveBuffer_.insert(receiveBuffer_.end(), data.begin(), data.end());

    // Try to extract complete frames from buffer
    bool framesExtracted = extractCompleteFrames();

    // Clean up expired fragments
    cleanupExpiredFragments();
}

// Extract complete frames from receive buffer
bool ProtocolProcessor::extractCompleteFrames() {
    bool foundFrames = false;
    size_t pos = 0;


    while (pos < receiveBuffer_.size()) {
        // Find frame header
        size_t frameStart = findFrameHeader(receiveBuffer_, pos);
        if (frameStart == SIZE_MAX) {
            break;    // No frame header found
        }


        // Check if there's enough data to read frame length
        if (frameStart + 7 > receiveBuffer_.size()) {
            break;    // Not enough data, wait for more
        }

        // 读取帧长度
        uint16_t frameLength = readUint16LE(receiveBuffer_, frameStart + 5);
        size_t totalFrameSize = 7 + frameLength;


        // 检查是否有完整的帧
        if (frameStart + totalFrameSize > receiveBuffer_.size()) {
            break;    // 帧不完整，等待更多数据
        }

        // 提取完整帧数据
        std::vector<uint8_t> frameData(
            receiveBuffer_.begin() + frameStart,
            receiveBuffer_.begin() + frameStart + totalFrameSize);

        // elog_v(
        //     "ProtocolProcessor",
        //     "Extracted complete frame data, size: %d bytes, frame prefix:
        //     %s", frameData.size(), bytesToHexString(frameData, 16).c_str());

        // 解析帧
        Frame frame;
        if (Frame::deserialize(frameData, frame)) {

            // 检查是否是分片
            if (frame.moreFragmentsFlag || frame.fragmentsSequence > 0) {
                // 处理分片重组
                std::vector<uint8_t> completeFrame;
                if (reassembleFragments(frame, completeFrame)) {
                    // 分片重组完成，解析完整帧
                    Frame completedFrame;
                    if (Frame::deserialize(completeFrame, completedFrame)) {
                        completeFrames_.push(completedFrame);
                        foundFrames = true;
                    } else {
                    }
                } else {
                }
            } else {
                // 单个完整帧
                completeFrames_.push(frame);
                foundFrames = true;
            }
        } else {
        }

        // 移动到下一个位置
        pos = frameStart + totalFrameSize;
    }

    // 清理已处理的数据
    if (pos > 0) {
        receiveBuffer_.erase(receiveBuffer_.begin(),
                             receiveBuffer_.begin() + pos);
    }

    return foundFrames;
}

// 查找帧头
size_t ProtocolProcessor::findFrameHeader(const std::vector<uint8_t> &buffer,
                                          size_t startPos) {
    for (size_t i = startPos; i < buffer.size() - 1; ++i) {
        if (buffer[i] == FRAME_DELIMITER_1 &&
            buffer[i + 1] == FRAME_DELIMITER_2) {
            return i;
        }
    }
    return SIZE_MAX;
}

// 分片重组
bool ProtocolProcessor::reassembleFragments(
    const Frame &frame, std::vector<uint8_t> &completeFrame) {

    // 从帧载荷中提取源ID (假设载荷格式为: MessageId + SourceId + ...)
    if (frame.payload.size() < 5) {

        return false;    // 载荷太小
    }

    uint8_t messageId = frame.payload[0];
    uint32_t sourceId = readUint32LE(frame.payload, 1);    // 跳过MessageId
    uint64_t fragmentId = generateFragmentId(frame.packetId);


    // 查找或创建分片信息
    auto &fragmentInfo = fragmentMap_[fragmentId];
    fragmentInfo.packetId = frame.packetId;
    // fragmentInfo.timestamp = hal_hptimer_get_us64();

    // 存储分片数据
    fragmentInfo.fragments[frame.fragmentsSequence] = frame.payload;


    // 如果这是最后一个分片，计算总分片数
    if (frame.moreFragmentsFlag == 0) {
        fragmentInfo.totalFragments = frame.fragmentsSequence + 1;

    }

    // 检查是否收集完所有分片
    if (fragmentInfo.totalFragments > 0 && fragmentInfo.isComplete()) {


        // 重组完整载荷
        std::vector<uint8_t> completePayload;

        // First add the complete payload of the first fragment
        auto firstFragment = fragmentInfo.fragments.find(0);
        if (firstFragment != fragmentInfo.fragments.end()) {

        } else {

            return false;
        }

        // Then add the data portion of subsequent fragments
        for (uint8_t i = 1; i < fragmentInfo.totalFragments; ++i) {
            auto it = fragmentInfo.fragments.find(i);
            if (it != fragmentInfo.fragments.end()) {
                completePayload.insert(completePayload.end(),
                                       it->second.begin(), it->second.end());
            }
        }


        // Build complete frame
        completeFrame.clear();
        completeFrame.push_back(FRAME_DELIMITER_1);
        completeFrame.push_back(FRAME_DELIMITER_2);
        completeFrame.push_back(frame.packetId);
        completeFrame.push_back(0);    // fragmentsSequence = 0
        completeFrame.push_back(0);    // moreFragmentsFlag = 0

        // Add payload length
        uint16_t payloadLength = static_cast<uint16_t>(completePayload.size());
        completeFrame.push_back(payloadLength & 0xFF);
        completeFrame.push_back((payloadLength >> 8) & 0xFF);



        // Add payload
        completeFrame.insert(completeFrame.end(), completePayload.begin(),
                             completePayload.end());

        // Clean up fragment information
        fragmentMap_.erase(fragmentId);


        return true;
    }

    return false;    // Haven't collected all fragments yet
}

// Get next complete frame
bool ProtocolProcessor::getNextCompleteFrame(Frame &frame) {
    if (completeFrames_.empty()) {
        return false;
    }

    frame = completeFrames_.front();
    completeFrames_.pop();
    return true;
}

// Clear receive buffer
void ProtocolProcessor::clearReceiveBuffer() {
    receiveBuffer_.clear();
    while (!completeFrames_.empty()) {
        completeFrames_.pop();
    }
    fragmentMap_.clear();
}

// Generate fragment ID
uint64_t ProtocolProcessor::generateFragmentId(uint8_t packetId) {
    return packetId;
}

// Clean up expired fragments
void ProtocolProcessor::cleanupExpiredFragments() {
    // Fragment timeout cleanup is currently disabled
    // This function can be implemented to clean up expired fragments
    // based on timestamp if needed in the future
}

}    // namespace WhtsProtocol
