#ifndef CB62_NAND_PROTOCOL_H
#define CB62_NAND_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#define CB62_NAND_COMMAND_SIZE 32u
#define CB62_NAND_COMMAND_TOKEN 0x55434d44u
#define CB62_NAND_READY_TO_RECEIVE 0u
#define CB62_NAND_RECEIVE_DATA 1u
#define CB62_NAND_FLAG_LAST_TRANS 0x01u
#define CB62_NAND_FLAG_ADDRESS 0x02u
#define CB62_NAND_FLAG_JUMP_EXEC 0x04u
#define CB62_NAND_FLAG_FORCE_FINISH 0x10u
#define CB62_NAND_PAYLOAD_ADDRESS 0x02000000u

void cb62_nand_build_address_command(uint8_t out[CB62_NAND_COMMAND_SIZE]);
void cb62_nand_build_data_command(uint8_t out[CB62_NAND_COMMAND_SIZE], uint32_t crc);
void cb62_nand_build_jump_command(uint8_t out[CB62_NAND_COMMAND_SIZE]);
uint32_t cb62_nand_crc32(const uint8_t *data, size_t length);

#endif
