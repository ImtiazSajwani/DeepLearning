//==============================================================
// Copyright © 2022 Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================
#include <immintrin.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdbool.h>
#include <vector>
#include "test-amxtile.hpp"


#define MAX 1024
// #define MAX_ROWS 16
// #define MAX_COLS 64
#define STRIDE 64
#define ARCH_GET_XCOMP_PERM     0x1022
#define ARCH_REQ_XCOMP_PERM     0x1023
#define XFEATURE_XTILECFG       17
#define XFEATURE_XTILEDATA      18

#define USE_BF16

#ifndef USE_BF16
#define MAX_ROWS 16
#define MAX_COLS 64
#define TILE_K 64           // Number of columns in an A tile or rows in a B tile
typedef int8_t type_t;      // The type of the data being operated on
typedef int32_t res_type_t; // The data type of the result
#define _tdp _tile_dpbssd   // Multiplication operator
#else
#define MAX_ROWS 16
#define MAX_COLS 64
#define STRIDE 64
#define TILE_K 32           // Number of columns in an A tile or rows in a B tile
typedef uint16_t bfloat16;
typedef bfloat16 type_t;    // The type of the data being operated on
typedef float res_type_t;   // The data type of the result
#define _tdp _tile_dpbf16ps // Multiplication operator
#endif

//Define tile config data structure 
typedef struct __tile_config
{
  uint8_t palette_id;
  uint8_t start_row;
  uint8_t reserved_0[14];
  uint16_t colsb[16]; 
  uint8_t rows[16]; 
} __tilecfg;

/* Initialize tile config */
static void init_tile_config (__tilecfg *tileinfo)
{
  int i;
  tileinfo->palette_id = 1;
  tileinfo->start_row = 0;

  for (i = 0; i < 1; ++i)
  {
    tileinfo->colsb[i] = MAX_ROWS;
    tileinfo->rows[i] =  MAX_ROWS;
  }

  for (i = 1; i < 4; ++i)
  {
    tileinfo->colsb[i] = MAX_COLS;
    tileinfo->rows[i] =  MAX_ROWS;
  }

  _tile_loadconfig (tileinfo);
}

template<typename T>
static void init_buffer (T *buf, int8_t value)
{
  int rows, colsb, i, j;
  rows  = MAX_ROWS;
  colsb = MAX_COLS;

  for (i = 0; i < rows; i++)
    for (j = 0; j < colsb; j++)
    {
        //buf[i * colsb + j] = value;
        buf[i * colsb + j] = static_cast <T> (rand()%20);
    }
}

/* Initialize int32_t buffer */
template<typename T>
static void init_buffer32 (T *buf, int32_t value)
{
  int rows, colsb, i, j;
  rows  = MAX_ROWS;
  colsb = MAX_COLS;
  int colsb2=colsb/4;

  for (i = 0; i < rows; i++)
    for (j = 0; j < (colsb2); j++)
    {
        buf[i * colsb2 + j] = value;
    }
}

/* Set_tiledata_use() - Invoke syscall to set ARCH_SET_STATE_USE */
static bool set_tiledata_use()
{
   if (syscall(SYS_arch_prctl, ARCH_REQ_XCOMP_PERM, XFEATURE_XTILEDATA)) 
   {
      printf("\n Fail to do XFEATURE_XTILEDATA \n\n");
      return false;
   }
   else
   {
      printf("\n TILE DATA USE SET - OK \n\n");
      return true;
   }

   return true;
}

template<typename T>
static void print_buffer(T* buf, int32_t rows, int32_t colsb) 
{
   for (int i = 0; i < rows; i++) {
     for (int j = 0; j < (colsb); j++)
     {
         printf("%d ", buf[i * colsb + j]);
     }
     printf("\n");
   }
   printf("\n");
}

/* Print int32_t buffer */
static void print_buffer32(int32_t* buf, int32_t rows, int32_t colsb)
{
   for (int i = 0; i < rows; i++) {
     for (int j = 0; j < (colsb); j++)
     {
         printf("%d ", buf[i * colsb + j]);
     }
     printf("\n");
   }
   printf("\n");
}

int main(){

   __tilecfg tile_data = {0};
   
   #ifndef USE_BF16
   int8_t src1[MAX];
   int8_t src2[MAX];
   int32_t res[MAX/4];
   int rows  = MAX_ROWS;
   int colsb = MAX_COLS;

   // Request permission to linux kernel to run AMX 
   if (!set_tiledata_use())
      exit(-1);

   // Load tile configuration 
   init_tile_config (&tile_data);

   // Init src matrix buffers with data
   init_buffer (src1, 4);
   print_buffer(src1, rows, colsb);
 
   init_buffer (src2, 2);
   print_buffer(src2, rows, colsb);

   // Init dst matrix buffers with data
   init_buffer32 (res, 0);

   // Load tile rows from memory
   _tile_loadd (2, src1, STRIDE);
   _tile_loadd (3, src2, STRIDE);
   _tile_loadd (1, res, STRIDE);

   // Compute dot-product of INT8 bytes in tiles using AMX Tiles
   _tile_dpbssd (1, 2, 3);

   // Store the tile data to memory
   _tile_stored (1, res, STRIDE);
   print_buffer32(res, rows, colsb/4);

   // Release the tile configuration to return to the init state, 
   // which releases all storage it currently holds
   _tile_release ();
  #else
    std::vector<bfloat16> src1bf16[MAX];
    std::vector<bfloat16> src2bf16[MAX];
    float res1bf16[MAX/2];
    //int32_t res1bf16[MAX];
    int rows  = MAX_ROWS;
    int colsb = MAX_COLS;

   // Request permission to linux kernel to run AMX 
   if (!set_tiledata_use())
      exit(-1);
   // Load tile configuration 
   init_tile_config (&tile_data);

   // Init src matrix buffers with data
   init_buffer (src1bf16, 4);
   // print_buffer(src1bf16, rows, colsb);
 
   init_buffer (src2bf16, 2);
   // print_buffer(src2bf16, rows, colsb);   
   // Init dst matrix buffers with data
   init_buffer32 (res1bf16, 0);

   // Load tile rows from memory
   _tile_loadd (2, src1bf16, STRIDE);
   _tile_loadd (3, src2bf16, STRIDE);
   _tile_loadd (1, res1bf16, STRIDE);

   // Compute dot-product of INT8 bytes in tiles using AMX Tiles
   _tile_dpbf16ps (1, 2, 3);

   // Store the tile data to memory
   _tile_stored (1, res1bf16, STRIDE);

   // Release the tile configuration to return to the init state, 
   // which releases all storage it currently holds
   _tile_release ();

  #endif 
}
