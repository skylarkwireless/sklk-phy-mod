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


#include "rpc.hpp"
#include "loader.hpp"
#include "schedule_mod.hpp"

#include "sklk-mii/function_utils.hpp"

ref_design_rpc_handler::ref_design_rpc_handler(ref_design_mod_loader *loader) :
    _loader(loader)
{
}

void ref_design_rpc_handler::add_commands(jsonrpccxx::JsonRpc2Server &rpc_server)
{
    using namespace jsonrpccxx;

    std::weak_ptr<ref_design_rpc_handler> wptr = _loader->rpc_hdl;

    rpc_server.ForceAdd("get_group_summary", "", sklk_mii_safe_callback(&ref_design_rpc_handler::_rpc_get_group_summary, wptr), NamedParamMapping{"resource_blk_no"});
}

void ref_design_rpc_handler::get_updates()
{

}

nlohmann::json ref_design_rpc_handler::_rpc_get_group_summary(ssize_t resource_blk_no)
{
    auto j = nlohmann::json::array();
    auto schedule_mod = _loader->scedule_mod.lock();
    if (not schedule_mod)
        return j;

    for (auto entry : schedule_mod->dump_grants(resource_blk_no, false)) {
        j.push_back(entry);
    }

    for (auto entry : schedule_mod->dump_grants(resource_blk_no, true)) {
        j.push_back(entry);
    }

    return j;
}
