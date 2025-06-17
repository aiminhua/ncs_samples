#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_include_directories(app PRIVATE 
  .
  ./transport
)

target_sources_ifndef(CONFIG_DTM_TRANSPORT_HCI app PRIVATE
  transport/dtm_uart_twowire.c
  transport/dtm_uart_wait.c
)

target_sources_ifdef(CONFIG_DTM_TRANSPORT_HCI app PRIVATE transport/dtm_hci.c)

if(CONFIG_DTM_TRANSPORT_HCI AND "${BOARD}" STREQUAL "nrf5340dk")
  target_sources(app PRIVATE transport/hci_uart_remote.c)
  target_include_directories(app PRIVATE rpc)
else()
  target_sources_ifdef(CONFIG_DTM_TRANSPORT_HCI app PRIVATE transport/hci_uart.c)
endif()

target_sources(app PRIVATE
  dtm.c
  dtm_hw.c
  dtm_main.c
)

