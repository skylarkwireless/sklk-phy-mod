#pragma once

#include <sklk-cpptest.hpp>

#include <sklk-mii/simple_log.hpp>

#include <sklkphy/refplat.hpp>

#include <filesystem>
#include <random>

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
                     size_t offset) const;

    void set_random_pilots(sklk_phy_refplat &refplat, size_t offset) const;

    void connect(sklk_phy_refplat &refplat) const;

    void disconnect(sklk_phy_refplat &refplat) const;
};


using pilot_test_vec_t = std::vector<std::vector<std::vector<std::vector<std::array<float, 2>>>>>;

void read_pilots_from_file(
    const std::filesystem::path &pilots_file,
    pilot_test_vec_t &pilots);

void check_total_stats(sklk_phy_refplat &refplat, const nlohmann::json &expected_stats);

class RefplatTest: public ::testing::Test
{
protected:
    std::filesystem::path _pilots_dir;
    std::filesystem::path _mod_library_path;

    void test(sklk_phy_refplat &refplat);

protected:
    void SetUp();
};
