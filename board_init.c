#include "py/mphal.h"

void PYBEHLAB_F767_board_early_init(void) {
    // Configure BOR (Brown-Out Reset) to LEVEL3 (~2.7V) on first boot.
    //
    // STM32F767 datasheet only guarantees flash write/erase above 2.7V.
    // With BOR off (factory default), the chip keeps running down to ~1.7V on
    // power-off. Between 2.7V and 1.7V flash writes silently program wrong
    // bits — files appear on the filesystem but their contents are garbage.
    // Symptom: F:\ shows the file but reading it back (e.g. via REPL import)
    // gives SyntaxError because the bytes are corrupt.
    //
    // The vulnerability window scales with erase time. NUCLEO_F767ZI's 96K
    // layout uses sectors 1-3 (32K, ~250ms erase each) so it usually escapes
    // the power-decay window. Layouts that include sector 4 (128K, ~1s erase)
    // hit it routinely.
    //
    // Setting BOR_LEVEL3 makes the chip auto-reset the moment supply drops
    // below ~2.7V — before flash becomes unreliable.
    //
    // See https://github.com/micropython/micropython/issues/4387 (open since
    // 2018, never fixed in upstream).
    //
    // Programmed once on first boot of new firmware; HAL_FLASH_OB_Launch
    // triggers a system reset, after which BOR is at LEVEL3 permanently.
    FLASH_OBProgramInitTypeDef ob = {0};
    HAL_FLASHEx_OBGetConfig(&ob);
    if (ob.BORLevel != OB_BOR_LEVEL3) {
        ob.OptionType = OPTIONBYTE_BOR;
        ob.BORLevel = OB_BOR_LEVEL3;
        HAL_FLASH_Unlock();
        HAL_FLASH_OB_Unlock();
        HAL_FLASHEx_OBProgram(&ob);
        HAL_FLASH_OB_Launch();  // triggers system reset
        // Not reached.
        HAL_FLASH_OB_Lock();
        HAL_FLASH_Lock();
    }

    // Turn off the USB switch
    #define USB_PowerSwitchOn pin_G6
    mp_hal_pin_output(USB_PowerSwitchOn);
    mp_hal_pin_low(USB_PowerSwitchOn);
}
