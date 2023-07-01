#pragma once

#include <dlfcn.h>
#include <filesystem>
#include <iostream>
#include <random>

#include <sklk-mii/testing/safe_test.hpp>

#include <sklkphy/refplat.hpp>

struct cpe
{
    size_t handle;
    size_t num_bands;
    size_t num_radios;
    size_t num_subpilots;

    explicit cpe(size_t id, size_t num_bands = 8, size_t num_radios = 40, size_t num_subpilots = 4)
        : handle(id), num_bands(num_bands),
          num_radios(num_radios),
          num_subpilots(num_subpilots) {};

    void load_pilots(sklk_phy_refplat &refplat,
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
                    assert(std::abs(pilot) > 0);
                    refplat.set_pilot(handle, offset, bandno, pilot_num, radio, pilot);
                }
            }
        }
    }

    void set_random_pilots(sklk_phy_refplat &refplat, size_t offset) const
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

    void connect(sklk_phy_refplat &refplat) const
    {
        sklk_mii_notice("Connecting CPE %zu...", handle);
        refplat.connect(handle);
    }

    void disconnect(sklk_phy_refplat &refplat) const
    {
        sklk_mii_notice("Disconnecting CPE %zu...", handle);
        refplat.disconnect(handle);
    }

};


class test_fixture
{
private:
    std::filesystem::path _pilots_dir;
    std::filesystem::path _mod_library_path;

    bool test(sklk_phy_refplat &refplat);

public:
    test_fixture(std::filesystem::path pilots_dir, std::filesystem::path mod_library_path)
        : _pilots_dir(std::move(pilots_dir)),
          _mod_library_path(std::move(mod_library_path)) {}

    bool run()
    {
        std::cout << "Mod library: " << _mod_library_path << std::endl;

        TEST_ASSERT_FALSE(_mod_library_path.empty());

        auto lib = dlopen(_mod_library_path.c_str(), RTLD_NOW);
        if (lib == nullptr) throw std::runtime_error("Can't load mod library: " + _mod_library_path.native());

        char path[1024];
        dlinfo(lib, RTLD_DI_ORIGIN, path);
        std::cout << "Loaded mod library from: " << path << std::endl;

        sklk_phy_refplat_config_t config;

        config.instance_id = 1;
        config.num_initial_bands = 8;
        config.num_users = 32;
        config.num_bands = 8;
        config.num_radios = 40;
        config.decode.initial = 0.95;

        sklk_phy_refplat refplat(config);
        return test(refplat);
    }

};

[[nodiscard]] std::vector<std::vector<std::vector<std::vector<std::array<float, 2>>>>>
read_pilots_from_file(const std::filesystem::path &pilots_file)
{
    sklk_mii_assert_with_backtrace(std::filesystem::exists(pilots_file));

    std::ifstream pilots_stream(pilots_file);
    std::vector<std::vector<std::vector<std::vector<std::array<float, 2>>>>> pilots;

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

    return pilots;
}

bool check_total_stats(sklk_phy_refplat &refplat, const nlohmann::json &expected_stats)
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
            TEST_ASSERT_TRUE(datum.contains(key));
            TEST_ASSERT_EQUAL(datum[key], expected_stats[key]);
        }

        return true;
    }

    sklk_mii_error("No total stats returned by refplat");
    return false;
}

int exit_usage(const char *const exe_name)
{
    std::cerr << "####################################################\n";
    std::cerr << "## Usage: " << exe_name << " <path-to-mod-library.so> <pilot-ndjsons-dir>" << std::endl;
    std::cerr << "####################################################\n";
    return EXIT_FAILURE;
}

int main(int argc, const char *argv[])
{
    if (argc != 3)
    {
        return exit_usage(argv[0]);
    }

    std::filesystem::path &&mod_library_path = argv[1];
    std::filesystem::path &&pilots_dir = argv[2];

    if (not std::filesystem::exists(mod_library_path) or not std::filesystem::exists(pilots_dir))
    {
        return exit_usage(argv[0]);
    }

    test_fixture fixture(pilots_dir, mod_library_path);

    fixture.run();

    return exit_test();
}
