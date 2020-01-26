/*
 *------------------------------------------------------------
 *                                  ___ ___ _   
 *  ___ ___ ___ ___ ___       _____|  _| . | |_ 
 * |  _| . |_ -|  _| . |     |     | . | . | '_|
 * |_| |___|___|___|___|_____|_|_|_|___|___|_,_| 
 *                     |_____|       firmware v1                 
 * ------------------------------------------------------------
 * Copyright (c)2020 Ross Bamford
 * See top-level LICENSE.md for licence information.
 *
 * Example and test-harness for Zmodem implementation
 * ------------------------------------------------------------
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "zmodem.h"

#ifndef TEST
static FILE *com;
#endif

#ifndef TEST
static ZHDR hdr_zrinit = {
  .type = 0x01,
  .f0 = 0x00,
  .f1 = 0x00,
  .f2 = 0x00,
  .f3 = 0x00
};

int recv() {
  return fgetc(com);
}

int send(uint8_t chr) {
  return putc((char)chr, com);
}

int main() {
  uint8_t rzr_buf[4];
  uint8_t zrinit_buf[HEX_HDR_STR_LEN + 1];
  zrinit_buf[HEX_HDR_STR_LEN] = 0;

  // Set up zrinit for later use...
  calc_hdr_crc(&hdr_zrinit);
  if (IS_ERROR(to_hex_header(&hdr_zrinit, zrinit_buf, HEX_HDR_STR_LEN))) {
    printf("Failed to make zrinit; Bailing...\n");
    return 1;
  }

  com = fopen("/dev/pts/2", "a+");

  if (com != NULL) {
    printf("Opened port just fine\n");

    while (true) {
      if (await("rz\r", (char*)rzr_buf, 4)) {
        printf("Got rzr...\n");

        if (await_zrqinit()) {
          printf("Got ZRQINIT\n");

          send(ZPAD);
          send(ZPAD);
          send(ZDLE);
          if (send_sz(zrinit_buf) == OK) {
            printf("Send ZRINIT was OK\n");
            printf("Echo next ten chars...");

            for (int i = 0; i < 20; i++) {
              int r = recv();
              printf("Received: 0x%02x", r);

              if (r > 32 && r < 127) {
                printf(" [%c]\n", (uint8_t)r);
              } else {
                printf("\n");
              }
            }

          } else {
            printf("Send ZRINIT was Failure\n");
          }
        } else {
          printf("Didn't get ZRQINIT...\n");
        }
      } else {
        printf("Didn't get rzr, must've got EOF...\n");
      }
      break;
    }
    
    if (fclose(com)) {
      printf("Whoah, close failed!\n");
    } else {
      printf("Closed okay...\n");
    }

  } else {
    printf("Unable to open port\n");
    return 1;
  }
}

#else
#include "acutest.h"

#define RECV_LEN 1024

static char recv_buf[RECV_LEN];
static char *buf_ptr, *buf_limit;

/* Set up the fake receive buffer for use in tests */
uint16_t set_buf(char* buf, int len) {
  buf_ptr = buf_limit = recv_buf;

  if (len > RECV_LEN) {
    return UNSUPPORTED;
  } else {
    for (int i = 0; i < len; i++) {
      *buf_limit++ = buf[i];
    }

    return OK;
  }
}

/* recv implementation for use in tests */
int recv() {
  if (buf_ptr < buf_limit) {
    return *buf_ptr++;
  } else {
    return EOF;
  }
}

/* Tests of the tests */
void test_recv_buffer() {
  TEST_CHECK(set_buf("a", 1025) == UNSUPPORTED);

  set_buf("abc", 3);

  TEST_CHECK(recv() == 'a');
  TEST_CHECK(recv() == 'b');
  TEST_CHECK(recv() == 'c');
  
  TEST_CHECK(recv() == EOF);
  TEST_CHECK(recv() == EOF);
}

/* The actual tests */
void test_is_error() {
  TEST_CHECK(IS_ERROR(0x0000) == false);
  TEST_CHECK(IS_ERROR(0x0001) == false);
  TEST_CHECK(IS_ERROR(0x00ff) == false);
  TEST_CHECK(IS_ERROR(0x0f00) == true);
  TEST_CHECK(IS_ERROR(0xf000) == true);
  TEST_CHECK(IS_ERROR(0xff00) == true);
  TEST_CHECK(IS_ERROR(BAD_DIGIT) == true);
}

void test_get_error_code() {
  TEST_CHECK(ERROR_CODE(0x0000) == 0x0000);
  TEST_CHECK(ERROR_CODE(0x0001) == 0x0000);
  TEST_CHECK(ERROR_CODE(0x00f0) == 0x0000);
  TEST_CHECK(ERROR_CODE(0x00ff) == 0x0000);
  TEST_CHECK(ERROR_CODE(0x0f00) == 0x0f00);
  TEST_CHECK(ERROR_CODE(0xf000) == 0xf000);
  TEST_CHECK(ERROR_CODE(0xff00) == 0xff00);
  TEST_CHECK(ERROR_CODE(0xffc0) == 0xff00);
}

void test_hex_to_nybble() {
  TEST_CHECK(hex_to_nybble('0') == 0x0);
  TEST_CHECK(ERROR_CODE(hex_to_nybble('0')) == 0);
  TEST_CHECK(hex_to_nybble('1') == 0x1);
  TEST_CHECK(ERROR_CODE(hex_to_nybble('1')) == 0);
  TEST_CHECK(hex_to_nybble('2') == 0x2);
  TEST_CHECK(ERROR_CODE(hex_to_nybble('2')) == 0);
  TEST_CHECK(hex_to_nybble('3') == 0x3);
  TEST_CHECK(ERROR_CODE(hex_to_nybble('3')) == 0);
  TEST_CHECK(hex_to_nybble('4') == 0x4);
  TEST_CHECK(ERROR_CODE(hex_to_nybble('4')) == 0);
  TEST_CHECK(hex_to_nybble('5') == 0x5);
  TEST_CHECK(ERROR_CODE(hex_to_nybble('5')) == 0);
  TEST_CHECK(hex_to_nybble('6') == 0x6);
  TEST_CHECK(ERROR_CODE(hex_to_nybble('6')) == 0);
  TEST_CHECK(hex_to_nybble('7') == 0x7);
  TEST_CHECK(ERROR_CODE(hex_to_nybble('7')) == 0);
  TEST_CHECK(hex_to_nybble('8') == 0x8);
  TEST_CHECK(ERROR_CODE(hex_to_nybble('8')) == 0);
  TEST_CHECK(hex_to_nybble('9') == 0x9);
  TEST_CHECK(ERROR_CODE(hex_to_nybble('9')) == 0);
  TEST_CHECK(hex_to_nybble('a') == 0xa);
  TEST_CHECK(ERROR_CODE(hex_to_nybble('a')) == 0);
  TEST_CHECK(hex_to_nybble('b') == 0xb);
  TEST_CHECK(ERROR_CODE(hex_to_nybble('b')) == 0);
  TEST_CHECK(hex_to_nybble('c') == 0xc);
  TEST_CHECK(ERROR_CODE(hex_to_nybble('c')) == 0);
  TEST_CHECK(hex_to_nybble('d') == 0xd);
  TEST_CHECK(ERROR_CODE(hex_to_nybble('d')) == 0);
  TEST_CHECK(hex_to_nybble('e') == 0xe);
  TEST_CHECK(ERROR_CODE(hex_to_nybble('e')) == 0);
  TEST_CHECK(hex_to_nybble('f') == 0xf);
  TEST_CHECK(ERROR_CODE(hex_to_nybble('f')) == 0);
  
  TEST_CHECK(ERROR_CODE(hex_to_nybble('A')) == BAD_DIGIT);
}

void test_hex_to_byte() {
  TEST_CHECK(hex_to_byte('0', '0') == 0x00);
  TEST_CHECK(ERROR_CODE(hex_to_byte('0', '0')) == 0);
  TEST_CHECK(hex_to_byte('0', '1') == 0x01);
  TEST_CHECK(ERROR_CODE(hex_to_byte('0', '1')) == 0);
  TEST_CHECK(hex_to_byte('0', 'e') == 0x0e);
  TEST_CHECK(ERROR_CODE(hex_to_byte('0', 'e')) == 0);
  TEST_CHECK(hex_to_byte('0', 'f') == 0x0f);
  TEST_CHECK(ERROR_CODE(hex_to_byte('0', 'f')) == 0);
  TEST_CHECK(hex_to_byte('1', '0') == 0x10);
  TEST_CHECK(ERROR_CODE(hex_to_byte('1', '0')) == 0);
  TEST_CHECK(hex_to_byte('1', '1') == 0x11);
  TEST_CHECK(ERROR_CODE(hex_to_byte('1', '1')) == 0);
  TEST_CHECK(hex_to_byte('1', 'e') == 0x1e);
  TEST_CHECK(ERROR_CODE(hex_to_byte('1', 'e')) == 0);
  TEST_CHECK(hex_to_byte('1', 'f') == 0x1f);
  TEST_CHECK(ERROR_CODE(hex_to_byte('1', 'f')) == 0);
  TEST_CHECK(hex_to_byte('f', '0') == 0xf0);
  TEST_CHECK(ERROR_CODE(hex_to_byte('f', '0')) == 0);
  TEST_CHECK(hex_to_byte('f', '1') == 0xf1);
  TEST_CHECK(ERROR_CODE(hex_to_byte('f', '1')) == 0);
  TEST_CHECK(hex_to_byte('f', 'e') == 0xfe);
  TEST_CHECK(ERROR_CODE(hex_to_byte('f', 'e')) == 0);
  TEST_CHECK(hex_to_byte('f', 'f') == 0xff);
  TEST_CHECK(ERROR_CODE(hex_to_byte('f', 'f')) == 0);
  
  TEST_CHECK(ERROR_CODE(hex_to_byte('0', 'A')) == BAD_DIGIT);
  TEST_CHECK(ERROR_CODE(hex_to_byte('A', '0')) == BAD_DIGIT);
  TEST_CHECK(ERROR_CODE(hex_to_byte('A', 'A')) == BAD_DIGIT);
  TEST_CHECK(ERROR_CODE(hex_to_byte('N', 'O')) == BAD_DIGIT);
}

void test_nybble_to_hex() {
  TEST_CHECK(nybble_to_hex(0x0) == '0');
  TEST_CHECK(nybble_to_hex(0x1) == '1');
  TEST_CHECK(nybble_to_hex(0x2) == '2');
  TEST_CHECK(nybble_to_hex(0x3) == '3');
  TEST_CHECK(nybble_to_hex(0x4) == '4');
  TEST_CHECK(nybble_to_hex(0x5) == '5');
  TEST_CHECK(nybble_to_hex(0x6) == '6');
  TEST_CHECK(nybble_to_hex(0x7) == '7');
  TEST_CHECK(nybble_to_hex(0x8) == '8');
  TEST_CHECK(nybble_to_hex(0x9) == '9');
  TEST_CHECK(nybble_to_hex(0xa) == 'a');
  TEST_CHECK(nybble_to_hex(0xb) == 'b');
  TEST_CHECK(nybble_to_hex(0xc) == 'c');
  TEST_CHECK(nybble_to_hex(0xd) == 'd');
  TEST_CHECK(nybble_to_hex(0xe) == 'e');
  TEST_CHECK(nybble_to_hex(0xf) == 'f');

  TEST_CHECK(IS_ERROR(nybble_to_hex(0x10)) == true);
  TEST_CHECK(IS_ERROR(nybble_to_hex(0xFF)) == true);
}

void test_byte_to_hex() {
  uint8_t buf[2];

  TEST_CHECK(byte_to_hex(0x00, buf) == OK);
  TEST_CHECK(buf[0] == '0');
  TEST_CHECK(buf[1] == '0');

  TEST_CHECK(byte_to_hex(0x01, buf) == OK);
  TEST_CHECK(buf[0] == '0');
  TEST_CHECK(buf[1] == '1');

  TEST_CHECK(byte_to_hex(0x0f, buf) == OK);
  TEST_CHECK(buf[0] == '0');
  TEST_CHECK(buf[1] == 'f');

  TEST_CHECK(byte_to_hex(0x10, buf) == OK);
  TEST_CHECK(buf[0] == '1');
  TEST_CHECK(buf[1] == '0');

  TEST_CHECK(byte_to_hex(0xff, buf) == OK);
  TEST_CHECK(buf[0] == 'f');
  TEST_CHECK(buf[1] == 'f');
}

void test_read_hex_header() {
  ZHDR hdr;

  // All zeros - CRC is zero
  set_buf("00000000000000", 14);
  TEST_CHECK(read_hex_header(&hdr) == OK);

  TEST_CHECK(hdr.type == 0);
  TEST_CHECK(hdr.f0 == 0);
  TEST_CHECK(hdr.f1 == 0);
  TEST_CHECK(hdr.f2 == 0);
  TEST_CHECK(hdr.f3 == 0);
  TEST_CHECK(hdr.crc1 == 0);
  TEST_CHECK(hdr.crc2 == 0);
 
  // Correct CRC - 01 02 03 04 05 - CRC is 0x6534 
  set_buf("01020304056534", 14);
  TEST_CHECK(read_hex_header(&hdr) == OK);

  TEST_CHECK(hdr.type == 0x01);
  TEST_CHECK(hdr.f0 == 0x02);
  TEST_CHECK(hdr.f1 == 0x03);
  TEST_CHECK(hdr.f2 == 0x04);
  TEST_CHECK(hdr.f3 == 0x05);
  TEST_CHECK(hdr.crc1 == 0x65);
  TEST_CHECK(hdr.crc2 == 0x34);
  
  // Incorrect CRC - 01 02 03 04 05 - CRC is 0x6534, but expect 0xc0c0 
  // Note that header left intact for debugging
  set_buf("0102030405c0c0", 14);
  TEST_CHECK(read_hex_header(&hdr) == BAD_CRC);

  TEST_CHECK(hdr.type == 0x01);
  TEST_CHECK(hdr.f0 == 0x02);
  TEST_CHECK(hdr.f1 == 0x03);
  TEST_CHECK(hdr.f2 == 0x04);
  TEST_CHECK(hdr.f3 == 0x05);
  TEST_CHECK(hdr.crc1 == 0xc0);
  TEST_CHECK(hdr.crc2 == 0xc0);
  
  // Invalid data - 01 02 0Z 04 05 
  // Note that header is undefined
  set_buf("01020Z0405c0c0", 14);
  TEST_CHECK(read_hex_header(&hdr) == BAD_DIGIT);
}

void test_calc_hdr_crc() {
  ZHDR hdr = {
    .type = 0x01,
    .f0 = 0x02,
    .f1 = 0x03,
    .f2 = 0x04,
    .f3 = 0x05
  };

  calc_hdr_crc(&hdr);

  TEST_CHECK(hdr.crc1 == 0x82);
  TEST_CHECK(hdr.crc2 == 0x08);

  TEST_CHECK(CRC(hdr.crc1, hdr.crc2) == 0x8208);

  ZHDR real_hdr = {
    .type = 0x06,
    .f0 = 0x00,
    .f1 = 0x00,
    .f2 = 0x00,
    .f3 = 0x00
  };

  calc_hdr_crc(&real_hdr);

  TEST_CHECK(real_hdr.crc1 == 0xcd);
  TEST_CHECK(real_hdr.crc2 == 0x85);

  TEST_CHECK(CRC(real_hdr.crc1, real_hdr.crc2) == 0xcd85);
}

void test_to_hex_header() {
  ZHDR hdr = {
    .type = 0x01,
    .f0 = 0x02,
    .f1 = 0x03,
    .f2 = 0x04,
    .f3 = 0x05,
    .crc1 = 0x0a,
    .crc2 = 0x0b
  };

  uint8_t buf[0xff];
  memset(buf, 0, 0xff);

  // Too small - buffer not modified
  TEST_CHECK(to_hex_header(&hdr, buf, 0x01) == OUT_OF_SPACE);
  TEST_CHECK(buf[0] == 0);

  // Too small - buffer not modified
  TEST_CHECK(to_hex_header(&hdr, buf, HEX_HDR_STR_LEN - 1) == OUT_OF_SPACE);
  TEST_CHECK(buf[0] == 0);

  // Exactly correct size
  TEST_CHECK(to_hex_header(&hdr, buf, HEX_HDR_STR_LEN) == HEX_HDR_STR_LEN);
  TEST_CHECK(strcmp("B01020304050a0b\r\x8a", (const char*)buf) == 0);

  memset(buf, 0, 0xff);

  // Exactly correct size
  TEST_CHECK(to_hex_header(&hdr, buf, HEX_HDR_STR_LEN) == HEX_HDR_STR_LEN);
  TEST_CHECK(strcmp("B01020304050a0b\r\x8a", (const char*)buf) == 0);

  memset(buf, 0, 0xff);

  // More than enough space
  TEST_CHECK(to_hex_header(&hdr, buf, 0xff) == HEX_HDR_STR_LEN);
  TEST_CHECK(strcmp("B01020304050a0b\r\x8a", (const char*)buf) == 0);
}

TEST_LIST = {
  { "recv_buffer",          test_recv_buffer      },
  { "IS_ERROR",             test_is_error         },
  { "GET_ERROR_CODE",       test_get_error_code   },
  { "hex_to_nybble",        test_hex_to_nybble    },
  { "hex_to_byte",          test_hex_to_byte      },
  { "nybble_to_hex",        test_nybble_to_hex    },
  { "byte_to_hex",          test_byte_to_hex      },
  { "read_hex_header",      test_read_hex_header  },
  { "calc_hdr_crc",         test_calc_hdr_crc     },
  { "to_hex_header",        test_to_hex_header    },
  { NULL, NULL }
};

#endif


