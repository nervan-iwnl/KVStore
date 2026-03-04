#pragma once
#include "command.hpp"
#include <utility>

inline CoreResp ok() { return {RespKind::Ok, std::monostate{}}; }
inline CoreResp nul() { return {RespKind::Null, std::monostate{}}; }
inline CoreResp err(std::string s) { return {RespKind::Err, std::move(s)}; }
inline CoreResp val(std::string s) { return {RespKind::Value, std::move(s)}; }
inline CoreResp i64(int64_t x) { return {RespKind::Int, x}; }
inline CoreResp f64(double x) { return {RespKind::Float, x}; }
