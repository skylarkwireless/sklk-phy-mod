#include "utils.hpp"

std::string get_identifier(const std::vector<sklk_phy_ue> &ues)
{
    auto identifier = std::accumulate(ues.begin(), ues.end(),
        std::string{}, [&](const std::string & accum, const sklk_phy_ue &ue){
            return accum + (accum.empty() ? "" : ",") + sklk_phy_mod_ue_access::get_identifier(ue);
        });
    return identifier;
}

std::string get_identifier(const std::vector<sklk_phy_ue_radio> &ue_radios)
{
    auto identifier = std::accumulate(ue_radios.begin(), ue_radios.end(),
        std::string{}, [&](const std::string & accum, const sklk_phy_ue_radio &ue_radio){
            return accum + (accum.empty() ? "" : ",") + sklk_phy_mod_ue_access::get_identifier(ue_radio);
        });
    return identifier;
}
