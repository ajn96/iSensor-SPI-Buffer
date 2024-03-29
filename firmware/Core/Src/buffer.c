/**
  * Copyright (c) Analog Devices Inc, 2020
  * All Rights Reserved.
  *
  * @file		buffer.c
  * @date		3/19/2020
  * @author		A. Nolan (alex.nolan@analog.com)
  * @brief		iSensor-SPI-Buffer buffer implementation
 **/

#include "reg.h"
#include "buffer.h"
#include "stm32f3xx_hal.h"

/** Index for the last buffer output register. This is based on buffer size. Global scope */
uint32_t g_bufLastRegIndex;

/** Number of elements currently stored in the buffer. Global scope */
uint32_t g_bufCount = 0;

/** Number of 32-bit words per buffer entry. Global scope */
uint32_t g_bufNumWords32;

/** The buffer storage (aligned to allow word-wise retrieval of buffer data) */
static uint8_t buf[BUF_SIZE] __attribute__((aligned (32)));

/** Index within buffer array for buffer head */
static volatile uint32_t buf_head = 0;

/** Index within buffer array for buffer tail */
static volatile uint32_t buf_tail = 0;

/** Increment per buffer entry. This is buffer length + 4, padded to multiple of 4 */
static uint32_t buf_increment = 64;

/** Buffer full setting (0 -> stop adding, Not 0 -> replace oldest) */
static uint32_t buf_replaceOldest = 0;

/** Buffer max count (determined once when buffer is initialized) */
static uint32_t buf_maxCount;

/** Position at which buffer needs to wrap around */
static uint32_t buf_lastEntryIndex;

/**
  * @brief Checks if an element can be added to the buffer
  *
  * @return 0 if no element can be added to the buffer, 1 otherwise
  *
  * The return value depends on the replace oldest setting and buffer count
  */
uint32_t Buffer_Can_Add_Element()
{
	/* can always add new element if replace oldest is set */
	if(buf_replaceOldest)
	{
		return 1;
	}

	/* Buffer full and replace oldest set to false */
	if(g_bufCount >= buf_maxCount)
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
  * pointer down. This function is called from the main loop when a buffer
  * dequeue is requested. As such, the ISRs are disabled for the duration
  * of this function execution to prevent issues with g_bufCount being
  * inadvertently changed
  */
uint8_t* Buffer_Take_Element()
{
	uint8_t* buf_addr = buf;

	/* Enter critical */
	__disable_irq();

	/* In FIFO mode take from the tail and move tail down */
	if(g_bufCount)
	{
		/*Get pointer to the current tail entry */
		buf_addr += buf_tail;

		/* Decrement counter and move tail down */
		g_bufCount--;
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
		g_bufCount = 0;
	}
	/* Update buffer count register */
	g_regs[BUF_CNT_0_REG] = g_bufCount;
	g_regs[BUF_CNT_1_REG] = g_regs[BUF_CNT_0_REG];

	/* Exit critical */
	__enable_irq();

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
uint8_t* Buffer_Add_Element()
{
	uint8_t* buf_addr = buf;
	if(g_bufCount < buf_maxCount)
	{
		/* Increment counter */
		g_bufCount++;

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
		g_bufCount = buf_maxCount;
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
void Buffer_Reset()
{
	/* Reset head/tail and count to 0 */
	buf_head = 0;
	buf_tail = 0;
	g_bufCount = 0;

	/* Enforce min/max settings for buffer increment */
	if(g_regs[BUF_LEN_REG] < BUF_MIN_ENTRY)
		g_regs[BUF_LEN_REG] = BUF_MIN_ENTRY;
	if(g_regs[BUF_LEN_REG] > BUF_MAX_ENTRY)
		g_regs[BUF_LEN_REG] = BUF_MAX_ENTRY;

	/* Buffer length must be multiple of 2 (SPI runs in 16 bit mode) */
	g_regs[BUF_LEN_REG] &= ~(0x1);

	/* Set the index for the last buffer entry */
	g_bufLastRegIndex = (BUF_DATA_0_REG + (g_regs[BUF_LEN_REG] >> 1));

	/* Get the buffer size setting (10 bytes added per buffer for 64 bit time stamp, buffer sig) */
	buf_increment = g_regs[BUF_LEN_REG] + 10;

	/* Want each buffer entry to be word aligned for ease of use. Add some extra bytes if needed */
	buf_increment = (buf_increment + 3) & ~(0x3);

	/* Number of 32-bit words per buffer entry */
	g_bufNumWords32 = buf_increment >> 2;

	/* Mask out unused bits in BUF_CONFIG */
	g_regs[BUF_CONFIG_REG] &= BUF_CFG_MASK;

	/* Get replacement setting */
	buf_replaceOldest = (g_regs[BUF_CONFIG_REG] & BUF_CFG_REPLACE_OLDEST);

	/* Find max buffer count and index */
	buf_maxCount = BUF_SIZE / buf_increment;
	buf_lastEntryIndex = (buf_maxCount - 1) * buf_increment;

	/* Reduce max count by 1 to allow for one "empty" space between head and tail when the buffer
	 * is full. This prevents issues when the buffer is full, user dequeues, and the data gets
	 * overwritten by the IMU interrupt before it can be retrieved */
	buf_maxCount--;

	/* Update buffer count register to 0 */
	g_regs[BUF_CNT_0_REG] = 0;
	g_regs[BUF_CNT_1_REG] = 0;

	/* Update buffer max count register */
	g_regs[BUF_MAX_CNT_REG] = buf_maxCount;
}
