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

#include <random>

extern const std::string ref_design_csi_mod_name;
class ref_design_mod_loader;

class ref_design_csi_estimation
{
    sklk_phy_csi_vec _data{};
    bool _valid{false};
    size_t _frame_time;
public:
    void set_csi(size_t frame_time, const sklk_phy_csi_vec &csi) {
        _frame_time = frame_time;
        _data = csi;
        _valid = true;
    }

    const sklk_phy_csi_vec & data() const { return _data; }
    [[nodiscard]] bool is_valid() const {return _valid;}
};

class ref_design_csi_estimations : public std::array<ref_design_csi_estimation, SKLK_PHY_MAX_ESTIMATIONS>
{
public:
    [[nodiscard]] bool ready() const {
        return std::all_of(this->begin(), this->end(),[](const auto & est) {return est.is_valid(); });
    }
};

class ref_design_csi_radio_container : public sklk_phy_mod_container
{
public:
    std::array<ref_design_csi_estimations, SKLK_PHY_MAX_BANDS> csi{};
};

class ref_design_csi_mod : public sklk_phy_modding
{
    bool _initialized{false};
    ref_design_mod_loader *_loader;
    const size_t _num_resouce_blks;
    const size_t _max_spatial_streams;
    const size_t _num_estimations;
    std::mt19937 _randomizer;
    std::array<bool, SKLK_PHY_MAX_RADIOS> _radio_enabled{};
    std::array<
        std::array<sklk_phy_csi_vec, SKLK_PHY_MAX_ESTIMATIONS>,
        SKLK_PHY_MAX_BANDS> _cc_values{};

    size_t _last_frame_time{0};

public:
    ref_design_csi_mod(ref_design_mod_loader *loader, const mimo_rrh_scheduler_config &config);
    ~ref_design_csi_mod() override = default;

    void ue_changed(size_t key, const sklk_phy_ue &ue, bool is_new) override;
    void ue_radio_changed(size_t key, const sklk_phy_ue_radio &ue_radio, bool is_new) override;
    void ue_stream_changed(size_t key, const sklk_phy_ue_stream &ue_stream, bool is_new)  override;

    bool run_once() override;

    sklk_phy_mod_container_ptr_t allocate_ue_radio() override;

    void csi_update(size_t frame_time, size_t key, const sklk_phy_ue_radio &ue_radio, size_t resource_blk_no, size_t est_idx, const sklk_phy_csi_vec &vec);

private:
    void _calculate_weights();
    void _calculate_weights(size_t resource_blk_no, bool is_downlink);
    void _calculate_weight_page(const std::vector<sklk_phy_ue_stream> &ue_streams, size_t resource_blk_no, bool is_downlink);
    bool _calculate_weight_page_estimate(const sklk_phy_weight_page_id_t &page_hdl, size_t resource_blk_no, size_t est_idx, bool is_downlink);

    void _scale_pages_for_uplink(sklk_phy_weight_page &page, size_t num_users, size_t num_radios, float amplitude_ceiling);
    void _scale_pages_for_downlink(sklk_phy_weight_page &page, size_t num_users, size_t num_radios, float amplitude_ceiling);
};
