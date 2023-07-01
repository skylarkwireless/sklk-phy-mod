/*
*   Copyright [2023] [Skylark Wireless LLC]
*
*   Licensed under the Apache License, Version 2.0 (the "License");
*   you may not use this file except in compliance with the License.
*   You may obtain a copy of the License at
*
*       http://www.apache.org/licenses/LICENSE-2.0
*
*   Unless required by applicable law or agreed to in writing, software
*   distributed under the License is distributed on an "AS IS" BASIS,
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*   See the License for the specific language governing permissions and
*   limitations under the License.
*/


#pragma once

#include "jsonrpccxx/server.hpp"

class ref_design_mod_loader;

class ref_design_rpc_handler
{
    ref_design_mod_loader *_loader;

public:
    explicit ref_design_rpc_handler(ref_design_mod_loader *loader);

    void add_commands(jsonrpccxx::JsonRpc2Server &rpc_server);

    void get_updates();

private:
    [[nodiscard]] nlohmann::json _rpc_get_group_summary(ssize_t resource_blk_no);
};