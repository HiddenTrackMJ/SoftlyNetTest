/**
  @author Hidden Track
  @since 2020/05/18
  @time: 14:25
 */
#pragma once


#ifdef WIN32
#include <chrono>
#include <thread>
#else
#include <sys/time.h>
#include <unistd.h>
#endif  // WIN32

#include <iostream>

namespace Tools {
static uint64_t getCurrentTimeMillis() {
#ifdef WIN32
  auto now = std::chrono::system_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
  return duration.count();
#else
  struct timeval tv;
  struct timezone tz;
  gettimeofday(&tv, &tz);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif 
}

static int64_t getCurrentTimeMicro() {
  auto now = std::chrono::system_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch());
  return duration.count();
};

template <class T>
static void writeData(uint8_t* buf, T num) {
  size_t len(sizeof(T));
  for (size_t i = 0; i < len; ++i) {
    buf[i] = (uint8_t)(num >> (i * 8));
  }
}

template <class T>
static void readData(uint8_t* buf, T& num) {
  uint8_t len(sizeof(T));
  static uint8_t byMask(0xFF);
  num = 0;

  for (size_t i = 0; i < len; ++i) {
    num <<= 8;
    num |= (T)(buf[len - 1 - i] & byMask);
  }
}


}