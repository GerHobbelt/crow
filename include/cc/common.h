#pragma once
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include "cc/str.h"
namespace cc {
  enum class HTTP {
	DEL=0,GET,HEAD,POST,PUT,CONNECT,OPTIONS,
	TRACE,PATCH,PURGE,InternalMethodCount,
	// should not add an item below this line: used for array count
  };
  inline std::string m2s(HTTP m) {
	switch (m) {
	  case HTTP::DEL:return "DELETE";
	  case HTTP::GET:return "GET";
	  case HTTP::HEAD:return "HEAD";
	  case HTTP::POST:return "POST";
	  case HTTP::PUT:return "PUT";
	  case HTTP::CONNECT:return "CONNECT";
	  case HTTP::OPTIONS:return "OPTIONS";
	  case HTTP::TRACE:return "TRACE";
	  case HTTP::PATCH:return "PATCH";
	  case HTTP::PURGE:return "PURGE";
	  default:return "invalid";
	}
	return "invalid";
  }
  inline HTTP c2m(const char*m) {
	switch (hack8Str(m)) {
	  case "DELETE"_l:return cc::HTTP::DEL;
	  case 4670804:return cc::HTTP::GET;
	  case 1212498244:return cc::HTTP::HEAD;
	  case 1347375956:return cc::HTTP::POST;
	  case 5264724:return cc::HTTP::PUT;
	  case "OPTIONS"_l:return cc::HTTP::OPTIONS;
	  case "CONNECT"_l:return cc::HTTP::CONNECT;
	  case "TRACE"_l:return cc::HTTP::TRACE;
	  case "PATCH"_l:return cc::HTTP::PATCH;
	  case "PURGE"_l:return cc::HTTP::PURGE;
	}
	return HTTP::InternalMethodCount;
  }
  enum class ParamType {
	INT,UINT,DOUBLE,
	STRING,PATH,MAX
  };
  struct routing_params {
	std::vector<int64_t> int_params;
	std::vector<uint64_t> uint_params;
	std::vector<double> double_params;
	std::vector<std::string> string_params;
	void debug_print() const {
	  std::cerr<<"routing_params"<<std::endl;
	  for (auto i:int_params) std::cerr<<i<<", ";
	  std::cerr<<std::endl;
	  for (auto i:uint_params) std::cerr<<i<<", ";
	  std::cerr<<std::endl;
	  for (auto i:double_params) std::cerr<<i<<", ";
	  std::cerr<<std::endl;
	  for (auto& i:string_params) std::cerr<<i<<", ";
	  std::cerr<<std::endl;
	}
	template <typename T>T get(unsigned) const;
  };
  template<>
  inline int64_t routing_params::get<int64_t>(unsigned index) const { return int_params[index]; }
  template<>
  inline uint64_t routing_params::get<uint64_t>(unsigned index) const { return uint_params[index]; }
  template<>
  inline double routing_params::get<double>(unsigned index) const { return double_params[index]; }
  template<>
  inline std::string routing_params::get<std::string>(unsigned index) const { return string_params[index]; }
}
#ifndef MSVC_WORKAROUND
constexpr cc::HTTP operator""_mt(const char* str,size_t /*len*/) {
  switch (hack8Str(str)) {
	case "DELETE"_l:return cc::HTTP::DEL;
	case "GET"_i:return cc::HTTP::GET;
	case "HEAD"_i:return cc::HTTP::HEAD;
	case "POST"_i:return cc::HTTP::POST;
	case "PUT"_i:return cc::HTTP::PUT;
	case "OPTIONS"_l:return cc::HTTP::OPTIONS;
	case "CONNECT"_l:return cc::HTTP::CONNECT;
	case "TRACE"_l:return cc::HTTP::TRACE;
	case "PATCH"_l:return cc::HTTP::PATCH;
	case "PURGE"_l:return cc::HTTP::PURGE;
  }
  throw std::runtime_error("invalid http method");
}
#endif