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


#include "loader.hpp"

#include "rpc.hpp"
#include "csi_mod.hpp"
#include "schedule_mod.hpp"

#include <iostream>

//! [Factory installed on module load]
static auto factory_is_installed [[maybe_unused]] = sklk_phy_mod_loader_factory::set_factory(
    std::make_shared<ref_design_mod_loader_factory>()
);
//! [Factory installed on module load]

//! [Factory creating the loader]
std::shared_ptr<sklk_phy_mod_loader> ref_design_mod_loader_factory::create(
    const sklk_phy_scheduler_config & config)
{
    auto ptr = std::make_shared<ref_design_mod_loader>(config);
    return ptr;
}
//! [Factory creating the loader]

//! [The loader creating the modules]
ref_design_mod_loader::ref_design_mod_loader(const sklk_phy_scheduler_config & config) :
    sklk_phy_mod_loader(config)
{
    rpc_hdl = std::make_shared<ref_design_rpc_handler>(this);
    auto local_csi_mod = std::make_shared<ref_design_csi_mod>(this, config);
    auto local_schedule_mod = std::make_shared<ref_design_schedule_mod>(this, config);

    //! [Subscribing to message queues]
    _msg_queues.enable_radio.subscribe(local_csi_mod);
    _msg_queues.cc.subscribe(local_csi_mod);

    //! [Subscribing to the csi queue]
    _msg_queues.csi.subscribe(local_csi_mod);
    //! [Subscribing to the csi queue]

    _msg_queues.schedule_request.subscribe(local_schedule_mod);
    //! [Subscribing to message queues]

    csi_mod = local_csi_mod;
    scedule_mod = local_schedule_mod;
    this->add_module(std::move(local_csi_mod));
    this->add_module(std::move(local_schedule_mod));
}
//! [The loader creating the modules]

//! [Send the weights between modules]
void ref_design_mod_loader::send_weight_page(size_t resource_blk_no, bool is_downlink, const sklk_phy_weight_page_id_t &page_hdl)
{
    auto &queue = is_downlink ? _dl_schedule_weight_pages : _ul_schedule_weight_pages;
    auto ok = queue.send_no_wake(resource_blk_no, page_hdl);
    assert(ok);
}
//! [Send the weights between modules]

void ref_design_mod_loader::add_rpc_commands(jsonrpccxx::JsonRpc2Server &rpc_server [[maybe_unused]])
{
    rpc_hdl->add_commands(rpc_server);
}

void ref_design_mod_loader::rpc_get_updates()
{
    rpc_hdl->get_updates();
}
