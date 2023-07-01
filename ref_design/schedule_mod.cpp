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


#include "schedule_mod.hpp"

#include "loader.hpp"

#include <sklk-mii/logger.hpp>

const std::string ref_design_schedule_mod_name{"scheduling"};

class sklk_phy_mod_loader_template;

ref_design_schedule_mod::ref_design_schedule_mod(ref_design_mod_loader *loader,  const mimo_rrh_scheduler_config &config) :
    sklk_phy_modding(ref_design_schedule_mod_name),
    _loader(loader),
    _num_resouce_blks(config.num_bands),
    _dl_pages(_num_resouce_blks, nullptr),
    _ul_pages(_num_resouce_blks, nullptr)
{
}

bool ref_design_schedule_mod::run_once()
{
    bool run_again{false};

    if (not _initialized) {
        if (sklk_mii_set_thread_priority(0.7) < 0)
        {
            sklk_mii_log::warn("Could not set elevated thread priority");
        }
        _initialized = true;
    }

    //! [get the weight page]
    _loader->get_weight_pages(true, [&](size_t resouce_blk_no, const sklk_phy_weight_page_id_t &page_hdl) {
        if (sklk_phy_mod_page_access::page_is_valid(page_hdl))
            _dl_pages[resouce_blk_no] = page_hdl;
        run_again = true;
    });
    _loader->get_weight_pages(false, [&](size_t resouce_blk_no, const sklk_phy_weight_page_id_t &page_hdl) {
        if (sklk_phy_mod_page_access::page_is_valid(page_hdl))
            _ul_pages[resouce_blk_no] = page_hdl;
        run_again = true;
    });
    //! [get the weight page]

    //! [get schedule request]
    sklk_phy_mod_schedule_request_msg_t msg{};
    while (_msg_queues.schedule_request.pop(msg))
    {
        const auto &[frame_time, sfn] = msg;
        schedule_update(frame_time, sfn);
    }
    //! [get schedule request]

    if (_handle_grant_stats())
        run_again = true;

    return run_again;
}

void ref_design_schedule_mod::ue_changed(size_t key, const sklk_phy_ue &ue [[maybe_unused]], bool is_new)
{
    sklk_mii_log::info("{}: UE update {} is_new={}", get_name(), key, is_new);
}

void ref_design_schedule_mod::ue_radio_changed(size_t key, const sklk_phy_ue_radio &ue_radio [[maybe_unused]], bool is_new)
{
    sklk_mii_log::info("{}: UE radio update {} is_new={}", get_name(), key, is_new);

}
void ref_design_schedule_mod::ue_stream_changed(size_t key, const sklk_phy_ue_stream &ue_stream [[maybe_unused]], bool is_new)
{
    sklk_mii_log::info("{}: UE stream update {} is_new={}", get_name(), key, is_new);
    if (not is_new) {
        // There has been a modification.  Check all pages
        for (size_t resouce_blk_no = 0; resouce_blk_no < _num_resouce_blks; resouce_blk_no++) {
            if (not sklk_phy_mod_page_access::page_is_valid(_ul_pages[resouce_blk_no]))
                _ul_pages[resouce_blk_no] = nullptr;
            if (not sklk_phy_mod_page_access::page_is_valid(_dl_pages[resouce_blk_no]))
                _dl_pages[resouce_blk_no] = nullptr;
        }
    }
}

//! [respond to schedule request]
void ref_design_schedule_mod::schedule_update(size_t frame_time [[maybe_unused]], uint8_t sfn [[maybe_unused]])
{
    for (size_t resouce_blk_no = 0; resouce_blk_no < _num_resouce_blks; resouce_blk_no++) {
        _loader->send_schedule_response(frame_time, sfn, resouce_blk_no, true, _dl_pages[resouce_blk_no]);
        _loader->send_schedule_response(frame_time, sfn, resouce_blk_no, false, _ul_pages[resouce_blk_no]);
    }
}
//! [respond to schedule request]

nlohmann::json ref_design_schedule_mod::dump_grants(size_t req_resource_blk_no, bool is_downlink)
{
    std::lock_guard guard(_grant_stats_lock);
    size_t request_id = _grant_stats_request_id;
    _grant_stats_request_queue.send_no_wake(request_id, req_resource_blk_no, is_downlink);
    nlohmann::json j = nlohmann::json::array();

    std::tuple<size_t, ssize_t, mimo_weight_page_id_t> msg{};
    const auto &[response_id, resource_blk_no, page_hdl] = msg;
    while (_grant_stats_response_queue.pop(msg, 1000)) {
        if (response_id < request_id)
            continue;
        assert(response_id == request_id);
        if (page_hdl == nullptr)
            break;

        auto entry = sklk_phy_mod_page_access::dump_group(page_hdl);
        entry["xfer"] = is_downlink ? "downlink" : "uplink";
        entry["resource_blk_no"] = resource_blk_no;
        j.push_back(entry);
    }
    return j;
}

bool ref_design_schedule_mod::_handle_grant_stats()
{
    std::tuple<size_t, ssize_t, bool> msg{};
    const auto &[request_id, req_resource_blk_no, is_downlink] = msg;
    if (_grant_stats_request_queue.pop(msg)) {
        if (req_resource_blk_no >= 0) {
            _send_grant_stats(request_id, req_resource_blk_no, is_downlink);
        } else {
            for(size_t resource_blk_no = 0; resource_blk_no < _num_resouce_blks; ++resource_blk_no) {
                _send_grant_stats(request_id, resource_blk_no, is_downlink);
            }
        }

        auto ok = _grant_stats_response_queue.send(request_id, -1, nullptr);
        assert(ok);
        return true;
    }
    return false;
}

void ref_design_schedule_mod::_send_grant_stats(size_t request_id, size_t resource_blk_no, bool is_downlink)
{
    const auto &pages = is_downlink ? _dl_pages : _ul_pages;
    auto &page = pages[resource_blk_no];
    if (page) {
        auto ok = _grant_stats_response_queue.send(request_id, resource_blk_no, page);
        assert(ok);
    }
}
