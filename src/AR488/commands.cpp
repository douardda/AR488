#include <Arduino.h>
#include "AR488.h"
#include "AR488_Layouts.h"
#include "commands.h"


extern union AR488conf AR488;
extern struct AR488state AR488st;
extern Stream *arSerial;


/***** Array containing index of accepted ++ commands *****/
/*
 * Commands without parameters require casting to a pointer
 * requiring a char* parameter. The functon is called with
 * NULL by the command processor.
 *
 * Format: token, mode, function_ptr
 * Mode: 1=device; 2=controller; 3=both;
 */
static cmdRec cmdHidx [] = {
  { "addr",        3, addr_h      },
  { "allspoll",    2, (void(*)(char*)) aspoll_h  },
  { "auto",        2, amode_h     },
  { "clr",         2, (void(*)(char*)) clr_h     },
  { "dcl",         2, (void(*)(char*)) dcl_h     },
  { "default",     3, (void(*)(char*)) default_h },
  { "eoi",         3, eoi_h       },
  { "eor",         3, eor_h       },
  { "eos",         3, eos_h       },
  { "eot_char",    3, eot_char_h  },
  { "eot_enable",  3, eot_en_h    },
  { "ifc",         2, (void(*)(char*)) ifc_h     },
  { "id",          3, id_h        },
  { "idn",         3, idn_h       },
  { "llo",         2, llo_h       },
  { "loc",         2, loc_h       },
  { "lon",         1, lon_h       },
  { "macro",       2, macro_h     },
  { "mode" ,       3, cmode_h     },
  { "ppoll",       2, (void(*)(char*)) ppoll_h   },
  { "read",        2, read_h      },
  { "read_tmo_ms", 2, rtmo_h      },
  { "ren",         2, ren_h       },
  { "repeat",      2, repeat_h    },
  { "rst",         3, (void(*)(char*)) rst_h     },
  { "trg",         2, trg_h       },
  { "savecfg",     3, (void(*)(char*)) save_h    },
  { "setvstr",     3, setvstr_h   },
  { "spoll",       2, spoll_h     },
  { "srq",         2, (void(*)(char*)) srq_h     },
  { "srqauto",     2, srqa_h      },
  { "status",      1, stat_h      },
  { "ton",         1, ton_h       },
  { "ver",         3, ver_h       },
  { "verbose",     3, (void(*)(char*)) verb_h    },
  { "tmbus",       3, tmbus_h     },
  { "xdiag",       3, xdiag_h     }

};


/***** Extract command and pass to handler *****/
void getCmd(char *buffr) {

  char *token;  // Pointer to command token
  char *params; // Pointer to parameters (remaining buffer characters)

  int casize = sizeof(cmdHidx) / sizeof(cmdHidx[0]);
  int i = 0;

#ifdef DEBUG1
  dbSerial->print("getCmd: ");
  dbSerial->print(buffr); dbSerial->print(F(" - length:")); dbSerial->println(strlen(buffr));
#endif

  // If terminator on blank line then return immediately without processing anything
  if (buffr[0] == 0x00) return;
  if (buffr[0] == CR) return;
  if (buffr[0] == LF) return;

  // Get the first token
  token = strtok(buffr, " \t");

#ifdef DEBUG1
  dbSerial->print("getCmd: process token: "); dbSerial->println(token);
#endif

  // Check whether it is a valid command token
  i = 0;
  do {
    if (strcasecmp(cmdHidx[i].token, token) == 0) break;
    i++;
  } while (i < casize);

  if (i < casize) {
    // We have found a valid command and handler
#ifdef DEBUG1
    dbSerial->print("getCmd: found handler for: "); dbSerial->println(cmdHidx[i].token);
#endif
    // If command is relevant to mode then execute it
    if (cmdHidx[i].opmode & AR488.cmode) {
      // If its a command with parameters
      // Copy command parameters to params and call handler with parameters
      params = token + strlen(token) + 1;

      // If command parameters were specified
      if (strlen(params) > 0) {
#ifdef DEBUG1
        dbSerial->print(F("Calling handler with parameters: ")); dbSerial->println(params);
#endif
        // Call handler with parameters specified
        cmdHidx[i].handler(params);
      }else{
        // Call handler without parameters
        cmdHidx[i].handler(NULL);
      }
    }else{
      errBadCmd();
      if (AR488st.isVerb) arSerial->println(F("Command not available in this mode."));
    }

  } else {
    // No valid command found
    errBadCmd();
  }

}

/*************************************/
/***** STANDARD COMMAND HANDLERS *****/
/*************************************/

/***** Show or change device address *****/
void addr_h(char *params) {
  //  char *param, *stat;
  char *param;
  uint16_t val;
  if (params != NULL) {

    // Primary address
    param = strtok(params, " \t");
    if (notInRange(param, 1, 30, val)) return;
    if (val == AR488.caddr) {
      errBadCmd();
      if (AR488st.isVerb) arSerial->println(F("That is my address! Address of a remote device is required."));
      return;
    }
    AR488.paddr = val;
    if (AR488st.isVerb) {
      arSerial->print(F("Set device primary address to: "));
      arSerial->println(val);
    }

    // Secondary address
    AR488.saddr = 0;
    val = 0;
    param = strtok(NULL, " \t");
    if (param != NULL) {
      if (notInRange(param, 96, 126, val)) return;
      AR488.saddr = val;
      if (AR488st.isVerb) {
        arSerial->print("Set device secondary address to: ");
        arSerial->println(val);
      }
    }

  } else {
    arSerial->print(AR488.paddr);
    if (AR488.saddr > 0) {
      arSerial->print(F(" "));
      arSerial->print(AR488.saddr);
    }
    arSerial->println();
  }
}


/***** Show or set read timout *****/
void rtmo_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 1, 32000, val)) return;
    AR488.rtmo = val;
    if (AR488st.isVerb) {
      arSerial->print(F("Set [read_tmo_ms] to: "));
      arSerial->print(val);
      arSerial->println(F(" milliseconds"));
    }
  } else {
    arSerial->println(AR488.rtmo);
  }
}


/***** Show or set end of send character *****/
void eos_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 3, val)) return;
    AR488.eos = (uint8_t)val;
    if (AR488st.isVerb) {
      arSerial->print(F("Set EOS to: "));
      arSerial->println(val);
    };
  } else {
    arSerial->println(AR488.eos);
  }
}


/***** Show or set EOI assertion on/off *****/
void eoi_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val)) return;
    AR488.eoi = val ? true : false;
    if (AR488st.isVerb) {
      arSerial->print(F("Set EOI assertion: "));
      arSerial->println(val ? "ON" : "OFF");
    };
  } else {
    arSerial->println(AR488.eoi);
  }
}


/***** Show or set interface to controller/device mode *****/
void cmode_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val)) return;
    switch (val) {
      case 0:
        AR488.cmode = 1;
        initDevice();
        break;
      case 1:
        AR488.cmode = 2;
        initController();
        break;
    }
    if (AR488st.isVerb) {
      arSerial->print(F("Interface mode set to: "));
      arSerial->println(val ? "CONTROLLER" : "DEVICE");
    }
  } else {
    arSerial->println(AR488.cmode - 1);
  }
}


/***** Show or enable/disable sending of end of transmission character *****/
void eot_en_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val)) return;
    AR488.eot_en = val ? true : false;
    if (AR488st.isVerb) {
      arSerial->print(F("Appending of EOT character: "));
      arSerial->println(val ? "ON" : "OFF");
    }
  } else {
    arSerial->println(AR488.eot_en);
  }
}


/***** Show or set end of transmission character *****/
void eot_char_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 255, val)) return;
    AR488.eot_ch = (uint8_t)val;
    if (AR488st.isVerb) {
      arSerial->print(F("EOT set to ASCII character: "));
      arSerial->println(val);
    };
  } else {
    arSerial->println(AR488.eot_ch, DEC);
  }
}


/***** Show or enable/disable auto mode *****/
void amode_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 3, val)) return;
    if (val > 0 && AR488st.isVerb) {
      arSerial->println(F("WARNING: automode ON can cause some devices to generate"));
      arSerial->println(F("         'addressed to talk but nothing to say' errors"));
    }
    AR488.amode = (uint8_t)val;
    if (AR488.amode < 3) AR488st.aRead = false;
    if (AR488st.isVerb) {
      arSerial->print(F("Auto mode: "));
      arSerial->println(AR488.amode);
    }
  } else {
    arSerial->println(AR488.amode);
  }
}


/***** Display the controller version string *****/
void ver_h(char *params) {
  // If "real" requested
  if (params != NULL && strncmp(params, "real", 3) == 0) {
    arSerial->println(F(FWVER));
    // Otherwise depends on whether we have a custom string set
  } else {
    if (strlen(AR488.vstr) > 0) {
      arSerial->println(AR488.vstr);
    } else {
      arSerial->println(F(FWVER));
    }
  }
}


/***** Address device to talk and read the sent data *****/
void read_h(char *params) {
  // Clear read flags
  AR488st.rEoi = false;
  AR488st.rEbt = false;
  // Read any parameters
  if (params != NULL) {
    if (strlen(params) > 3) {
      if (AR488st.isVerb) arSerial->println(F("Invalid termination character - ignored!"));
    } else if (strncmp(params, "eoi", 3) == 0) { // Read with eoi detection
      AR488st.rEoi = true;
    } else { // Assume ASCII character given and convert to an 8 bit byte
      AR488st.rEbt = true;
      AR488st.eByte = atoi(params);
    }
  }
  if (AR488.amode == 3) {
    // In auto continuous mode we set this flag to indicate we are ready for continuous read
    AR488st.aRead = true;
  } else {
    // If auto mode is disabled we do a single read
    gpibReceiveData();
  }
}


/***** Send device clear (usually resets the device to power on state) *****/
void clr_h() {
  if (addrDev(AR488.paddr, 0)) {
    if (AR488st.isVerb) arSerial->println(F("Failed to address device"));
    return;
  }
  if (gpibSendCmd(GC_SDC))  {
    if (AR488st.isVerb) arSerial->println(F("Failed to send SDC"));
    return;
  }
  if (uaddrDev()) {
    if (AR488st.isVerb) arSerial->println(F("Failed to untalk GPIB bus"));
    return;
  }
  // Set GPIB controls back to idle state
  setGpibControls(CIDS);
}


/***** Send local lockout command *****/
void llo_h(char *params) {
  // NOTE: REN *MUST* be asserted (LOW)
  if (digitalRead(REN)==LOW) {
    // For 'all' send LLO to the bus without addressing any device - devices will show REM
    if (params != NULL) {
      if (0 == strncmp(params, "all", 3)) {
        if (gpibSendCmd(GC_LLO)) {
          if (AR488st.isVerb) arSerial->println(F("Failed to send universal LLO."));
        }
      }
    } else {
      // Address current device
      if (addrDev(AR488.paddr, 0)) {
        if (AR488st.isVerb) arSerial->println(F("Failed to address the device."));
        return;
      }
      // Send LLO to currently addressed device
      if (gpibSendCmd(GC_LLO)) {
        if (AR488st.isVerb) arSerial->println(F("Failed to send LLO to device"));
        return;
      }
      // Unlisten bus
      if (uaddrDev()) {
        if (AR488st.isVerb) arSerial->println(F("Failed to unlisten the GPIB bus"));
        return;
      }
    }
  }
  // Set GPIB controls back to idle state
  setGpibControls(CIDS);
}


/***** Send Go To Local (GTL) command *****/
void loc_h(char *params) {
  // REN *MUST* be asserted (LOW)
  if (digitalRead(REN)==LOW) {
    if (params != NULL) {
      if (strncmp(params, "all", 3) == 0) {
        // Un-assert REN
        setGpibState(0b00100000, 0b00100000, 0);
        delay(40);
        // Simultaneously assert ATN and REN
        setGpibState(0b00000000, 0b10100000, 0);
        delay(40);
        // Unassert ATN
        setGpibState(0b10000000, 0b10000000, 0);
      }
    } else {
      // Address device to listen
      if (addrDev(AR488.paddr, 0)) {
        if (AR488st.isVerb) arSerial->println(F("Failed to address device."));
        return;
      }
      // Send GTL
      if (gpibSendCmd(GC_GTL)) {
        if (AR488st.isVerb) arSerial->println(F("Failed sending LOC."));
        return;
      }
      // Unlisten bus
      if (uaddrDev()) {
        if (AR488st.isVerb) arSerial->println(F("Failed to unlisten GPIB bus."));
        return;
      }
      // Set GPIB controls back to idle state
      setGpibControls(CIDS);
    }
  }
}


/***** Assert IFC for 150 microseconds *****/
/* This indicates that the AR488 the Controller-in-Charge on
 * the bus and causes all interfaces to return to their idle
 * state
 */
void ifc_h() {
  if (AR488.cmode==2) {
    // Assert IFC
    setGpibState(0b00000000, 0b00000001, 0);
    delayMicroseconds(150);
    // De-assert IFC
    setGpibState(0b00000001, 0b00000001, 0);
    if (AR488st.isVerb) arSerial->println(F("IFC signal asserted for 150 microseconds"));
  }
}


/***** Send a trigger command *****/
void trg_h(char *params) {
  char *param;
  uint8_t addrs[15];
  uint16_t val = 0;
  uint8_t cnt = 0;

  // Initialise address array
  for (int i = 0; i < 15; i++) {
    addrs[i] = 0;
  }

  // Read parameters
  if (params == NULL) {
    // No parameters - trigger addressed device only
    addrs[0] = AR488.paddr;
    cnt++;
  } else {
    // Read address parameters into array
    while (cnt < 15) {
      if (cnt == 0) {
        param = strtok(params, " \t");
      } else {
        param = strtok(NULL, " \t");
      }
      if (notInRange(param, 1, 30, val)) return;
      addrs[cnt] = (uint8_t)val;
      cnt++;
    }
  }

  // If we have some addresses to trigger....
  if (cnt > 0) {
    for (int i = 0; i < cnt; i++) {
      // Address the device
      if (addrDev(addrs[i], 0)) {
        if (AR488st.isVerb) arSerial->println(F("Failed to address device"));
        return;
      }
      // Send GTL
      if (gpibSendCmd(GC_GET))  {
        if (AR488st.isVerb) arSerial->println(F("Failed to trigger device"));
        return;
      }
      // Unaddress device
      if (uaddrDev()) {
        if (AR488st.isVerb) arSerial->println(F("Failed to unlisten GPIB bus"));
        return;
      }
    }

    // Set GPIB controls back to idle state
    setGpibControls(CIDS);

    if (AR488st.isVerb) arSerial->println(F("Group trigger completed."));
  }
}


/***** Reset the controller *****/
/*
 * Arduinos can use the watchdog timer to reset the MCU
 * For other devices, we restart the program instead by
 * jumping to address 0x0000. This is not a hardware reset
 * and will not reset a crashed MCU, but it will re-start
 * the interface program and re-initialise all parameters.
 */
void rst_h() {
#ifdef WDTO_1S
  // Where defined, reset controller using watchdog timeout
  unsigned long tout;
  tout = millis() + 2000;
  wdt_enable(WDTO_1S);
  while (millis() < tout) {};
  // Should never reach here....
  if (AR488st.isVerb) {
    arSerial->println(F("Reset FAILED."));
  };
#else
  // Otherwise restart program (soft reset)
#if defined(__AVR__)
  asm volatile ("  jmp 0");
#endif
#endif
}


/***** Serial Poll Handler *****/
void spoll_h(char *params) {
  char *param;
  uint8_t addrs[15];
  uint8_t sb = 0;
  uint8_t r;
  //  uint8_t i = 0;
  uint8_t j = 0;
  uint16_t val = 0;
  bool all = false;
  bool eoiDetected = false;

  // Initialise address array
  for (int i = 0; i < 15; i++) {
    addrs[i] = 0;
  }

  // Read parameters
  if (params == NULL) {
    // No parameters - trigger addressed device only
    addrs[0] = AR488.paddr;
    j = 1;
  } else {
    // Read address parameters into array
    while (j < 15) {
      if (j == 0) {
        param = strtok(params, " \t");
      } else {
        param = strtok(NULL, " \t");
      }
      // The 'all' parameter given?
      if (strncmp(param, "all", 3) == 0) {
        all = true;
        j = 30;
        if (AR488st.isVerb) arSerial->println(F("Serial poll of all devices requested..."));
        break;
        // Read all address parameters
      } else if (strlen(params) < 3) { // No more than 2 characters
        if (notInRange(param, 1, 30, val)) return;
        addrs[j] = (uint8_t)val;
        j++;
      } else {
        errBadCmd();
        if (AR488st.isVerb) arSerial->println(F("Invalid parameter"));
        return;
      }
    }
  }

  // Send Unlisten [UNL] to all devices
  if ( gpibSendCmd(GC_UNL) )  {
#ifdef DEBUG4
    dbSerial->println(F("spoll_h: failed to send UNL"));
#endif
    return;
  }

  // Controller addresses itself as listner
  if ( gpibSendCmd(GC_LAD + AR488.caddr) )  {
#ifdef DEBUG4
    dbSerial->println(F("spoll_h: failed to send LAD"));
#endif
    return;
  }

  // Send Serial Poll Enable [SPE] to all devices
  if ( gpibSendCmd(GC_SPE) )  {
#ifdef DEBUG4
    dbSerial->println(F("spoll_h: failed to send SPE"));
#endif
    return;
  }

  // Poll GPIB address or addresses as set by i and j
  for (int i = 0; i < j; i++) {

    // Set GPIB address in val
    if (all) {
      val = i;
    } else {
      val = addrs[i];
    }

    // Don't need to poll own address
    if (val != AR488.caddr) {

      // Address a device to talk
      if ( gpibSendCmd(GC_TAD + val) )  {

#ifdef DEBUG4
        dbSerial->println(F("spoll_h: failed to send TAD"));
#endif
        return;
      }

      // Set GPIB control to controller active listner state (ATN unasserted)
      setGpibControls(CLAS);

      // Read the response byte (usually device status) using handshake
      r = gpibReadByte(&sb, &eoiDetected);

      // If we successfully read a byte
      if (!r) {
        if (j > 1) {
          // If all, return specially formatted response: SRQ:addr,status
          // but only when RQS bit set
          if (sb & 0x40) {
            arSerial->print(F("SRQ:")); arSerial->print(i); arSerial->print(F(",")); arSerial->println(sb, DEC);
            i = j;
          }
        } else {
          // Return decimal number representing status byte
          arSerial->println(sb, DEC);
          if (AR488st.isVerb) {
            arSerial->print(F("Received status byte ["));
            arSerial->print(sb);
            arSerial->print(F("] from device at address: "));
            arSerial->println(val);
          }
          i = j;
        }
      } else {
        if (AR488st.isVerb) arSerial->println(F("Failed to retrieve status byte"));
      }
    }
  }
  if (all) arSerial->println();

  // Send Serial Poll Disable [SPD] to all devices
  if ( gpibSendCmd(GC_SPD) )  {
#ifdef DEBUG4
    dbSerial->println(F("spoll_h: failed to send SPD"));
#endif
    return;
  }

  // Send Untalk [UNT] to all devices
  if ( gpibSendCmd(GC_UNT) )  {
#ifdef DEBUG4
    dbSerial->println(F("spoll_h: failed to send UNT"));
#endif
    return;
  }

  // Unadress listners [UNL] to all devices
  if ( gpibSendCmd(GC_UNL) )  {
#ifdef DEBUG4
    dbSerial->println(F("spoll_h: failed to send UNL"));
#endif
    return;
  }

  // Set GPIB control to controller idle state
  setGpibControls(CIDS);

  // Set SRQ to status of SRQ line. Should now be unasserted but, if it is
  // still asserted, then another device may be requesting service so another
  // serial poll will be called from the main loop
  if (digitalRead(SRQ) == LOW) {
    AR488st.isSRQ = true;
  } else {
    AR488st.isSRQ = false;
  }
  if (AR488st.isVerb) arSerial->println(F("Serial poll completed."));

}


/***** Return status of SRQ line *****/
void srq_h() {
  //NOTE: LOW=asserted, HIGH=unasserted
  arSerial->println(!digitalRead(SRQ));
}


/***** Set the status byte (device mode) *****/
void stat_h(char *params) {
  uint16_t val = 0;
  // A parameter given?
  if (params != NULL) {
    // Byte value given?
    if (notInRange(params, 0, 255, val)) return;
    AR488.stat = (uint8_t)val;
    if (val & 0x40) {
      setSrqSig();
      if (AR488st.isVerb) arSerial->println(F("SRQ asserted."));
    } else {
      clrSrqSig();
      if (AR488st.isVerb) arSerial->println(F("SRQ un-asserted."));
    }
  } else {
    // Return the currently set status byte
    arSerial->println(AR488.stat);
  }
}


/***** Save controller configuration *****/
void save_h() {
#ifdef E2END
  epWriteData(AR488.db, AR_CFG_SIZE);
  if (AR488st.isVerb) arSerial->println(F("Settings saved."));
#else
  arSerial->println(F("EEPROM not supported."));
#endif
}


/***** Show state or enable/disable listen only mode *****/
void lon_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val)) return;
    AR488st.isRO = val ? true : false;
    if (AR488st.isTO) AR488st.isTO = false; // Talk-only mode must be disabled!
    if (AR488st.isVerb) {
      arSerial->print(F("LON: "));
      arSerial->println(val ? "ON" : "OFF") ;
    }
  } else {
    arSerial->println(AR488st.isRO);
  }
}



/***** Set the SRQ signal *****/
void setSrqSig() {
  // Set SRQ line to OUTPUT HIGH (asserted)
  setGpibState(0b01000000, 0b01000000, 1);
  setGpibState(0b00000000, 0b01000000, 0);
}


/***** Clear the SRQ signal *****/
void clrSrqSig() {
  // Set SRQ line to INPUT_PULLUP (un-asserted)
  setGpibState(0b00000000, 0b01000000, 1);
  setGpibState(0b01000000, 0b01000000, 0);
}


/***********************************/
/***** CUSTOM COMMAND HANDLERS *****/
/***********************************/

/***** All serial poll *****/
/*
 * Polls all devices, not just the currently addressed instrument
 * This is an alias wrapper for ++spoll all
 */
void aspoll_h() {
  //  char all[4];
  //  strcpy(all, "all\0");
  spoll_h((char*)"all");
}


/***** Send Universal Device Clear *****/
/*
 * The universal Device Clear (DCL) is unaddressed and affects all devices on the Gpib bus.
 */
void dcl_h() {
  if ( gpibSendCmd(GC_DCL) )  {
    if (AR488st.isVerb) arSerial->println(F("Sending DCL failed"));
    return;
  }
  // Set GPIB controls back to idle state
  setGpibControls(CIDS);
}


/***** Re-load default configuration *****/
void default_h() {
  initAR488();
}


/***** Show or set end of receive character(s) *****/
void eor_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 15, val)) return;
    AR488.eor = (uint8_t)val;
    if (AR488st.isVerb) {
      arSerial->print(F("Set EOR to: "));
      arSerial->println(val);
    };
  } else {
    if (AR488.eor>7) AR488.eor = 0;  // Needed to reset FF read from EEPROM after FW upgrade
    arSerial->println(AR488.eor);
  }
}


/***** Parallel Poll Handler *****/
/*
 * Device must be set to respond on DIO line 1 - 8
 */
void ppoll_h() {
  uint8_t sb = 0;

  // Poll devices
  // Start in controller idle state
  setGpibControls(CIDS);
  delayMicroseconds(20);
  // Assert ATN and EOI
  setGpibState(0b00000000, 0b10010000, 0);
  //  setGpibState(0b10010000, 0b00000000, 0b10010000);
  delayMicroseconds(20);
  // Read data byte from GPIB bus without handshake
  sb = readGpibDbus();
  // Return to controller idle state (ATN and EOI unasserted)
  setGpibControls(CIDS);

  // Output the response byte
  arSerial->println(sb, DEC);

  if (AR488st.isVerb) arSerial->println(F("Parallel poll completed."));
}


/***** Assert or de-assert REN 0=de-assert; 1=assert *****/
void ren_h(char *params) {
#if defined (SN7516X) && not defined (SN7516X_DC)
  params = params;
  arSerial->println(F("Unavailable")) ;
#else
  // char *stat;
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val)) return;
    digitalWrite(REN, (val ? LOW : HIGH));
    if (AR488st.isVerb) {
      arSerial->print(F("REN: "));
      arSerial->println(val ? "REN asserted" : "REN un-asserted") ;
    };
  } else {
    arSerial->println(digitalRead(REN) ? 0 : 1);
  }
#endif
}


/***** Enable verbose mode 0=OFF; 1=ON *****/
void verb_h() {
  AR488st.isVerb = !AR488st.isVerb;
  arSerial->print("Verbose: ");
  arSerial->println(AR488st.isVerb ? "ON" : "OFF");
}


/***** Set version string *****/
/* Replace the standard AR488 version string with something else
 *  NOTE: some instrument software requires a sepcific version string to ID the interface
 */
void setvstr_h(char *params) {
  uint8_t plen;
  char idparams[64];
  plen = strlen(params);
  memset(idparams, '\0', 64);
  strncpy(idparams, "verstr ", 7);
  strncat(idparams, params, plen);

/*
arSerial->print(F("Plen: "));
arSerial->println(plen);
arSerial->print(F("Params: "));
arSerial->println(params);
arSerial->print(F("IdParams: "));
arSerial->println(idparams);
*/

  id_h(idparams);

/*
  if (params != NULL) {
    len = strlen(params);
    if (len>47) len=47; // Ignore anything over 47 characters
    memset(AR488.vstr, '\0', 48);
    strncpy(AR488.vstr, params, len);
    if (AR488st.isVerb) {
      arSerial->print(F("Changed version string to: "));
      arSerial->println(params);
    };
  }
*/
}


/***** Talk only mode *****/
void ton_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val)) return;
    AR488st.isTO = val ? true : false;
    if (AR488st.isTO) AR488st.isRO = false; // Read-only mode must be disabled in TO mode!
    if (AR488st.isVerb) {
      arSerial->print(F("TON: "));
      arSerial->println(val ? "ON" : "OFF") ;
    }
  } else {
    arSerial->println(AR488st.isTO);
  }
}


/***** SRQ auto - show or enable/disable automatic spoll on SRQ *****/
/*
 * In device mode, when the SRQ interrupt is triggered and SRQ
 * auto is set to 1, a serial poll is conducted automatically
 * and the status byte for the instrument requiring service is
 * automatically returned. When srqauto is set to 0 (default)
 * an ++spoll command needs to be given manually to return
 * the status byte.
 */
void srqa_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 1, val)) return;
    switch (val) {
      case 0:
        AR488st.isSrqa = false;
        break;
      case 1:
        AR488st.isSrqa = true;
        break;
    }
    if (AR488st.isVerb) arSerial->println(AR488st.isSrqa ? "SRQ auto ON" : "SRQ auto OFF") ;
  } else {
    arSerial->println(AR488st.isSrqa);
  }
}


/***** Repeat a given command and return result *****/
void repeat_h(char *params) {

  uint16_t count;
  uint16_t tmdly;
  char *param;

  if (params != NULL) {
    // Count (number of repetitions)
    param = strtok(params, " \t");
    if (strlen(param) > 0) {
      if (notInRange(param, 2, 255, count)) return;
    }
    // Time delay (milliseconds)
    param = strtok(NULL, " \t");
    if (strlen(param) > 0) {
      if (notInRange(param, 0, 30000, tmdly)) return;
    }

    // Pointer to remainder of parameters string
    param = strtok(NULL, "\n\r");
    if (strlen(param) > 0) {
      for (uint16_t i = 0; i < count; i++) {
        // Send string to instrument
        gpibSendData(param, strlen(param));
        delay(tmdly);
        gpibReceiveData();
      }
    } else {
      errBadCmd();
      if (AR488st.isVerb) arSerial->println(F("Missing parameter"));
      return;
    }
  } else {
    errBadCmd();
    if (AR488st.isVerb) arSerial->println(F("Missing parameters"));
  }

}


/***** Run a macro *****/
void macro_h(char *params) {
#ifdef USE_MACROS
  uint16_t val;
  const char * macro;

  if (params != NULL) {
    if (notInRange(params, 0, 9, val)) return;
    //    execMacro((uint8_t)val);
    runMacro = (uint8_t)val;
  } else {
    for (int i = 0; i < 10; i++) {
      macro = (pgm_read_word(macros + i));
      //      arSerial->print(i);arSerial->print(F(": "));
      if (strlen_P(macro) > 0) {
        arSerial->print(i);
        arSerial->print(" ");
      }
    }
    arSerial->println();
  }
#else
  memset(params, '\0', 5);
  arSerial->println(F("Disabled"));
#endif
}


/***** Bus diagnostics *****/
/*
 * Usage: xdiag mode byte
 * mode: 0=data bus; 1=control bus
 * byte: byte to write on the bus
 * Note: values to switch individual bits = 1,2,4,8,10,20,40,80
 */
void xdiag_h(char *params){
  char *param;
  uint8_t mode = 0;
  uint8_t val = 0;

  // Get first parameter (mode = 0 or 1)
  param = strtok(params, " \t");
  if (param != NULL) {
    if (strlen(param)<4){
      mode = atoi(param);
      if (mode>2) {
        arSerial->println(F("Invalid: 0=data bus; 1=control bus"));
        return;
      }
    }
  }
  // Get second parameter (8 bit byte)
  param = strtok(NULL, " \t");
  if (param != NULL) {
    if (strlen(param)<4){
      val = atoi(param);
    }

    if (mode) {   // Control bus
      // Set to required state
      setGpibState(0xFF, 0xFF, 1);  // Set direction
      setGpibState(~val, 0xFF, 0);  // Set state (low=asserted so must be inverse of value)
      // Reset after 10 seconds
      delay(10000);
      if (AR488.cmode==2) {
        setGpibControls(CINI);
      }else{
        setGpibControls(DINI);
      }
    }else{        // Data bus
      // Set to required value
      setGpibDbus(val);
      // Reset after 10 seconds
      delay(10000);
      setGpibDbus(0);
    }
  }

}


/****** Timing parameters ******/

void tmbus_h(char *params) {
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 30000, val)) return;
    AR488.tmbus = val;
    if (AR488st.isVerb) {
      arSerial->print(F("TmBus set to: "));
      arSerial->println(val);
    };
  } else {
    arSerial->println(AR488.tmbus, DEC);
  }
}


/***** Set device ID *****/
/*
 * Sets the device ID parameters including:
 * ++id verstr - version string (same as ++setvstr)
 * ++id name   - short name of device (e.g. HP3478A) up to 15 characters
 * ++id serial - serial number up to 9 digits long
 */
void id_h(char *params) {
  uint8_t dlen = 0;
  char * keyword; // Pointer to keyword following ++id
  char * datastr; // Pointer to supplied data (remaining characters in buffer)
  char serialStr[10];

#ifdef DEBUG10
  arSerial->print(F("Params: "));
  arSerial->println(params);
#endif

  if (params != NULL) {
    keyword = strtok(params, " \t");
    datastr = keyword + strlen(keyword) + 1;
    dlen = strlen(datastr);
    if (dlen) {
      if (strncmp(keyword, "verstr", 6)==0) {
#ifdef DEBUG10
        arSerial->print(F("Keyword: "));
        arSerial->println(keyword);
        arSerial->print(F("DataStr: "));
        arSerial->println(datastr);
#endif
        if (dlen>0 && dlen<48) {
#ifdef DEBUG10
        arSerial->println(F("Length OK"));
#endif
          memset(AR488.vstr, '\0', 48);
          strncpy(AR488.vstr, datastr, dlen);
          if (AR488st.isVerb) arSerial->print(F("VerStr: "));
		  arSerial->println(AR488.vstr);
        }else{
          if (AR488st.isVerb)
			  arSerial->println(F("Length of version string must not exceed 48 characters!"));
          errBadCmd();
        }
        return;
      }
      if (strncmp(keyword, "name", 4)==0) {
        if (dlen>0 && dlen<16) {
          memset(AR488.sname, '\0', 16);
          strncpy(AR488.sname, datastr, dlen);
        }else{
          if (AR488st.isVerb) arSerial->println(F("Length of name must not exceed 15 characters!"));
          errBadCmd();
        }
        return;
      }
      if (strncmp(keyword, "serial", 6)==0) {
        if (dlen < 10) {
          AR488.serial = atol(datastr);
        }else{
          if (AR488st.isVerb) arSerial->println(F("Serial number must not exceed 9 characters!"));
          errBadCmd();
        }
        return;
      }
//      errBadCmd();
    }else{
      if (strncmp(keyword, "verstr", 6)==0) {
        arSerial->println(AR488.vstr);
        return;
      }
      if (strncmp(keyword, "name", 4)==0) {
        arSerial->println(AR488.sname);
        return;
      }
      if (strncmp(keyword, "serial", 6)==0) {
        memset(serialStr, '\0', 10);
        snprintf(serialStr, 10, "%09lu", AR488.serial);  // Max str length = 10-1 i.e 9 digits + null terminator
        arSerial->println(serialStr);
        return;
      }
    }
  }
  errBadCmd();
}


void idn_h(char * params){
  uint16_t val;
  if (params != NULL) {
    if (notInRange(params, 0, 2, val)) return;
    AR488.idn = (uint8_t)val;
    if (AR488st.isVerb) {
      arSerial->print(F("Sending IDN: "));
      arSerial->print(val ? "Enabled" : "Disabled");
      if (val==2) arSerial->print(F(" with serial number"));
      arSerial->println();
    };
  } else {
    arSerial->println(AR488.idn, DEC);
  }
}