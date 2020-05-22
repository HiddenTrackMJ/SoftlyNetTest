/**
  @author Hidden Track
  @since 2020/05/19
  @time: 17:15
 */

#pragma once
#include <config.h>
#include <iostream>
#include <Tools.h>
#include <atomic>
#include <string>


using Tools::getCurrentTimeMillis;
//using Tools::readData;



enum MsgType : uint8_t {
  test_req = 1,
  test_rsp = 2,
  rtt_test_msg = 3,
  bw_test_msg = 4,
  bw_finish = 5,
  bw_report = 6,
};

enum TestType : uint8_t {
  rtt = 1,
  bandwidth = 2,
};

class Message {

 protected:
  uint8_t msg_type;
  uint16_t test_id;
  uint32_t msg_id;
  uint64_t timestamp;
  constexpr uint16_t headLen() const { return 15; }


 public:


  Message() = delete;

  Message(uint8_t* data) { setHead(data); }

  Message(enum MsgType m_t, uint16_t t_id, int m_id)
      : msg_type(m_t), test_id(t_id), msg_id(m_id), timestamp(Tools::getCurrentTimeMicro()) {}

  void writeHead(uint8_t* data) const {
    using Tools::writeData;
    writeData(data, msg_type);
    writeData(data + 1, test_id);
    writeData(data + 3, msg_id);
    writeData(data + 7, timestamp);
  }

   void setHead(uint8_t* data) {
    using Tools::readData;
    readData(data, msg_type);
    readData(data + 1, test_id);
    readData(data + 3, msg_id);
    readData(data + 7, timestamp);
  }

  MsgType get_msg_type() { return (MsgType)msg_type; }

  uint16_t get_test_id() { return test_id; }

  uint16_t get_msg_id() { return msg_id; }

  uint64_t get_timestamp() { return timestamp; }

  static int genMid() {
    static std::atomic<int> nextMid{1000};
    return nextMid.fetch_add(1);
  }



};


class TestReq : public Message {
 public:
  int test_type; 
  int test_time; 

  static const MsgType msgType = MsgType::test_req;

  TestReq() = delete;

  TestReq(uint8_t* data) : Message(data) {
    Tools::readData(data + 15, test_type);
    Tools::readData(data + 19, test_time);
  }

  TestReq(enum TestType tType, int tTime, int mid)
      : Message(msgType, 0, mid), test_type((int)tType), test_time(tTime) {}

  size_t getLength() { return headLen() + 8; }

   void writeIntoBuf(uint8_t* buf) {
 /*   if (sizeof(buf) < getLength()) {
      throw std::runtime_error("buf too small.");
    }*/

    writeHead(buf);
    Tools::writeData(buf + 15, test_type);
    Tools::writeData(buf + 19, test_time);
  }

};


class TestRsp : public Message {
 public:
  int result;   
  int reMsgId;  

  static const MsgType msgType = MsgType::test_rsp;

  TestRsp() = default;

  TestRsp(uint8_t* data) : Message(data) {
    Tools::readData(data + 15, result);
    Tools::readData(data + 19, reMsgId);
  }

  TestRsp(int rst, int tId, int reId, int mid)
      : Message(msgType, tId, mid), result(rst), reMsgId(reId) {}

};


class RttTestMsg : public Message {
 public:
  int payloadLen; 

  static const MsgType msgType = MsgType::rtt_test_msg;


  RttTestMsg(uint8_t* data) : Message(data) { Tools::readData(data + 15, payloadLen);
  }

  RttTestMsg(int packetSize, int testId, int mid)
      : Message(msgType, testId, mid), payloadLen(packetSize - 15) {}

  void writeData(uint8_t* data) {
    writeHead(data);
    Tools::writeData(data + 15, payloadLen);
  }
};

class BwTestMsg : public Message {
 public:

  int payloadLen;        //                       index: 15.
  int testPacketNumber;  //                       index: 19.

  static const MsgType msgType = MsgType::bw_test_msg;

  BwTestMsg(uint8_t* data) : Message(data) {
    writeHead(data);
    Tools::readData(data + 15, payloadLen);
    Tools::readData(data + 19, testPacketNumber);
  }

  static void update(uint8_t* data, const int& mId, const int& testNum, const int64_t& ts) {
    Tools::writeData(data + 3, mId);
    Tools::writeData(data + 7, ts);
    Tools::writeData(data + 19, testNum);
  }

  static int getTestNum(uint8_t* data) {
    int testNum;
    Tools::readData(data + 19, testNum);
    return testNum;
  }

  BwTestMsg(int packetSize, int testId, int testNumber, int mid)
      : Message(msgType, testId, mid),
        payloadLen(packetSize - 15),
        testPacketNumber(testNumber) {}

  size_t getLength() { return headLen() + (size_t)payloadLen; }

  void writeData(uint8_t* data) {
    writeHead(data);
    Tools::writeData(data + 15, payloadLen);
    Tools::writeData(data + 19, testPacketNumber);
    for (int i = 0; i < payloadLen - 8; i++) {
      Tools::writeData(data + 23 + i, (uint8_t)i);
    }
  }

};

class BwFinishMsg : public Message {
 public:
  int totalTestNum;

  static const MsgType msgType = MsgType::bw_finish;

  BwFinishMsg(uint8_t* data) : Message(data) { Tools::readData(data + 15, totalTestNum); }

  BwFinishMsg(int testId, int totalNum, int mid)
      : Message(msgType, testId, mid), totalTestNum(totalNum) {}

  size_t getLength() { return headLen() + 4; }

  void writeData(uint8_t* data, int len) {
    writeHead(data);
    Tools::writeData(data + 15, totalTestNum);
  }
};

class BandWidthReport : public Message {
 private:
  int jitterMicroSec;     //                   index: 15.
  int receivedPkt;        //                   index: 19.
  uint64_t transferByte;  //                   index: 23.

  static const MsgType msgType = MsgType::bw_report;

 public:
  BandWidthReport(int m_jitterMicroSec, int m_receivedPkt, uint64_t m_transferByte,
           uint64_t m_timeStamp, uint16_t m_test_id)
      : Message(msgType, m_timeStamp, m_test_id),
        jitterMicroSec(m_jitterMicroSec),
        receivedPkt(m_receivedPkt),
        transferByte(m_transferByte) {}

  BandWidthReport(uint8_t* data) : Message(data) {
    Tools::readData(data + 15, jitterMicroSec);
    Tools::readData(data + 19, receivedPkt);
    Tools::readData(data + 23, transferByte);
  }
  BandWidthReport() = delete;

  int getJitterMicroSec() const { return jitterMicroSec; }

  int getReceivedPkt() const { return receivedPkt; }

  uint64_t getTransferByte() const { return transferByte; }

  size_t getLength() { return headLen() + 16; }

  void writeData(uint8_t* data) {
    writeHead(data);
    Tools::writeData(data + 15, jitterMicroSec);
    Tools::writeData(data + 19, receivedPkt);
    Tools::writeData(data + 23, transferByte);
  }

};

