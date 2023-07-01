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
