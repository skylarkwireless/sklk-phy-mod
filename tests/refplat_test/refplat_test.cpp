#include "refplat_test.hpp"

#include <sklkphy/refplat.hpp>

#include <sklk-syscfg/env.hpp>

#include <filesystem>
#include <fstream>

#include <dlfcn.h>

namespace fs = std::filesystem;

//
// CPE
//

void cpe::load_pilots(sklk_phy_refplat &refplat,
                      const std::vector<std::vector<std::vector<std::array<float, 2>>>> &pilots,
                      size_t offset) const
{
    for (auto band_it = pilots.begin(); band_it != pilots.end(); ++band_it)
    {
        const auto bandno = std::distance(pilots.begin(), band_it);
        const auto &pilots_per_band = *band_it;

        for (auto radio_it = pilots_per_band.begin(); radio_it != pilots_per_band.end(); ++radio_it)
        {
            const auto radio = std::distance(pilots_per_band.begin(), radio_it);
            const auto &pilots_per_radio = *radio_it;

            for (auto pilot_it = pilots_per_radio.begin(); pilot_it != pilots_per_radio.end(); ++pilot_it)
            {
                const auto pilot_num = std::distance(pilots_per_radio.begin(), pilot_it);
                sklk_mii_cf_t pilot((*pilot_it)[0], (*pilot_it)[1]);
                ASSERT_GT(std::abs(pilot), 0);
                refplat.set_pilot(handle, offset, bandno, pilot_num, radio, pilot);
            }
        }
    }
}

void cpe::set_random_pilots(sklk_phy_refplat &refplat, size_t offset) const
{
    std::random_device rand_dev;
    std::mt19937 generator(rand_dev());
    std::uniform_real_distribution<float> distr(-0.5, 0.5);

    for (size_t bandno = 0; bandno < num_bands; ++bandno)
    {
        for (size_t radio = 0; radio < num_radios; ++radio)
        {
            for (size_t pno = 0; pno < num_subpilots; ++pno)
            {
                sklk_mii_cf_t cf(distr(generator), distr(generator));
                refplat.set_pilot(handle, offset, bandno, pno, radio, cf);
            }
        }
    }
}

void cpe::connect(sklk_phy_refplat &refplat) const
{
    sklk_mii_log::notice("Connecting CPE {}...", handle);
    refplat.connect(handle);
}

void cpe::disconnect(sklk_phy_refplat &refplat) const
{
    sklk_mii_log::notice("Disconnecting CPE {}...", handle);
    refplat.disconnect(handle);
}

//
// Utility
//

void read_pilots_from_file(
    const fs::path &pilots_file,
    pilot_test_vec_t &pilots)
{
    ASSERT_TRUE(fs::exists(pilots_file));

    std::ifstream pilots_stream(pilots_file);
    std::string line;

    while (std::getline(pilots_stream, line))
    {
        const nlohmann::json &json = nlohmann::json::parse(line);
        std::vector<std::vector<std::vector<std::array<float, 2>>>> pilot{};

        for (const auto &item:json)
        {
            if (not item.is_null())
                pilot.push_back(item.get<std::vector<std::vector<std::array<float, 2>>>>());
        }

        pilots.push_back(pilot);
    }
}

void check_total_stats(sklk_phy_refplat &refplat, const nlohmann::json &expected_stats)
{
    auto data = refplat.dump_bw(true);
    size_t max_index = 0;

    for (const auto &datum: data)
    {
        auto index = datum["index"].get<size_t>();
        if (index > max_index)
            max_index = index;
    }

    for (const auto &datum: data)
    {
        if (datum.contains("handle") or max_index != datum["index"].get<size_t>())
            continue;

        for (const auto &item: expected_stats.items())
        {
            const auto &key = item.key();
            ASSERT_TRUE(datum.contains(key));
            ASSERT_EQ(datum[key], expected_stats[key]);
        }

        return;
    }

    FAIL() << "No total stats returned by reflat";
}


//
// RefplatTest
//

void RefplatTest::SetUp()
{
    static constexpr auto pilots_dir_envvar = "PILOTS_DIR";
    static constexpr auto mod_library_envvar = "MOD_LIBRARY";

    ASSERT_TRUE(sklk_syscfg::env_var_exists(pilots_dir_envvar)) << "Env var: " << pilots_dir_envvar;
    _pilots_dir = *sklk_syscfg::getenv(pilots_dir_envvar);
    ASSERT_TRUE(fs::exists(_pilots_dir)) << _pilots_dir.string();
    sklk_mii_log::notice("Pilots directory: {}", _pilots_dir);

    ASSERT_TRUE(sklk_syscfg::env_var_exists(mod_library_envvar)) << "Env var: " << mod_library_envvar;
    _mod_library_path = *sklk_syscfg::getenv(mod_library_envvar);
    ASSERT_TRUE(fs::exists(_mod_library_path)) << _mod_library_path.string();
    sklk_mii_log::notice("Mod library: {}", _mod_library_path);
}

TEST_F(RefplatTest, RefplatTest)
{
    auto lib = dlopen(this->_mod_library_path.c_str(), RTLD_NOW);
    ASSERT_FALSE(sklkcpptest::pointer_null(lib)) << "Can't load mod library: " << _mod_library_path.string();

    char path[1024];
    dlinfo(lib, RTLD_DI_ORIGIN, path);
    sklk_mii_log::notice("dlinfo: loaded mod library from: {}", path);

    sklk_phy_refplat_config_t config;

    config.instance_id = 1;
    config.num_initial_bands = 8;
    config.num_users = 32;
    config.num_bands = 8;
    config.num_radios = 40;
    config.decode.initial = 0.95;

    sklk_phy_refplat refplat(config);
    this->test(refplat);
}
