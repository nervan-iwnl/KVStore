#include "app/dispatch.hpp"
#include "app/response.hpp"
#include "app/command_registry.hpp"
#include "domain/kv_store.hpp"
#include "spec/schema.hpp"


CoreResp dispatch(const CoreReq& req, AppContext& ctx) {
    const auto* spec = command_registry::find_by_op(req.op);
    if (!spec || !spec->handler) return err("ERR unknown op");

    if (!validate_schema(schema_of(spec->schema_id), req.fields_seen)) {
        return err("ERR bad args");
    }

    return spec->handler(req, ctx);
}