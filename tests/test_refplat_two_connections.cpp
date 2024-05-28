#include "refplat_test.hpp"

#include <sklk-mii/simple_log.hpp>

// https://stackoverflow.com/a/37102597
using namespace testing;
MATCHER_P2(IsBetween, a, b,
           std::string(negation ? "isn't" : "is") + " between " + PrintToString(a)
           + " and " + PrintToString(b))
{
    return a <= arg && arg <= b;
}

void RefplatTest::test(sklk_phy_refplat &refplat)
{
    refplat.run_one();

    std::array<cpe, 2> cpes = {cpe(1), cpe(2)};

    std::array<const char *, 2> pilots_files = {"uaa_900795_upl_1_ch2.ndjson", "uaa_900795_upl_1_ch3.ndjson"};

    for (size_t i = 0; i < pilots_files.size(); ++i)
    {
        pilot_test_vec_t pilots;
        read_pilots_from_file(_pilots_dir / pilots_files[i], pilots);
        cpes[0].load_pilots(refplat, pilots[0], i);
    }

    for (size_t offset = 0; offset < 2; ++offset)
        cpes[1].set_random_pilots(refplat, offset);

    refplat.set_downlink_snr(cpes[0].handle, 20, 4, 2);

    for (int i = 0; i < 10; ++i)
        refplat.run_one();

    for (const auto &cpe: cpes)
        cpe.connect(refplat);

    for (int i = 0; i < 1000; ++i)
        refplat.run_one();

    check_total_stats(
        refplat,
        {
            {"num_bands", 32},
            {"num_conns", 4},
        }
    );

    auto bw_stats = refplat.dump_bw(false);

    for (const auto &datum: bw_stats)
    {
        if (datum.contains("handle")) continue;
        sklk_mii_log::notice("Checking...");

        ASSERT_THAT(datum["decode"]["downlink"]["percent"].get<double>(), IsBetween(90, 100));
        ASSERT_THAT(datum["decode"]["uplink"]["percent"].get<double>(), IsBetween(90, 100));
        break;
    }

    for (size_t i = 0; i < cpes.size(); ++i)
    {
        cpes[i].disconnect(refplat);

        for (int f = 0; f < 1000; ++f)
            refplat.run_one();

        check_total_stats(
            refplat,
            {
                {"num_bands", (cpes.size() - i - 1) * 16},
                {"num_conns", (cpes.size() - i - 1) * 2},
            }
        );
    }
}
