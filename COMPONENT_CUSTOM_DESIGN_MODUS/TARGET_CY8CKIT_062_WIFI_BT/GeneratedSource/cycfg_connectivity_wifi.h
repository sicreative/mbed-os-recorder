/*******************************************************************************
* File Name: cycfg_connectivity_wifi.h
*
* Description:
* Connectivity Wi-Fi configuration
* This file was automatically generated and should not be modified.
* Tools Package 2.1.0.1266
* psoc6pdl 1.4.0.1889
* personalities 1.0.0.0
* udd 1.1.3.51
*
********************************************************************************
* Copyright 2020 Cypress Semiconductor Corporation
* SPDX-License-Identifier: Apache-2.0
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
********************************************************************************/

#if !defined(CYCFG_CONNECTIVITY_WIFI_H)
#define CYCFG_CONNECTIVITY_WIFI_H

#include "cycfg_notices.h"
#include "cy_lpa_compat.h"
#include "cy_lpa_wifi_ol.h"
#include "cy_lpa_wifi_ol_common.h"
#include "cy_lpa_wifi_pf_ol.h"
#include "cycfg_pins.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define CYBSP_WIFI_LP_ENABLED 1U
#define CYCFG_WIFI_HOST_WAKE_ENABLED (1u)
#define CYCFG_WIFI_HOST_WAKE_GPIO CYBSP_WIFI_HOST_WAKE_HAL_PORT_PIN
#define CYCFG_WIFI_HOST_WAKE_IRQ_EVENT CYBSP_WIFI_HOST_WAKE_HAL_IRQ


#if defined(__cplusplus)
}
#endif


#endif /* CYCFG_CONNECTIVITY_WIFI_H */
