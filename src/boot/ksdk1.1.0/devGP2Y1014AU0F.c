#include "devGP2Y1014AU0F.h"
#include "fsl_tpm_driver.h"
#include "gpio_pins.h"
#include "fsl_port_hal.h"

void initGP2Y1014AU0F()
{
	// Set PTB10 to TPM0_CH1
	GPIO_MAKE_PIN(HW_GPIOB, 10);
	PORT_HAL_SetMuxMode(PORTB_BASE, 10, kPortMuxAlt2);
	// Set configure TPM
	tpm_general_config_t tpm_config = {
		true,  // allow running in debug mode
		false, // disable Global time base
		false, // disable Trigger mode
		false, // Continue counter after overflow
		false, // Don't reload counter on Trigger
		0      // n/a (Trigger mode disabled)
	};
	TPM_DRV_Init(0, &tpm_config); // Apply configuration to TPM0
	TPM_DRV_SetClock(0, kTpmClockSourceModuleMCGIRCLK, kTpmDividedBy1);
	tpm_pwm_param_t pwm_config = {
		kTpmEdgeAlignedPWM, // Operation mode edge aligned
		kTpmHighTrue,       // PWM true-high
		100,                // 100 Hz (10us period)
		3                   // 3% duty cycle (0.3us width)
	};
	TPM_DRV_PwmStart(0, &pwm_config, 1); // Start PWM with config
}
