#pragma once

#include "display_ops.h"
#include "pca9555.h"
#include <driver/i2c.h>
#include <driver/gpio.h>
#include <sys/time.h>

#define CFG_PIN_POWER_ENABLE        (PCA_PIN_PC10 >> 8)
#define CFG_PIN_POWER_ENABLE_VPOS   (PCA_PIN_PC11 >> 8)
#define CFG_PIN_POWER_ENABLE_VNEG   (PCA_PIN_PC12 >> 8)
#define CFG_PIN_POWER_ENABLE_GL     (PCA_PIN_PC13 >> 8)
#define CFG_PIN_EP_STV              (PCA_PIN_PC14 >> 8)
#define CFG_PIN_POWER_ENABLE_GH     (PCA_PIN_PC15 >> 8)
#define CFG_PIN_EP_MODE             (PCA_PIN_PC16 >> 8)
#define CFG_PIN_EP_OE               (PCA_PIN_PC17 >> 8)

/*
  push_cfg_bit(cfg->ep_output_enable); EP_OE
  push_cfg_bit(cfg->ep_mode); EP_MODE
  push_cfg_bit(cfg->power_enable_gh); CTRL+22V
  push_cfg_bit(cfg->ep_stv); EP_STV

  push_cfg_bit(cfg->power_enable_gl); CTRL-20V
  push_cfg_bit(cfg->power_enable_vneg); CTRL-15V
  push_cfg_bit(cfg->power_enable_vpos); CTRL+15V
  push_cfg_bit(cfg->power_enable); SMPS_CTRL
*/

/*1_0: SPMPS_CTRL 8
1_1: CTRL+15: 9
1_2: CTRL-15: 10
1_3: CTRL-20: 11
1_4: EP_STV: 12
1_5: CTRL+22: 13
1_6: EP_MODE: 14
1_7: EP_OE: 15*/


typedef struct {
  i2c_port_t port;
  bool power_enable : 1;
  bool power_enable_vpos : 1;
  bool power_enable_vneg : 1;
  bool power_enable_gl : 1;
  bool ep_stv : 1;
  bool power_enable_gh : 1;
  bool ep_mode : 1;
  bool ep_output_enable : 1;
} epd_config_register_t;

static void config_reg_init(epd_config_register_t *cfg) {
  cfg->power_enable = false;
  cfg->power_enable_vpos = false;
  cfg->power_enable_vneg = false;
  cfg->power_enable_gl = false;
  cfg->ep_stv = true;
  cfg->power_enable_gh = false;
  cfg->ep_mode = false;
  cfg->ep_output_enable = false;

  ESP_ERROR_CHECK(pca9555_set_config(cfg->port, 0, 1));
}


void IRAM_ATTR busy_delay(uint32_t cycles);


static void push_cfg(epd_config_register_t* reg) {
    uint8_t value = 0x00;
    if (reg->ep_output_enable) value |= CFG_PIN_EP_OE;
    if (reg->ep_mode) value |= CFG_PIN_EP_MODE;
    if (reg->ep_stv) value |= CFG_PIN_EP_STV;
    if (reg->power_enable) value |= CFG_PIN_POWER_ENABLE;
    if (reg->power_enable_vpos) value |= CFG_PIN_POWER_ENABLE_VPOS;
    if (reg->power_enable_vneg) value |= CFG_PIN_POWER_ENABLE_VNEG;
    if (reg->power_enable_gl) value |= CFG_PIN_POWER_ENABLE_GL;
    if (reg->power_enable_gh) value |= CFG_PIN_POWER_ENABLE_GH;

    ESP_ERROR_CHECK(pca9555_set_value(reg->port, value, 1));
}

static void cfg_poweron(epd_config_register_t *cfg) {
  // POWERON
  cfg->power_enable = true;
  push_cfg(cfg);
  busy_delay(100 * 240);
  cfg->power_enable_gl = true;
  push_cfg(cfg);
  busy_delay(500 * 240);
  cfg->power_enable_vneg = true;
  push_cfg(cfg);
  busy_delay(500 * 240);
  cfg->power_enable_gh = true;
  push_cfg(cfg);
  busy_delay(500 * 240);
  cfg->power_enable_vpos = true;
  push_cfg(cfg);
  busy_delay(100 * 240);
  cfg->ep_stv = true;
  push_cfg(cfg);
  // END POWERON
}

static void cfg_deinit(epd_config_register_t* reg) {
    ESP_ERROR_CHECK(pca9555_set_config(reg->port, 0, 1));


    ESP_LOGI("epdiy", "going to sleep.");
}

static void cfg_poweroff(epd_config_register_t *cfg) {
  // POWEROFF
  cfg->power_enable_gh = false;
  cfg->power_enable_vpos = false;
  push_cfg(cfg);
  busy_delay(10 * 240);
  cfg->power_enable_gl = false;
  cfg->power_enable_vneg = false;
  push_cfg(cfg);
  busy_delay(100 * 240);

  cfg->ep_stv = false;
  cfg->ep_output_enable = false;
  cfg->ep_mode = false;
  cfg->power_enable = false;
  push_cfg(cfg);
  // END POWEROFF
}
