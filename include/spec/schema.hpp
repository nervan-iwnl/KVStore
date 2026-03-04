#pragma once
#include <cstdint>
#include <array>
#include <initializer_list>

enum class FieldId : uint8_t {
  Key, Value, I64, F64,
  Nx, Xx, Get, Px, Ex, KeepTtl,
  _COUNT
};

using Mask = uint64_t;
constexpr Mask B(FieldId f) { return 1ull << (uint8_t)f; }

enum class SchemaId : uint8_t {
  None,
  Key,
  KeyValue,
  KeyI64,
  KeyF64,
  Set,
  _COUNT
};

struct ImpliesRule {
  Mask a = 0;
  Mask b = 0;
};


struct Schema {
  	Mask req = 0;
	Mask opt = 0;
	Mask repeatable = 0;
	bool strict_unknown = true;

	std::array<Mask, 4> xors{};  
	uint8_t xor_n = 0;

	std::array<ImpliesRule, 4> implies{}; 
  	uint8_t implies_n = 0;
};

struct SchemaBuilder {
	Schema s{};

	constexpr SchemaBuilder& req(std::initializer_list<FieldId> fs) {
		for (auto f: fs) s.req |= B(f);
		return *this;
	}
	constexpr SchemaBuilder& opt(std::initializer_list<FieldId> fs) {
		for (auto f: fs) s.opt |= B(f);
		return *this;
	}
	constexpr SchemaBuilder& repeat(std::initializer_list<FieldId> fs) {
		for (auto f: fs) s.repeatable |= B(f);
		return *this;
	}
	constexpr SchemaBuilder& xor_(std::initializer_list<FieldId> fs) {
		Mask m = 0;
		for (auto f: fs) m |= B(f);
		s.xors[s.xor_n++] = m;
		return *this;
	}
	constexpr SchemaBuilder& implies_any(std::initializer_list<FieldId> if_any,
										std::initializer_list<FieldId> must_any) {
		Mask a = 0, b = 0;
		for (auto f: if_any)  a |= B(f);
		for (auto f: must_any) b |= B(f);
		s.implies[s.implies_n++] = ImpliesRule{a, b};    return *this;
	}
	constexpr SchemaBuilder& lenient_unknown() { s.strict_unknown = false; return *this; }

	constexpr Schema build() const { return s; }
};

constexpr SchemaBuilder schema() { return {}; }

inline bool has2plus_bits(Mask m) { return (m & (m - 1)) != 0; }

inline bool validate_schema(const Schema& sc, Mask seen) {
	if ((seen & sc.req) != sc.req) return false;
  	if (sc.strict_unknown && (seen & ~(sc.req | sc.opt)) != 0) return false;

  	for (uint8_t i = 0; i < sc.xor_n; ++i)
    	if (has2plus_bits(seen & sc.xors[i])) return false;

  	for (uint8_t i = 0; i < sc.implies_n; ++i) {
    	auto [a, b] = sc.implies[i];
    	if ((seen & a) && !(seen & b)) return false;
  	}

  	return true;
}

const Schema& schema_of(SchemaId id);

