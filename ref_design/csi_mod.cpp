#include "csi_mod.hpp"
#include "loader.hpp"
#include "utils.hpp"

#include <sklkphy/weights.hpp>

#include <sklk-mii/simple_log.hpp>

#include <sklk-dsp/utils.hpp>

#define ARMA_MAT_PREALLOC (SKLK_PHY_MAX_RADIOS*SKLK_PHY_MAX_MIMO_USERS)
#include <armadillo>

#define TX_BF_SCALE_FLT (0.5f/1.05f)
#define RX_BF_SCALE_FLT (0.5f)

const std::string ref_design_csi_mod_name{"csi"};

class sklk_phy_mod_loader_template;

ref_design_csi_mod::ref_design_csi_mod(ref_design_mod_loader *loader, const mimo_rrh_scheduler_config &config) :
    sklk_phy_modding(ref_design_csi_mod_name),
    _loader(loader),
    _num_resouce_blks(config.num_bands),
    _max_spatial_streams(config.max_users_per_group),
    _num_estimations(config.num_pilot_estimates),
    _randomizer{std::random_device{}()}
{
}

bool ref_design_csi_mod::run_once()
{
    if (not _initialized) {
        if (sklk_mii_set_thread_priority(0.6) < 0)
        {
            sklk_mii_log::warn("Could not set elevated thread priority");
        }
        _initialized = true;
    }

    sklk_phy_mod_enable_radio_msg_t enable_radio_msg{};
    while (_msg_queues.enable_radio.pop(enable_radio_msg))
    {
        const auto &[frame_time, radio_ch, enable] = enable_radio_msg;
        _radio_enabled[radio_ch] = enable;
    }

    sklk_phy_mod_cc_msg_t cc_msg;
    while (_msg_queues.cc.pop(cc_msg))
    {
        const auto &[frame_time, radio_ch, resource_block_no, est_no, value] = cc_msg;
        _cc_values.at(resource_block_no).at(est_no).at(radio_ch) = value;
    }

    //! [CSI module requesting CSI update]
    sklk_phy_mod_csi_msg_t csi_msg;
    while (_msg_queues.csi.pop(csi_msg))
    {
        const auto &[frame_time, key, ue_radio, resource_blk_no, est_no, vec] = csi_msg;
        csi_update(frame_time, key, ue_radio, resource_blk_no, est_no, vec);
    }
    //! [CSI module requesting CSI update]

    _calculate_weights();

    return false;
}

void ref_design_csi_mod::ue_changed(size_t key, const sklk_phy_ue &ue [[maybe_unused]], bool is_new)
{
    sklk_mii_log::info("{}: UE update {} is_new={}", get_name(), key, is_new);
}

void ref_design_csi_mod::ue_radio_changed(size_t key, const sklk_phy_ue_radio &ue_radio [[maybe_unused]], bool is_new)
{
    sklk_mii_log::info("{}: UE radio update {} is_new={}", get_name(), key, is_new);

}
void ref_design_csi_mod::ue_stream_changed(size_t key, const sklk_phy_ue_stream &ue_stream [[maybe_unused]], bool is_new)
{
    sklk_mii_log::info("{}: UE stream update {} is_new={}", get_name(), key, is_new);
}

//! [CSI module creating a container]
sklk_phy_mod_container_ptr_t ref_design_csi_mod::allocate_ue_radio()
{
    return std::make_shared<ref_design_csi_radio_container>();
}
//! [CSI module creating a container]

//! [CSI module receiving CSI update]
void ref_design_csi_mod::csi_update(
    size_t frame_time, size_t key[[maybe_unused]], const sklk_phy_ue_radio &ue_radio, size_t resource_blk_no, size_t est_idx, const sklk_phy_csi_vec &vec)
{
    auto ptr = sklk_phy_mod_ue_access::get_container(get_name(), ue_radio);
    auto ue_radio_container = std::dynamic_pointer_cast<ref_design_csi_radio_container>(ptr);
    if (not ue_radio_container)
        return;
    auto &csi = ue_radio_container->csi.at(resource_blk_no).at(est_idx);
    csi.set_csi(frame_time, vec);
    _last_frame_time = frame_time;
}
//! [CSI module receiving CSI update]

void ref_design_csi_mod::_calculate_weights()
{
    for (size_t resource_blk_no = 0; resource_blk_no < _num_resouce_blks; resource_blk_no++) {
        _calculate_weights(resource_blk_no, true);
        _calculate_weights(resource_blk_no, false);
    }
}

void ref_design_csi_mod::_calculate_weights(size_t resource_blk_no, bool is_downlink)
{
    std::vector<sklk_phy_ue_stream> all_ue_streams{};
    all_ue_streams.reserve(ue_radio_map.size());
    for (const auto &[_, ue_radio] : ue_radio_map) {
        auto ptr = sklk_phy_mod_ue_access::get_container(get_name(), ue_radio);
        auto ue_radio_container = std::dynamic_pointer_cast<ref_design_csi_radio_container>(ptr);
        if (ue_radio_container and ue_radio_container->csi[resource_blk_no].ready())
            all_ue_streams.emplace_back(ue_radio);
    }
    if (all_ue_streams.empty())
        return;

    std::vector<sklk_phy_ue_stream>  ue_streams_to_use{};
    auto num_csi = std::max(all_ue_streams.size(), _max_spatial_streams);
    if (num_csi == all_ue_streams.size()) {
        std::swap(all_ue_streams, ue_streams_to_use);
    } else {
        std::sample(all_ue_streams.begin(), all_ue_streams.end(), std::back_inserter(ue_streams_to_use), num_csi, _randomizer);
    }

    _calculate_weight_page(ue_streams_to_use, resource_blk_no, is_downlink);
}

void ref_design_csi_mod::_calculate_weight_page(
    const std::vector<sklk_phy_ue_stream> &ue_streams, size_t resource_blk_no, bool is_downlink)
{
    auto page_hdl = _loader->get_weight_page(_last_frame_time, resource_blk_no, is_downlink, ue_streams).initialize().first;

    for (size_t est_idx = 0; est_idx < _num_estimations; est_idx++) {
        if (not _calculate_weight_page_estimate(page_hdl, resource_blk_no, est_idx, is_downlink)) {
            auto identifier = get_identifier(ue_streams);
            sklk_mii_log::error("Weight calculation failed: streams: {}", identifier);
            sklk_phy_mod_page_access::set_page_status(_last_frame_time, page_hdl, false);
            return;
        }
    }

    sklk_phy_weight_page &page = *sklk_phy_mod_page_access::get_page_from_hdl(page_hdl);
    if (is_downlink) {
        _scale_pages_for_downlink(page, ue_streams.size(), SKLK_PHY_MAX_RADIOS, TX_BF_SCALE_FLT);
    } else {
        _scale_pages_for_uplink(page, ue_streams.size(), SKLK_PHY_MAX_RADIOS, RX_BF_SCALE_FLT);
    }

    sklk_phy_mod_page_access::set_page_status(_last_frame_time, page_hdl, true);
    _loader->send_weight_page(resource_blk_no, is_downlink, page_hdl);
}

bool ref_design_csi_mod::_calculate_weight_page_estimate(
    const sklk_phy_weight_page_id_t &page_hdl, size_t resource_blk_no, size_t est_idx, bool is_downlink )
{
    std::array<size_t, SKLK_PHY_MAX_RADIOS> _indexes{};
    size_t num_radios{};
    assert(num_radios == 0);
    for(size_t radio_ch{0}; radio_ch < SKLK_PHY_MAX_RADIOS; radio_ch++) {
        assert(num_radios < SKLK_PHY_MAX_RADIOS);
        if (_radio_enabled[radio_ch])
            _indexes[num_radios++] = radio_ch;
    }
    assert(num_radios <= SKLK_PHY_MAX_RADIOS);

    if (not num_radios) {
        return false;
    }

    sklk_phy_weight_page &page = *sklk_phy_mod_page_access::get_page_from_hdl(page_hdl);
    auto streams = sklk_phy_mod_page_access::get_ue_streams(page_hdl);

    arma::Mat<sklk_mii_cf_t> A(streams.size(), num_radios);
    arma::Mat<sklk_mii_cf_t> B(num_radios, streams.size());
    static_assert(arma::arma_config::mat_prealloc == ARMA_MAT_PREALLOC);

    //load the matrix with channel estimates for this particular subcarrier
    for (size_t userno = 0; userno < streams.size(); userno++)
    {
        auto ue_radio = sklk_phy_mod_ue_access::get_container(get_name(), streams[userno]);
        auto ue_radio_container = std::dynamic_pointer_cast<ref_design_csi_radio_container>(ue_radio);

        // NOTE: This works because there is currently a one-to-one mapping from radio to stream.
        const sklk_phy_csi_vec zeros{};
        const sklk_phy_csi_vec &user_csi_vec = ue_radio_container ? ue_radio_container->csi[resource_blk_no][est_idx].data() : zeros;

        for (size_t radio_idx = 0; radio_idx < num_radios; radio_idx++) {
            size_t radio_ch = _indexes[radio_idx];
            assert(radio_ch < SKLK_PHY_MAX_RADIOS);
            auto value = user_csi_vec[radio_ch];
            if (is_downlink)
                value *= _cc_values[resource_blk_no][est_idx][radio_ch];
            A.row(userno)[radio_idx] = value;
        }
    }

    //compute the pseudo-inverse
    //the divide-and-conquer method provides slightly different results than the standard method, but is considerably faster for large matrices
    if (not arma::pinv(B, A, 0, "std"))
    {
        sklk_mii_log::error("pinv failed");
        return false;
    }

    //copy pinv buffer into weight structure
    for (size_t userno = 0; userno < streams.size(); userno++) {
        // Clear the weights for disable radios
        for(size_t radio_ch{0}; radio_ch < SKLK_PHY_MAX_RADIOS; radio_ch++) {
            assert(num_radios < SKLK_PHY_MAX_RADIOS);
            if (not _radio_enabled[radio_ch])
                page.get_symbol(radio_ch, userno, est_idx) = sklk_mii_cf_t{};
        }

        // Set the weights enable radios
        for (size_t radio_idx = 0; radio_idx < num_radios; radio_idx++) {
            size_t radio_ch = _indexes[radio_idx];
            auto &wf = page.get_symbol(radio_ch, userno, est_idx);
            wf = B.row(radio_idx)[userno];
        }
    }

    return true;
}

void ref_design_csi_mod::_scale_pages_for_downlink(
    sklk_phy_weight_page &page, size_t num_users, size_t num_radios, float amplitude_ceiling)
{
    for (size_t sbno = 0; sbno < NUM_PILOT_SUBBANDS; sbno++) {
        // Normalize the weights for each user
        for (size_t userno = 0; userno < num_users; userno++) {
            float dnl_power{};
            for (size_t ch = 0; ch < num_radios; ch++) {
                const auto &d_w = page.get_symbol(ch, userno, sbno);
                dnl_power += sklk_dsp_mag2(d_w);
            }
            const float scale = 1.0f/ std::sqrt(dnl_power);
            for (size_t ch = 0; ch < num_radios; ch++) {
                page.get_symbol(ch, userno, sbno) *= scale;
            }
        }

        // Get max power across all users
        float max_power{};
        for (size_t ch = 0; ch < num_radios; ch++){
            for (size_t userno = 0; userno < num_users; userno++) {
                const auto &w = page.get_symbol(ch, userno, sbno);
                max_power = std::max(max_power, sklk_dsp_mag2(w));
            }
        }

        // Scale relative to largest power
        const float scale = amplitude_ceiling/std::sqrt(max_power);

        // Normalize the weights
        for (size_t ch = 0; ch < num_radios; ch++) {
            for (size_t userno = 0; userno < num_users; userno++) {
                auto &w = page.get_symbol(ch, userno, sbno);
                w *= scale;
            }
        }
    }
}

void ref_design_csi_mod::_scale_pages_for_uplink(
    sklk_phy_weight_page &page, size_t num_users, size_t num_radios, float amplitude_ceiling)
{
    for (size_t sbno = 0; sbno < NUM_PILOT_SUBBANDS; sbno++)
    {
        // Scale each user separately.
        for (size_t userno = 0; userno < num_users; userno++)
        {
            float max_power{};
            for (size_t ch = 0; ch < num_radios; ch++)
            {
                auto &w = page.get_symbol(ch, userno, sbno);
                max_power = std::max(max_power, sklk_dsp_mag2(w));
            }

            const float scale = amplitude_ceiling / 1.0;
            for (size_t ch = 0; ch < num_radios; ch++)
            {
                auto &w = page.get_symbol(ch, userno, sbno);
                w *= scale;
            }
        }
    }
}
