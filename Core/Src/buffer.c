/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		buffer.c
  * @date		3/19/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer buffer implementation
 **/

#include "buffer.h"

/* register array */
extern volatile uint16_t g_regs[];

/** Index within buffer array for buffer head */
volatile uint32_t buf_head = 0;

/** Index within buffer array for buffer tail */
volatile uint32_t buf_tail = 0;

/** Number of elements currently stored in the buffer */
volatile uint32_t buf_count = 0;

/** The buffer storage (aligned to allow word-wise retrieval of buffer data) */
uint8_t buf[BUF_SIZE] __attribute__((aligned (32)));

/** Increment per buffer entry. This is buffer len + 4, padded to multiple of 4 */
uint32_t buf_increment = 64;

/** Buffer full setting (0 -> stop adding, 1 -> replace oldest) */
uint32_t buf_replaceOldest = 0;

/** Buffer max count (determined once when buffer is initialized) */
uint32_t buf_maxCount;

/** Position at which buffer needs to wrap around */
uint32_t buf_lastEntryIndex;

/** Index for the last buffer output register. This is based on buffer size */
uint32_t buf_lastRegIndex;

/** Number of 32-bit words per buffer entry */
uint32_t buf_numWords32;

/**
  * @brief Checks if an element can be added to the buffer
  *
  * @return 0 if no element can be added to the buffer, 1 otherwise
  *
  * The return value depends on the replace oldest setting and buffer count
  */
uint32_t BufCanAddElement()
{
	/* can always add new element if replace oldest is set */
	if(buf_replaceOldest)
	{
		return 1;
	}

	/* Buffer full and replace oldest set to false */
	if(buf_count >= buf_maxCount)
	{
		return 0;
	}

	/* Buf not full, return 1 */
	return 1;
}

/**
  * @brief Take a single element from the buffer
  *
  * @return Pointer to the element retrieved from the buffer
  *
  * In FIFO mode (queue) this function takes from the tail and moves the tail
  * pointer down.
  */
uint8_t* BufTakeElement()
{
	uint8_t* buf_addr = buf;
	/* In FIFO mode take from the tail and move tail down */
	if(buf_count)
	{
		/*Get pointer to the current tail entry */
		buf_addr += buf_tail;

		/* Decrement counter and move tail down */
		buf_count--;
		buf_tail += buf_increment;

		/* Check that buffer tail hasn't wrapped around */
		if(buf_tail > buf_lastEntryIndex)
		{
			/* reset to top of buffer */
			buf_tail = 0;
		}
	}
	else
	{
		/*Return pointer to current tail and leave in place for zero entries */
		buf_addr += buf_tail;
		buf_count = 0;
	}
	/* Update buffer count register */
	g_regs[BUF_CNT_0_REG] = buf_count;
	g_regs[BUF_CNT_1_REG] = g_regs[BUF_CNT_0_REG];
	/* Return pointer to the buffer entry */
	return buf_addr;
}

/**
  * @brief Add an element to the buffer
  *
  * @return Pointer to the new element added to the buffer
  *
  * New elements always get added to the head of the buffer, in
  * both buffer modes. If the buffer is full and ReplaceOldest is set
  * to true, then the head continues moving through the buffer memory.
  * If replace oldest is set to false, the head stays still when the
  * buffer data structure reaches capacity.
  */
uint8_t* BufAddElement()
{
	uint8_t* buf_addr = buf;
	if(buf_count < buf_maxCount)
	{
		/* Increment counter */
		buf_count++;

		/* Set return pointer to current buffer head */
		buf_addr += buf_head;

		/* Move buffer head down */
		buf_head += buf_increment;

		/* Check if head has wrapped around */
		if(buf_head > buf_lastEntryIndex)
		{
			buf_head = 0;
		}
	}
	else
	{
		buf_count = buf_maxCount;
		/* Buffer is full */
		if(buf_replaceOldest)
		{
			/* Set head to current tail */
			buf_head = buf_tail;

			/* Move tail down. Tail should be one entry ahead of head */
			buf_tail += buf_increment;
			if(buf_tail > buf_lastEntryIndex)
			{
				buf_tail = 0;
			}
		}
		/* Return pointer to head */
		buf_addr += buf_head;
	}
	/* Return pointer to write buffer value to */
	return buf_addr;
}

/**
  * @brief Clears the buffer data structure
  *
  * @return void
  *
  * This function resets the buffer to its default state. All
  * stored buffer entries are discarded. The buffer control registers
  * (buffer length, buffer config) are both validated to ensure the
  * buffer is initialized to a valid state.
  */
void BufReset()
{
	/* Reset head/tail and count to 0 */
	buf_head = 0;
	buf_tail = 0;
	buf_count = 0;

	/* Enforce min/max settings for buffer increment */
	if(g_regs[BUF_LEN_REG] < BUF_MIN_ENTRY)
		g_regs[BUF_LEN_REG] = BUF_MIN_ENTRY;
	if(g_regs[BUF_LEN_REG] > BUF_MAX_ENTRY)
		g_regs[BUF_LEN_REG] = BUF_MAX_ENTRY;

	/* Set the index for the last buffer entry */
	buf_lastRegIndex = (BUF_DATA_0_REG + (g_regs[BUF_LEN_REG] >> 1));

	/* Get the buffer size setting (8 bytes added per buffer for time stamp, delta time, sig) */
	buf_increment = g_regs[BUF_LEN_REG] + 8;

	/* Want each buffer entry to be word aligned for ease of use. Add some extra bytes if needed */
	buf_increment = (buf_increment + 3) & ~(0x3);

	/* Number of 32-bit words per buffer entry */
	buf_numWords32 = buf_increment >> 2;

	/* Mask out unused bits in BUF_CONFIG */
	g_regs[BUF_CONFIG_REG] &= 0xFF01;

	/* Get replacement setting */
	buf_replaceOldest = (g_regs[BUF_CONFIG_REG] & 0x1);

	/* Find max buffer count and index */
	buf_maxCount = BUF_SIZE / buf_increment;
	buf_lastEntryIndex = (buf_maxCount - 1) * buf_increment;

	/* Update buffer count register */
	g_regs[BUF_CNT_0_REG] = 0;
	g_regs[BUF_CNT_1_REG] = 0;

	/* Update buffer max count register */
	g_regs[BUF_MAX_CNT_REG] = buf_maxCount;
}
