.. _peripheral_uart_smp:

Peripheral UART + SMP
##########################

.. contents::
   :local:
   :depth: 2

This sample supports both peripheral_uart and ble smp. The secondary slot is on the internal Flash.

The Peripheral UART sample demonstrates how to use the :ref:`nus_service_readme`.
It uses the NUS service to send data back and forth between a UART connection and a Bluetooth® LE connection, emulating a serial port over Bluetooth LE.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. note::
   * The boards ``nrf52dk_nrf52810``, ``nrf52840dk_nrf52811``, and ``nrf52833dk_nrf52820`` only support the `Minimal sample variant`_.
   * When used with :ref:`zephyr:thingy53_nrf5340`, the sample supports the MCUboot bootloader with serial recovery and SMP DFU over Bluetooth.
     Thingy:53 has no built-in SEGGER chip, so the UART 0 peripheral is not gated to a USB CDC virtual serial port.

The sample also requires a smartphone or tablet running a compatible application.
The `Testing`_ instructions refer to `nRF Connect for Mobile`_, but you can also use other similar applications (for example, `nRF Blinky`_ or `nRF Toolbox`_).

You can also test the application with the :ref:`central_uart` sample.
See the documentation for that sample for detailed instructions.

.. note::
   |thingy53_sample_note|

   The sample also enables an additional USB CDC ACM port that is used instead of UART 0.
   Because of that, it uses a separate USB Vendor and Product ID.

Overview
********

When connected, the sample forwards any data received on the RX pin of the UART 0 peripheral to the Bluetooth LE unit.
On Nordic Semiconductor's development kits, the UART 0 peripheral is typically gated through the SEGGER chip to a USB CDC virtual serial port.

Any data sent from the Bluetooth LE unit is sent out of the UART 0 peripheral's TX pin.

.. note::
   Thingy:53 uses the second instance of USB CDC ACM class instead of UART 0, because it has no built-in SEGGER chip that could be used to gate UART 0.

.. _peripheral_uart_debug:

Debugging
=========

In this sample, the UART console is used to send and read data over the NUS service.
Debug messages are not displayed in this UART console.
Instead, they are printed by the RTT logger.

If you want to view the debug messages, follow the procedure in :ref:`testing_rtt_connect`.

.. note::
   On the Thingy:53, debug logs are provided over the USB CDC ACM class serial port, instead of using RTT.

FEM support
***********

.. include:: /includes/sample_fem_support.txt

.. _peripheral_uart_minimal_ext:

Minimal sample variant
======================

You can build the sample with a minimum configuration as a demonstration of how to reduce code size and RAM usage.
This variant is available for resource-constrained boards.

See :ref:`peripheral_uart_sample_activating_variants` for details.

.. _peripheral_uart_cdc_acm_ext:

USB CDC ACM extension
=====================

For the boards with the USB device peripheral, you can build the sample with support for the USB CDC ACM class serial port instead of the physical UART.
This build uses the sample-specific UART async adapter module that acts as a bridge between USB CDC ACM and Zephyr's UART asynchronous API used by the sample.
See :ref:`peripheral_uart_sample_activating_variants` for details about how to build the sample with this extension using the :file:`prj_cdc.conf`.

Async adapter experimental module
   The default sample configuration uses the UART async API.
   The UART async adapter creates and initializes an instance of the async module.
   This is needed because the USB CDC ACM implementation provides only the interrupt interface.
   The adapter uses data provided in the :c:struct:`uart_async_adapter_data` to connect to the UART device that does not use the asynchronous interface.

   The module requires the :kconfig:option:`CONFIG_BT_NUS_UART_ASYNC_ADAPTER` to be set to ``y``.
   For more information about the adapter, see the :file:`uart_async_adapter` source files available in the :file:`peripheral_uart/src` directory.

User interface
**************

The user interface of the sample depends on the hardware platform you are using.

Development kits
================

LED 1:
   Blinks with a period of 2 seconds, duty cycle 50%, when the main loop is running (device is advertising).

LED 2:
   On when connected.

Button 1:
   Confirm the passkey value that is printed in the debug logs to pair/bond with the other device.

Button 2:
   Reject the passkey value that is printed in the debug logs to prevent pairing/bonding with the other device.

Thingy:53
=========

RGB LED:
   The RGB LED channels are used independently to display the following information:

   * Red channel blinks with a period of two seconds, duty cycle 50%, when the main loop is running (device is advertising).
   * Green channel displays if device is connected.

Button:
   Confirm the passkey value that is printed in the debug logs to pair/bond with the other device.
   Thingy:53 has only one button, therefore the passkey value cannot be rejected by pressing a button.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/peripheral_uart`

.. include:: /includes/build_and_run.txt

.. _peripheral_uart_sample_activating_variants:

Activating sample extensions
============================

To activate the optional extensions supported by this sample, modify :makevar:`OVERLAY_CONFIG` in the following manner:

* For the minimal build variant, set :file:`prj_minimal.conf`.
* For the USB CDC ACM extension, set :file:`prj_cdc.conf`.
  Additionally, you need to set :makevar:`DTC_OVERLAY_FILE` to :file:`usb.overlay`.

See :ref:`cmake_options` for instructions on how to add this option.
For more information about using configuration overlay files, see :ref:`zephyr:important-build-vars` in the Zephyr documentation.

.. _peripheral_uart_testing:

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

1. Connect the kit to the computer using a USB cable. The kit is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. |connect_terminal|
#. Copy ``build*/zephyr/app_update.bin`` to your mobile phone. (If you want to update the net core image, use **net_core_app_update.bin** instead)
#. Open nRF connect for Mobile on your phone. 
#. Connect the board. 
#. Tap **DFU** button on the right top corner of the mobile app.
#. Select **app_update.bin** in your phone. (If you want to update the net core image, use **net_core_app_update.bin** instead)
#. Complete the DFU process.


Dependencies
************

This sample uses the following sample-specific library:

* :file:`uart_async_adapter` at :file:`peripheral_uart/src`

This sample uses the following |NCS| libraries:

* :ref:`nus_service_readme`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* ``boards/arm/nrf*/board.h``
* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

* :ref:`zephyr:api_peripherals`:

   * ``incude/gpio.h``
   * ``include/uart.h``

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/gatt.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/uuid.h``
