#include <cstddef>

#include "spec/schema.hpp"

static constexpr Schema kSchemas[(size_t)SchemaId::_COUNT] = {
    /* None */    schema().build(),
    /* Key */     schema().req({FieldId::Key}).build(),
    /* KeyValue*/ schema().req({FieldId::Key, FieldId::Value}).build(),
    /* KeyI64 */  schema().req({FieldId::Key, FieldId::I64}).build(),
    /* KeyF64 */  schema().req({FieldId::Key, FieldId::F64}).build(),

    /* Set */ schema()
        .req({FieldId::Key, FieldId::Value})
        .opt({FieldId::Nx, FieldId::Xx, FieldId::Get, FieldId::Px, FieldId::Ex, FieldId::KeepTtl})
        .xor_({FieldId::Nx, FieldId::Xx})
        .xor_({FieldId::Px, FieldId::Ex})
        .build(),
};     

const Schema& schema_of(SchemaId id) {
	return kSchemas[(size_t)id];
}	