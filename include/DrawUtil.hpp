/**
  @author Hidden Track
  @since 2020/05/22
  @time: 10:15
 */

#pragma once

#include <Python.h>
#include <string>
#include <iostream>
#include <vector>
#include <tuple>

//namespace DrawUtil {
template <class T>
inline std::string arr_to_string_list(T* arr, int N) {
  std::string s = "[";
  for (int i = 0; i < N; ++i) {
    s += std::to_string(arr[i]);
    if (i != N - 1) s += ",";
  }
  s += "]";
  return s;
}

template <class T, class V = int>
inline void plot(T* x, int N1, V* y = NULL, bool equal = false) {
  PyRun_SimpleString("import matplotlib.pyplot as plt");
  if (equal) {
    PyRun_SimpleString("plt.axis(\"equal\")");
  }

  std::string cmd = "plt.plot(";
  std::string s1 = arr_to_string_list(x, N1);
  if (y != NULL) {
    std::string s2 = arr_to_string_list(y, N1);
    cmd += (s1 + "," + s2 + ")");
    PyRun_SimpleString(cmd.c_str());
  } else {
    cmd += (s1 + ")");
    PyRun_SimpleString(cmd.c_str());
  }
  PyRun_SimpleString("plt.show()");
}

template <class T, class V = int>
inline void plotOne(T* x, int N1, V* y = NULL) {
  std::string cmd = "plt.plot(";
  std::string s1 = arr_to_string_list(x, N1);
  if (y != NULL) {
    std::string s2 = arr_to_string_list(y, N1);
    cmd += (s1 + "," + s2 + ")");
    std::cout << "cmd: " << cmd << std::endl;
    PyRun_SimpleString(cmd.c_str());
  } else {
    cmd += (s1 + ")");
    PyRun_SimpleString(cmd.c_str());
  }

   PyRun_SimpleString("plt.xlabel(\"Round\")");
}


inline void plotMore(std::vector<std::tuple<double*, double*>> v, int size,
                     bool equal = false) {
  PyRun_SimpleString("import matplotlib.pyplot as plt");
  if (equal) {
    PyRun_SimpleString("plt.axis(\"equal\")");
  }

  //for (std::tuple<T*, T*> it = v.begin(); it != v.end(); ++it) {
  //  it;
  //}

  for (auto a : v) {
    plotOne(std::get<0>(a), size, std::get<1>(a));
  }

  PyRun_SimpleString("plt.show()");
}


inline void py_init() {
  Py_Initialize();
  std::string path = ".";
  std::string chdir_cmd = std::string("sys.path.append(\"") + path + "\")";
  const char* cstr_cmd = chdir_cmd.c_str();
  PyRun_SimpleString("import sys");
  PyRun_SimpleString(cstr_cmd);
}

//inline void py_finalize() { Py_Finalize(); }
//}