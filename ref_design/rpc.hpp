#pragma once

#include "api.hpp"

#include <jsonrpccxx/server.hpp>

class ref_design_mod_loader;

class SKLK_PHY_MOD_REFDESIGN_API ref_design_rpc_handler
{
    ref_design_mod_loader *_loader;

public:
    explicit ref_design_rpc_handler(ref_design_mod_loader *loader);

    void add_commands(jsonrpccxx::JsonRpc2Server &rpc_server);

    void get_updates();

private:
    [[nodiscard]] nlohmann::json _rpc_get_group_summary(ssize_t resource_blk_no);
};
