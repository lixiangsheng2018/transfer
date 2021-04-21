SET PATH=C:\Keil_v5\ARM\ARMCC\Bin;C:\ProgramData\Oracle\Java\javapath;C:\Program Files (x86)\STMicroelectronics\st_toolset\asm;C:\Windows\system32;C:\Windows;C:\Windows\System32\Wbem;C:\Windows\System32\WindowsPowerShell\v1.0\;C:\Program Files\IDM Computer Solutions\UltraEdit;C:\Program Files (x86)\Microchip\MPLAB C32 Suite\bin;C:\Program Files (x86)\Windows Kits\8.1\Windows Performance Toolkit\;C:\Program Files\MATLAB\R2016a\runtime\win64;C:\Program Files\MATLAB\R2016a\bin;C:\Program Files\MATLAB\R2016a\polyspace\bin;C:\Program Files (x86)\Microchip\xc8\v1.45\bin;C:\Program Files (x86)\HI-TECH Software\PICC\9.83\bin;C:\Program Files (x86)\opencv\build\x64\vc12\bin;C:\Program Files (x86)\opencv\build\x64\vc14\bin;C:\Program Files (x86)\cmake-3.11.2-win64-x64\bin;C:\Program Files\Git\cmd;C:\Program Files (x86)\Nordic Semiconductor\nrf5x\bin\;C:\Program Files (x86)\Nordic Semiconductor\nrf51\bin\;C:\Program Files (x86)\Atmel\sam-ba_2.16\drv\;C:\Program Files (x86)\Atmel\sam-ba_2.16;C:\Program Files\CMake\bin
SET CPU_TYPE=nRF52832_xxAA
SET CPU_VENDOR=Nordic Semiconductor
SET UV2_TARGET=receiver
SET CPU_CLOCK=0x01E84800
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\play.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\receiver.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\rf.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\uart.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\systemtick.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\tipsaudio.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\ima_adpcm.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\queue.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\rtransfer.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\flash.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmAsm" --Via ".\objects\arm_startup_nrf52._ia"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\system_nrf52.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\nrf_drv_common.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\nrf_delay.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\nrf_drv_uart.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\app_uart_fifo.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\app_error.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\app_fifo.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\app_util_platform.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\nrf_assert.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\retarget.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmLink" --Via ".\Objects\receiver.lnp"
"C:\Keil_v5\ARM\ARMCC\Bin\fromelf.exe" ".\Objects\receiver.axf" --i32combined --output ".\Objects\receiver.hex"
