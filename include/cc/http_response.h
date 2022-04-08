
#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include "cc/json.hh"
#include "cc/http_request.h"
#include "cc/any_types.h"
#include "cc/ci_map.h"
#include "cc/detail.h"

namespace cc { static const std::string RES_CT("Content-Type", 12), RES_CL("Content-Length", 14), RES_Loc("Location", 8), Res_Ca("Cache-Control", 13), RES_Cookie("Cookie", 6), RES_AJ("application/json", 16), RES_Txt("text/html;charset=UTF-8", 23), RES_Xc("X-Content-Type-Options", 22), RES_No("nosniff", 7), RES_Allow("Allow", 5); template <typename Adaptor, typename Handler, typename ... Middlewares> class Connection; struct Res { private: ci_map headers; public: template <typename Adaptor, typename Handler, typename ... Middlewares> friend class cc::Connection; uint16_t code{ 200 }; std::string body; json json_value; 
#ifdef ENABLE_COMPRESSION
 bool compressed = true; 
#endif
 bool is_head_response = false;  inline void set_header(const std::string& key, std::string value) { headers.erase(key); headers.emplace(key, std::move(value)); }  inline void add_header(const std::string& key, std::string value) { headers.emplace(key, std::move(value)); } const std::string& get_header_value(const std::string& key) { return cc::get_header_value(headers, key); } Res() {} ~Res() { complete_request_handler_ = nullptr; } explicit Res(int code) : code(code) {} Res(std::string body) : body(std::move(body)) {} Res(int code, std::string body) : code(code), body(std::move(body)) {} Res(const json&& json_value) : body(std::move(json_value).dump()) { headers.emplace(RES_CT, RES_AJ); } Res(int code, json& json_value) : code(code), body(json_value.dump()) { headers.emplace(RES_CT, RES_AJ); } Res(const char*&& char_value) : body(std::move(char_value)) {} Res(int code, const char*&& char_value) : code(code), body(std::move(char_value)) {} Res(Res&& r) { *this = std::move(r); } Res& operator = (const Res& r) = delete; Res& operator = (Res&& r) noexcept { body = std::move(r.body); json_value = std::move(r.json_value); code = r.code; headers = std::move(r.headers); path_ = std::move(r.path_); completed_ = r.completed_; return *this; } inline bool is_completed() const noexcept { return completed_; } inline void clear() { completed_ = false; body.clear(); headers.clear(); }   inline void redirect(const std::string& location) { code = 301; headers.erase(RES_Loc); headers.emplace(RES_Loc, std::move(location)); }  inline void redirect_perm(const std::string& location) { code = 303; headers.erase(RES_Loc); headers.emplace(RES_Loc, std::move(location)); } void write(const std::string& body_part) { body += body_part; } inline void end() { if (!completed_) { completed_ = true; if (is_head_response) { set_header(RES_CL, std::to_string(body.size())); body = ""; } if (complete_request_handler_) { complete_request_handler_(); } } } inline void end(const std::string& body_part) { body += body_part; end(); }  void set_static_file_info(std::string path) { struct stat statbuf_; path_ = detail::directory_; path_ += DecodeURL(path); is_file = stat(path_.c_str(), &statbuf_);
#ifdef ENABLE_COMPRESSION
 compressed = false;
#endif
 if (is_file == 0) {  std::string ss = path.substr(path.find_last_of('.') + 1); std::string_view extension(ss.data(), ss.size()); this->add_header(RES_CL, std::to_string(statbuf_.st_size)); if (content_types.find(extension) != content_types.end()) { if (ss[0] == 'h') { is_file = 2; } else { is_file = 1; ss = content_types[extension]; this->add_header(RES_CT, ss); } } else { code = 404; this->headers.clear(); this->end();  } } else { code = 404; } } private: std::string path_; int is_file{ 0 }; bool completed_{}; std::function<void()> complete_request_handler_; };}