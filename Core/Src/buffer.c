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
extern volatile uint16_t regs[];

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

/** Buffer mode ( 0 -> FIFO, 1 -> LIFO) */
uint32_t buf_LIFOMode = 0;

/** Buffer full setting (0 -> stop adding, 1 -> replace oldest) */
uint32_t buf_replaceOldest = 0;

/** Buffer max count (determined once when buffer is initialized) */
uint32_t buf_maxCount;

/** Position at which buffer needs to wrap around */
uint32_t buf_lastEntryIndex;

/** Number of 32-bit words per buffer entry */
uint32_t buf_numWords32;

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
  * When running in LIFO mode (stack) this function takes from the head
  * of the buffer and moves the header pointer up. When running in FIFO
  * mode (queue) this function takes from the tail and moves the tail
  * pointer down.
  */
uint8_t* BufTakeElement()
{
	uint8_t* buf_addr = buf;
	if(buf_LIFOMode)
	{
		/* In LIFO mode take from the head and move it up */
		if(buf_count > 1)
		{
			/*Get pointer to the current head entry */
			buf_addr += buf_head;

			/* Decrement counter and move head up */
			buf_count--;
			buf_head -= buf_increment;

			/* Check that buffer tail hasn't wrapped around */
			if(buf_head > buf_lastEntryIndex)
			{
				/* reset head to end of buffer */
				buf_head = buf_lastEntryIndex;
			}
		}
		else
		{
			/*Return pointer to current head and leave in place for 0 and 1 entries */
			buf_addr += buf_head;
			buf_count = 0;
		}
	}
	else
	{
		/* In FIFO mode take from the tail and move tail down */
		if(buf_count > 1)
		{
			/*Get pointer to the current tail entry */
			buf_addr += buf_tail;

			/* Decrement counter and move tail down */
			buf_count--;
			buf_tail += buf_increment;

			/* Check that buffer header hasn't wrapped around
			 * We can check if greater because buf_head is unsigned, so when it goes negative will become very large */
			if(buf_tail > buf_lastEntryIndex)
			{
				/* reset to end of buffer */
				buf_tail = 0;
			}
		}
		else
		{
			/*Return pointer to current tail and leave in place for zero and 1 entries */
			buf_addr += buf_tail;
			buf_count = 0;
		}
	}
	/* Update buffer count register */
	regs[BUF_CNT_0_REG] = buf_count;
	regs[BUF_CNT_1_REG] = regs[BUF_CNT_0_REG];
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
	if(buf_count == 0)
	{
		/* Increment buffer count */
		buf_count++;
		/* Return pointer to current buffer head without moving head */
		buf_addr += buf_head;
	}
	else if(buf_count < buf_maxCount)
	{
		/* Increment counter and move head down */
		buf_count++;
		buf_head += buf_increment;

		/* Check if head has wrapped around */
		if(buf_head > buf_lastEntryIndex)
		{
			buf_head = 0;
		}

		/* Return pointer to current buffer head */
		buf_addr += buf_head;
	}
	else
	{
		buf_count = buf_maxCount;
		/* Buffer is full */
		if(buf_replaceOldest)
		{
			/* Move head down */
			buf_head += buf_increment;
			if(buf_head > buf_lastEntryIndex)
			{
				buf_head = 0;
			}
			/* Move tail down. Tail should be one entry in front of head */
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
	if(regs[BUF_LEN_REG] < BUF_MIN_ENTRY)
		regs[BUF_LEN_REG] = BUF_MIN_ENTRY;
	if(regs[BUF_LEN_REG] > BUF_MAX_ENTRY)
		regs[BUF_LEN_REG] = BUF_MAX_ENTRY;

	/* Get the buffer size setting (4 bytes added per buffer for time stamp) */
	buf_increment = regs[BUF_LEN_REG] + 4;

	/* Want each buffer entry to be word aligned for ease of use. Add some extra bytes if needed */
	buf_increment = (buf_increment + 3) & ~(0x3);

	/* Number of 32-bit words per buffer entry */
	buf_numWords32 = buf_increment >> 2;

	/* Mask out unused bits in BUF_CONFIG */
	regs[BUF_CONFIG_REG] &= 0xFF03;

	/* Get the mode setting */
	buf_LIFOMode = regs[BUF_CONFIG_REG] & 0x1;

	/* Get replacement setting */
	buf_replaceOldest = (regs[BUF_CONFIG_REG] & 0x2) >> 1;

	/* Find max buffer count and index */
	buf_maxCount = BUF_SIZE / buf_increment;
	buf_lastEntryIndex = (buf_maxCount - 1) * buf_increment;

	/* Update buffer count register */
	regs[BUF_CNT_0_REG] = 0;
	regs[BUF_CNT_1_REG] = 0;

	/* Update buffer max count register */
	regs[BUF_MAX_CNT_REG] = buf_maxCount;
}
