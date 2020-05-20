#pragma once
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


using Tools::getCurrentTimeMillis;
//using Tools::readData;



enum MsgType : uint8_t {
    test_req = 1,
    test_rsp = 2,
    rtt_test_msg =3,
};

enum TestType : uint8_t {
  rtt = 1,
  bandwidth = 2,
};

class Message {

 protected:
  constexpr uint16_t headLen() const { return 15; }


 public:
  uint8_t msg_type;
  uint16_t test_id;
  uint32_t msg_id;
  uint64_t timestamp;

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

  //virtual size_t getLength() const = 0;

  //virtual void getBinary(uint8_t* buf, size_t size) const = 0;

  MsgType getMsgType() { return (MsgType)msg_type; }

  uint16_t getTestId() { return test_id; }

  uint16_t getMsgId() { return msg_id; }

  uint64_t getTimeStamp() { return timestamp; }

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

  //size_t getLength() const override { return headLen() + 8; }

  //void getBinary(uint8_t* buf, size_t size) const override {
  //  if (size < getLength()) {
  //    throw std::runtime_error("buf too small.");
  //  }

  //  writeHead(buf);
  //  Tools::writeData(buf + 15, test_type);
  //  Tools::writeData(buf + 19, test_time);
  //}
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

  //size_t getLength() const override { return headLen() + 8; }

  //void getBinary(uint8_t* buf, size_t size) const override {
  //  if (size < getLength()) {
  //    throw std::runtime_error("buf too small.");
  //  }

  //  writeHead(buf);
  //  Tools::writeData(buf + 15, result);
  //  Tools::writeData(buf + 19, reMsgId);
  //}
};


class RttTestMsg : public Message {
 public:
  int payloadLen; 

  static const MsgType msgType = MsgType::rtt_test_msg;


  RttTestMsg(uint8_t* data) : Message(data) { Tools::readData(data + 15, payloadLen);
  }

  RttTestMsg(int packetSize, int testId, int mid)
      : Message(msgType, testId, mid), payloadLen(packetSize - 15) {}

  //size_t getLength() const override { return headLen() + (size_t)payloadLen; }

  //void getBinary(uint8_t* buf, size_t size) const override {
  //  if (size < getLength()) {
  //    throw std::runtime_error("buf too small.");
  //  }

  //  writeHead(buf);
  //  Tools::writeData(buf + 15, payloadLen);
  //  for (int i = 0; i < payloadLen - 4; i++) {
  //    Tools::writeData(buf + 19 + i, (uint8_t)i);
  //  }
  //}
};
