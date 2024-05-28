#include "api.hpp"

#include <sklkphy/modding.hpp>

#include <string>

SKLK_PHY_MOD_REFDESIGN_API std::string get_identifier(const std::vector<sklk_phy_ue> &ues);

SKLK_PHY_MOD_REFDESIGN_API std::string get_identifier(const std::vector<sklk_phy_ue_radio> &ue_radios);
