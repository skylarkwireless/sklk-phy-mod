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



/**
 * @file sklkphy/modding.hpp
 */
#pragma once

#include "sklk-mii/threading.hpp"
#include "sklk-mii/macros.hpp"
#include "sklk-mii/message_queue.hpp"
#include "sklk-mii/safe_thread.hpp"

#include "sklkphy/mimo_rrh_scheduler.hpp"
#include "sklkphy/common.hpp"
#include "sklkphy/weight_page.hpp"

#include <memory>
#include <unordered_map>

/**
 *  @class sklk_phy_ue
 *
 *  A handle for a UE.
 */
class mimo_rrh_radio_unit;
using sklk_phy_ue = std::weak_ptr<mimo_rrh_radio_unit>;

/**
 *  @class sklk_phy_ue_radio
 *
 *  A handle for a UE radio.
 */
class mimo_rrh_connection;
using sklk_phy_ue_radio = std::weak_ptr<mimo_rrh_connection>;

/**
 *  @class sklk_phy_ue_stream
 *
 *  A handle for a UE stream.
 */
using sklk_phy_ue_stream = std::weak_ptr<mimo_rrh_connection>;

class mimo_weight_group;
using sklk_phy_ue_group = std::shared_ptr<mimo_weight_group>;

class sklk_phy_mod_ue_access;
class sklk_phy_mod_loader;
class sklk_phy_modding;
class mimo_sched_bookkeep;
class mimo_sched_main;
class mimo_sched_link_grant;
class mimo_sched_pilot;

using sklk_phy_csi_vec = sklk_phy_pilot_vec_t;
using sklk_phy_scheduler_config = mimo_rrh_scheduler_config;

/**
 * An opaque container to module specific information with each object.
 *
 * See sklk_phy_mod_ue_access::get_container
 *
 * @headerfile modding.hpp <sklkphy/modding.hpp>
 */
class sklk_phy_mod_container
{
public:
    virtual ~sklk_phy_mod_container() = default;
};


/**
 *  @typedef sklk_phy_mod_container sklk_phy_mod_container_ptr_t
 *
 *  Pointer to a sklk_phy_mod_container
 */
typedef std::shared_ptr<sklk_phy_mod_container> sklk_phy_mod_container_ptr_t;
using sklk_phy_mod_container_map = std::unordered_map<std::string, sklk_phy_mod_container_ptr_t>;

/**
 * Base factory class for modding.  Override the create method enable modding.
 *
 * @headerfile modding.hpp <sklkphy/modding.hpp>
 */
class sklk_phy_mod_loader_factory
{
public:
    /**
     * Create a new module loader.
     *
     * @param config module name
     */
    virtual std::shared_ptr<sklk_phy_mod_loader> create(const sklk_phy_scheduler_config &config);

    /**
     * Install the module loader factory.
     *
     * @param factory The factory to create module loaders
     */
    static bool set_factory(const std::shared_ptr<sklk_phy_mod_loader_factory> &factory) {
        mimo_rrh_scheduler::set_mod_loader_factory(factory);
        return true;
    }
};

struct sklk_phy_mod_enable_radio_msg_t
{
    size_t frame_time;  ///< Monotonically increasing time based on the number of frames sent
    size_t radio_ch;    ///< Base station radio channel
    bool enable;        ///< true to enable, false to disable
};

struct sklk_phy_mod_cc_msg_t
{
    size_t frame_time;      ///< Monotonically increasing time based on the number of frames sent
    size_t radio_ch;        ///< Base station radio channel
    size_t resource_blk_no; ///< The frequency resource block number for the CSI information
    size_t est_idx;         ///< The estimation index of the sub-carrier for the CSI
    sklk_mii_cf_t value;    ///< The calibration coefficient
};

struct sklk_phy_mod_csi_msg_t
{
    size_t frame_time;          ///< monotonic frame time of the update
    size_t key;                 ///< An identifier for the UE radio
    sklk_phy_ue_radio ue_radio; ///< The device with new CSI
    size_t resource_blk_no;     ///< The frequency resource block number index for the CSI information
    size_t est_idx;             ///< The estimation index of the sub-carrier for the CSI
    sklk_phy_csi_vec vec;       ///< the array of CSI for each base station radio

};

struct sklk_phy_mod_schedule_request_msg_t
{
    size_t frame_time;  ///< Current frame time
    uint8_t sfn;        ///< Frame to schedule
};

using sklk_phy_mod_ue_update_msg_t = std::pair<size_t, std::shared_ptr<mimo_rrh_radio_unit>>;
using sklk_phy_mod_ue_radio_update_msg_t = std::pair<size_t, std::shared_ptr<mimo_rrh_connection>>;
using sklk_phy_mod_ue_stream_update_msg_t = std::pair<size_t, std::shared_ptr<mimo_rrh_connection>>;

/**
 * Base object for all modules.
 *
 * @headerfile modding.hpp <sklkphy/modding.hpp>
 */
class sklk_phy_modding
{
    static size_t constexpr _max_message_depth{100};
    // Asserts could happen if we cannot keep up with this amount of frame backlog.
    static constexpr size_t _max_frame_delay{10};
    const std::string _name;

    std::atomic_bool _going{true};
    std::vector<sklk_mii_safe_thread::uptr_t> _threads;

    std::atomic_size_t _last_frame_time{0};
    sklk_mii_message_queue<size_t, _max_frame_delay> _frame_start_queue;

    sklk_mii_message_queue<sklk_phy_mod_ue_update_msg_t, _max_message_depth> _ue_update_msgs;
    sklk_mii_message_queue<sklk_phy_mod_ue_radio_update_msg_t, _max_message_depth> _ue_radio_update_msgs;
    sklk_mii_message_queue<sklk_phy_mod_ue_stream_update_msg_t, _max_message_depth> _ue_stream_update_msgs;

    template<typename PointerType>
    static void _cleanup(std::unordered_map<size_t, PointerType> &map);
    void _cleanup();

    template<typename T>
    void _update_map(
        std::unordered_map<size_t, std::weak_ptr<T>> &map,
        sklk_mii_message_queue<std::pair<size_t, std::shared_ptr<T>>, _max_message_depth> &queue,
        void (sklk_phy_modding::*callback  [[maybe_unused]])(size_t, const std::weak_ptr<T> &, bool))
    {
        std::pair<size_t, std::shared_ptr<T>> msg{};
        while (queue.pop(msg)) {
            auto key = std::get<0>(msg);
            auto entry = std::get<1>(msg);
            auto iter = map.find(key);
            bool found = (iter != map.end());
            bool inserted = false;
            if (entry != nullptr) {
                if (not found) {
                    auto inserter = map.insert(msg);
                    assert(inserter.second);
                    inserted = true;
                } else {
                    iter->second = entry;
                }
            } else if (found) {
                map.erase(iter);
            }
            (*this .* callback)(key, entry, inserted);
        }
    }

protected:
    /**
     * Message queue container for modules. Member queues can only be used to receive messages.
     *
     * Has a one-to-one relationship with sklk_phy_mod_loader::message_queues.
     */
    struct message_queues
    {
        sklk_mii_message_queue<sklk_phy_mod_enable_radio_msg_t,
            SKLK_PHY_MAX_RADIOS * _max_frame_delay> enable_radio;
        sklk_mii_message_queue<sklk_phy_mod_cc_msg_t, SKLK_PHY_MAX_RADIOS * _max_frame_delay> cc;
        sklk_mii_message_queue<sklk_phy_mod_csi_msg_t,
            SKLK_PHY_MAX_BANDS * SKLK_PHY_MAX_ESTIMATIONS * _max_frame_delay> csi;
        sklk_mii_message_queue<sklk_phy_mod_schedule_request_msg_t, _max_frame_delay> schedule_request;
    } _msg_queues;

public:
    /**
     * Basic constructor for all modules
     *
     * @param name module name
     */
    explicit sklk_phy_modding(std::string name) : _name(std::move(name)) {}

    /**
     * Basic destructor for all modules
     *
     * @param name module name
     */
    virtual ~sklk_phy_modding() = default;

    /**
     * Get the name of this module
     *
     * @return name of the module
     */
    std::string get_name() const {return _name;};

    /**
     * @return The frame time of the last frame that this module finished processing.
     */
    size_t get_last_frame_time() const { return _last_frame_time.load(); }

    /**
     * A map of all currently available UEs
     */
    std::unordered_map<size_t, sklk_phy_ue> ue_map{};

    /**
     * A map of all currently available UE radios
     */
    std::unordered_map<size_t, sklk_phy_ue_radio> ue_radio_map{};

    /**
     * A map of all currently available UE spatial streams
     */
    std::unordered_map<size_t, sklk_phy_ue_stream> ue_stream_map{};

    /**
     * Method called when the UE changes
     *
     * @param key An identifier for the UE
     * @param ue The device
     * @param is_new true if the device was added to the map; false if updated
     */
    virtual void ue_changed(size_t key, const sklk_phy_ue &ue, bool is_new) = 0;

    /**
     * Method called when the UE radio changes
     *
     * @param key An identifier for the UE radio
     * @param ue_radio The device
     * @param is_new true if the device was added to the map; false if updated
     */
    virtual void ue_radio_changed(size_t key, const sklk_phy_ue_radio &ue_radio, bool is_new) = 0;

    /**
     * Method called when the UE spatial stream changes
     *
     * @param key An identifier for the UE spatial stream
     * @param ue_stream The device
     * @param is_new true if the device was added to the map; false if updated
     */
    virtual void ue_stream_changed(size_t key, const sklk_phy_ue_stream &ue_stream, bool is_new) = 0;

    /**
     * Method called when the UE spatial stream changes
     *
     * @return true if the function should be executed again, false if it can wait.
     */
    virtual bool run_once() = 0;

    /**
     * UE internal storage allocator
     *
     * Callback when memory needs to be allocated for a new UE.  This is internal storage
     * associated with the UE and can be retrieved using the UE handle.  This function
     * must be thread-safe and fast.
     *
     * @return shared memory to a container of internal storage
     */
    virtual sklk_phy_mod_container_ptr_t allocate_ue() {return nullptr;}

    /**
     * UE radio internal storage allocator
     *
     * Callback when memory needs to be allocated for a new UE radio.  This is internal storage
     * associated with the UE radio and can be retrieved using the UE radio handle.  This function
     * must be thread-safe and fast.
     *
     * @return shared memory to a container of internal storage
     */
    virtual sklk_phy_mod_container_ptr_t allocate_ue_radio() {return nullptr;}

    /**
     * UE stream internal storage allocator
     *
     * Callback when memory needs to be allocated for a new UE stream.  This is internal storage
     * associated with the UE stream and can be retrieved using the UE stream handle.  This function
     * must be thread-safe and fast.
     *
     * @return shared memory to a container of internal storage
     */
    virtual sklk_phy_mod_container_ptr_t allocate_ue_stream(bool is_downlink);

private:
    void _loop();
    void _start();
    void _stop();
    void _update_maps();
    bool is_going() {return _going;}

    friend sklk_phy_mod_loader;
};

/**
 * Base object for loading and using a series of modules.
 *
 * @headerfile modding.hpp <sklkphy/modding.hpp>
 */
class sklk_phy_mod_loader
{
public:
    template<class Message>
    class message_queue
    {
    private:
        std::vector<std::shared_ptr<sklk_phy_modding>> _subscribers{};
        std::function<void(sklk_phy_modding *, Message &&)> _forward_function;

        explicit message_queue(const std::function<void(sklk_phy_modding *, Message &&)> &forward_function)
            : _forward_function(forward_function) {}

        explicit message_queue(std::function<void(sklk_phy_modding *, Message &&)> &&forward_function)
            : _forward_function(std::move(forward_function)) {}

        friend sklk_phy_mod_loader;

    public:
        /**
         * Send a message to the queue
         */
        template<class... Args>
        void send(Args &&... args)
        {
            for (const auto &subscriber: _subscribers)
            {
                auto message = Message{std::forward<Args>(args)...};
                _forward_function(subscriber.get(), std::move(message));
            }
        }

        void subscribe(const std::shared_ptr<sklk_phy_modding> &mod)
        {
            _subscribers.emplace_back(mod);
        }
    };

private:
    // Asserts could happen if we cannot keep up with this amount of frame backlog.
    static constexpr size_t _max_frame_delay{10};
    mimo_rrh_scheduler::impl *_pimpl{nullptr};

    ////////////////////////////////////////////////////////////////////
    // Group map
    ////////////////////////////////////////////////////////////////////
    using sklk_phy_ue_group_uid = std::bitset<SKLK_PHY_MAX_USERS>;
    std::vector<std::unordered_map<sklk_phy_ue_group_uid, sklk_phy_ue_group>> _dl_groups{};
    std::vector<std::unordered_map<sklk_phy_ue_group_uid, sklk_phy_ue_group>> _ul_groups{};
    std::shared_ptr<mimo_weight_group> _get_ue_group(
        size_t frame_time, size_t resource_blk_no, bool is_downlink, const std::vector<sklk_phy_ue_stream> &ue_streams);

    ////////////////////////////////////////////////////////////////////
    // These are used internal to loader API
    ////////////////////////////////////////////////////////////////////
    std::unordered_map<std::string, std::shared_ptr<sklk_phy_modding>> _modules;

    using schedule_response_t = std::tuple<size_t, uint8_t, size_t, sklk_phy_weight_page_id_t>;

    sklk_mii_message_queue<schedule_response_t, _max_frame_delay*SKLK_PHY_MAX_BANDS> _dl_schedule_response_queue;
    sklk_mii_message_queue<schedule_response_t, _max_frame_delay*SKLK_PHY_MAX_BANDS> _ul_schedule_response_queue;

    const size_t _dl_subframes;
    const size_t _ul_subframes;
    const sklk_phy_mcs_to_snr_table _dl_mcs_table;
    const sklk_phy_mcs_to_snr_table _ul_mcs_table;

#define SKLK_PHY_MOD_MESSAGE_QUEUE_NO_WAKE(MSG_T, NAME, FWD_Q)                      \
    message_queue<MSG_T> NAME {                                        \
        [](sklk_phy_modding *mod, MSG_T &&message)                                  \
        {                                                                           \
            auto ok = mod->FWD_Q.send_no_wake(std::move(message));                  \
            assert(ok);                                                             \
        }                                                                           \
    }

#define SKLK_PHY_MOD_MESSAGE_QUEUE_WAKING(MSG_T, NAME, FWD_Q)                       \
    message_queue<MSG_T> NAME {                                        \
        [](sklk_phy_modding *mod, MSG_T &&message)                                  \
        {                                                                           \
            auto ok = mod->FWD_Q.send(std::move(message));                          \
            assert(ok);                                                             \
        }                                                                           \
    }

    SKLK_PHY_MOD_MESSAGE_QUEUE_WAKING(size_t, _frame_start_queue, _frame_start_queue);

    SKLK_PHY_MOD_MESSAGE_QUEUE_NO_WAKE(sklk_phy_mod_ue_update_msg_t, _ue_update_msgs, _ue_update_msgs);
    SKLK_PHY_MOD_MESSAGE_QUEUE_NO_WAKE(sklk_phy_mod_ue_radio_update_msg_t, _ue_radio_update_msgs, _ue_radio_update_msgs);
    SKLK_PHY_MOD_MESSAGE_QUEUE_NO_WAKE(sklk_phy_mod_ue_stream_update_msg_t, _ue_stream_update_msgs, _ue_stream_update_msgs);

private:
    ////////////////////////////////////////////////////////////////////
    // These are functions accessed by scheduler
    ////////////////////////////////////////////////////////////////////
    void set_pimpl(mimo_rrh_scheduler::impl *pimpl) {_pimpl = pimpl;}

    using schedule_response_callback = void (mimo_rrh_scheduler::impl::*)(size_t frame_time, uint8_t sfn, size_t bandno, bool is_downlink, void *ptr);

    void allocate_ue(sklk_phy_mod_container_map &map);
    void allocate_ue_radio(sklk_phy_mod_container_map &map);
    void allocate_ue_stream(sklk_phy_mod_container_map &map, bool is_downlink);
    void start();
    void stop();
    friend mimo_sched_main;
    friend mimo_sched_bookkeep;
    friend mimo_rrh_scheduler::impl;
    friend mimo_sched_link_grant;
    friend mimo_sched_pilot;

protected:
    /**
     * Message queue container for the loader. Member queues can only be subscribed to or sent messages.
     *
     * Has a one-to-one relationship with sklk_phy_modding::message_queues.
     */
    struct message_queues
    {
        SKLK_PHY_MOD_MESSAGE_QUEUE_NO_WAKE(sklk_phy_mod_enable_radio_msg_t, enable_radio, _msg_queues.enable_radio);
        SKLK_PHY_MOD_MESSAGE_QUEUE_NO_WAKE(sklk_phy_mod_cc_msg_t, cc, _msg_queues.cc);
        SKLK_PHY_MOD_MESSAGE_QUEUE_NO_WAKE(sklk_phy_mod_csi_msg_t, csi, _msg_queues.csi);
        SKLK_PHY_MOD_MESSAGE_QUEUE_NO_WAKE(sklk_phy_mod_schedule_request_msg_t, schedule_request, _msg_queues.schedule_request);
    } _msg_queues;

#undef SKLK_PHY_MOD_MESSAGE_QUEUE_NO_WAKE
#undef SKLK_PHY_MOD_MESSAGE_QUEUE_WAKING

    /**
     * Method called to add a new module
     */
    void add_module(std::shared_ptr<sklk_phy_modding> &&ptr);

    /**
     * Get a module by name
     *
     * @param name The module name
     *
     * @return The module
     */
    SKLK_MII_NODISCARD std::shared_ptr<sklk_phy_modding> get_module(const std::string &name) const;

public:
    /**
     * Basic constructor for the module loader
     *
     * @param config The scheduler config file
     */
    explicit sklk_phy_mod_loader(const mimo_rrh_scheduler_config &config);

    /**
     * Basic destructor for the module loader
     */
    virtual ~sklk_phy_mod_loader() = default;

    /**
     * Get a weight page for the given resource block number.
     *
     * This function is not thread-safe and should only be called from one module
     *
     * @param frame_time Monotonically increasing time based on the number of frames sent
     * @param resource_blk_no The frequency resource block number index for the CSI information
     * @param is_downlink true downlink; false for uplink
     * @param ue_streams a vector of UE spatial streams
     */
    sklk_phy_weight_page_initializer get_weight_page(size_t frame_time, size_t resource_blk_no, bool is_downlink, const std::vector<sklk_phy_ue_stream> &ue_streams);

    /**
     * Send the schedule for the given frame.
     *
     * @param frame_time Monotonically increasing time based on the number of frames sent
     * @param sfn The frame to schedule
     * @param resource_blk_no The frequency resource block number index for the CSI information
     * @param is_downlink true downlink; false for uplink
     * @param page_hdl the page to schedule
     */
    void send_schedule_response(size_t frame_time, uint8_t sfn, size_t resource_blk_no, bool is_downlink, const sklk_phy_weight_page_id_t &page_hdl);

    /**
     * Add commands to a JSON RPC server.
     *
     * @param rpc_server The server
     */
    virtual void add_rpc_commands(jsonrpccxx::JsonRpc2Server &rpc_server [[maybe_unused]]) {}

    /**
     * Update the RPC thread.
     */
    virtual void rpc_get_updates() {}
};

/**
 * Static methods to translate one type to another
 *
 * @headerfile modding.hpp <sklkphy/modding.hpp>
 */
class sklk_phy_mod_ue_access
{
public:
    /**
     * Get the number of bytes enqueued for a given UE.
     *
     * @param ue The UE
     * @param is_downlink true downlink; false for uplink
     */
    SKLK_MII_NODISCARD static size_t get_bytes_enqueued(const sklk_phy_ue &ue, bool is_downlink);

    /**
     * Get the opaque container for a UE.
     *
     * @param name Name of the module
     * @param ue The UE
     *
     * @return a container for that UE for the module
     */
    SKLK_MII_NODISCARD static sklk_phy_mod_container_ptr_t get_container(const std::string &name, const sklk_phy_ue &ue);

    /**
     * Get the opaque container for a UE radio.
     *
     * @param name Name of the module
     * @param ue_radio The UE radio
     *
     * @return a container for that UE radio for the module
     */
    SKLK_MII_NODISCARD static sklk_phy_mod_container_ptr_t get_container(const std::string &name, const sklk_phy_ue_radio &ue_radio);

    /**
     * Get the opaque container for UE spatial stream.
     *
     * @param name Name of the module
     * @param ue_stream The UE spatial stream
     * @param is_downlink true downlink; false for uplink
     *
     * @return a container for that UE spatial stream for the module
     */
    SKLK_MII_NODISCARD static sklk_phy_mod_container_ptr_t get_container(const std::string &name, const sklk_phy_ue_radio &ue_stream, bool is_downlink);

    /**
     * Get a name for the UE.
     *
     * @param ue The UE
     *
     * @return a container for that UE for the module
     */
    SKLK_MII_NODISCARD static std::string get_identifier(const sklk_phy_ue &ue);

    /**
     * Get a name for the UE radio.
     *
     * @param ue_radio The UE radio
     *
     * @return a container for that UE radio for the module
     */
    SKLK_MII_NODISCARD static std::string get_identifier(const sklk_phy_ue_radio &ue_radio);
};

/**
 * Static methods to use the weight pages
 *
 * @headerfile modding.hpp <sklkphy/modding.hpp>
 */
class sklk_phy_mod_page_access
{
public:
    /**
     * Get the weight page from the handle.
     *
     * @param page_hdl The handle for the weight page
     *
     * @return the weight page itself
     */
    SKLK_MII_NODISCARD static sklk_phy_weight_page * get_page_from_hdl(const mimo_weight_page_id_t &page_hdl);

    /**
     * Sets the results of weight page calculation.
     *
     * @param frame_time Monotonically increasing time based on the number of frames sent
     * @param page_hdl The handle for the weight page
     * @param is_good true if the calculation was successful, false otherwise
     */
    static void set_page_status(size_t frame_time, const sklk_phy_weight_page_id_t &page_hdl, bool is_good);

    /**
     * Sets the results of weight page calculation.
     *
     * @param page_hdl The handle for the weight page
     *
     * @return true if the page is usable, false otherwise.
     */
    SKLK_MII_NODISCARD static bool page_is_valid(const mimo_weight_page_id_t &page_hdl);

    /**
     * Gets the spatial stream handles.
     *
     * @param page_hdl The handle for the weight page
     *
     * @return the spatial streams.
     */
    SKLK_MII_NODISCARD static std::vector<sklk_phy_ue_radio> get_ue_streams(const mimo_weight_page_id_t &page_hdl);


    /**
     * Dumps the group details for the page.
     *
     * @param page_hdl The handle for the weight page
     *
     * @return Returns a json object describing the group that the page references
     */
    SKLK_MII_NODISCARD static nlohmann::json dump_group(const mimo_weight_page_id_t &page_hdl);
};
