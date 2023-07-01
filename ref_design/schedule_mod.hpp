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

#include "sklkphy/modding.hpp"
#include "sklkphy/mimo_rrh_scheduler.hpp"
#include "sklkphy/common.hpp"

extern const std::string ref_design_schedule_mod_name;
class ref_design_mod_loader;

class ref_design_schedule_mod : public sklk_phy_modding
{
    bool _initialized{false};
    ref_design_mod_loader *_loader;
    size_t _num_resouce_blks;
    std::vector<sklk_phy_weight_page_id_t> _dl_pages{};
    std::vector<sklk_phy_weight_page_id_t> _ul_pages{};

    ////////////////////////////////////////////////////////////////////
    // Grant stats
    ////////////////////////////////////////////////////////////////////
    std::mutex _grant_stats_lock;
    std::atomic_size_t _grant_stats_request_id{};
    sklk_mii_message_queue<std::tuple<size_t, ssize_t, bool>, 100> _grant_stats_request_queue;
    sklk_mii_message_queue<std::tuple<size_t, ssize_t, mimo_weight_page_id_t>, 1000> _grant_stats_response_queue;

public:
    ref_design_schedule_mod(ref_design_mod_loader *loader, const mimo_rrh_scheduler_config &config);
    ~ref_design_schedule_mod() override = default;

    void ue_changed(size_t key, const sklk_phy_ue &ue, bool is_new) override;
    void ue_radio_changed(size_t key, const sklk_phy_ue_radio &ue_radio, bool is_new) override;
    void ue_stream_changed(size_t key, const sklk_phy_ue_stream &ue_stream, bool is_new)  override;

    bool run_once() override;

    void schedule_update(size_t frame_time, uint8_t sfn);

    [[nodiscard]] nlohmann::json dump_grants(size_t resource_blk_no, bool is_downlink);

private:
    bool _handle_grant_stats();
    void _send_grant_stats(size_t request_id, size_t resource_blk_no, bool is_downlink);
};
