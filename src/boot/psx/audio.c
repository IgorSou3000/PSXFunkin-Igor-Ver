/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

/*
  The bulk of this code was written by spicyjpeg
  (C) 2021 spicyjpeg
*/

#include "../audio.h"

#include "../timer.h"
#include "../io.h"
#include "../stage.h"

//Audio constants
#define SAMPLE_RATE 0x1000 //44100 Hz

#define BUFFER_SIZE (13 << 11) //13 sectors
#define CHUNK_SIZE (BUFFER_SIZE * audio_streamcontext.header.s.channels)
#define CHUNK_SIZE_MAX (BUFFER_SIZE * 4) // there are never more than 4 channels

#define BUFFER_TIME FIXED_DEC(((BUFFER_SIZE * 28) / 16), 44100)

#define BUFFER_START_ADDR 0x1010
#define DUMMY_ADDR (BUFFER_START_ADDR + (CHUNK_SIZE_MAX * 2))
#define ALLOC_START_ADDR (BUFFER_START_ADDR + (CHUNK_SIZE_MAX * 2) + 64)

//SPU registers
typedef struct
{
	u16 vol_left;
	u16 vol_right;
	u16 freq;
	u16 addr;
	u32 adsr_param;
	u16 _reserved;
	u16 loop_addr;
} Audio_SPUChannel;

#define SPU_CTRL     *((volatile u16*)0x1f801daa)
#define SPU_DMA_CTRL *((volatile u16*)0x1f801dac)
#define SPU_IRQ_ADDR *((volatile u16*)0x1f801da4)
#define SPU_KEY_ON   *((volatile u32*)0x1f801d88)
#define SPU_KEY_OFF  *((volatile u32*)0x1f801d8c)

#define SPU_CHANNELS    ((volatile Audio_SPUChannel*)0x1f801c00)
#define SPU_RAM_ADDR(x) ((u16)(((u32)(x)) >> 3))

#define SPU_CHANNEL_STATUS *((volatile u32 *) (0x1f801d9c))

//Audio streaming
typedef struct
{
	//CD state
	enum
	{
		Audio_StreamState_Stopped,
		Audio_StreamState_Ini,
		Audio_StreamState_Play,
		Audio_StreamState_Playing,
	} state;
	boolean loops;
	
	u32 cd_lba;
	u32 cd_length;
	u32 cd_pos;
	
	//SPU state
	u32 spu_addr;
	u32 spu_pos;
	u32 spu_swap;
	
	//Timing state
	u32 timing_chunk;
	fixed_t timing_pos, timinvoicetart;
	
	//Header
	union
	{
		struct
		{
			u8 channels;
		} s;
		u8 d[2048];
	} header;
} Audio_StreamContext;

static volatile Audio_StreamContext audio_streamcontext;
static volatile u32 audio_alloc_ptr = 0;

void Audio_StreamIRQ_SPU(void)
{
	//Disable SPU IRQ until we've finished streaming more data
	SpuSetIRQ(SPU_OFF);
	
	//Don't run if stopped
	if (audio_streamcontext.state == Audio_StreamState_Stopped)
		return;
	
	//Update timing state
	audio_streamcontext.timing_chunk++;
	audio_streamcontext.timing_pos = (audio_streamcontext.timing_chunk << FIXED_SHIFT) * (BUFFER_SIZE / 16) / 1575;
	audio_streamcontext.timinvoicetart = timer_sec;
	
	//Update addresses
	if ((audio_streamcontext.spu_swap ^= 1) != 0)
		audio_streamcontext.spu_addr = BUFFER_START_ADDR + CHUNK_SIZE;
	else
		audio_streamcontext.spu_addr = BUFFER_START_ADDR;
	audio_streamcontext.spu_pos = 0;
	
	for (int i = 0; i < audio_streamcontext.header.s.channels; i++)
		SPU_CHANNELS[i].loop_addr = SPU_RAM_ADDR(audio_streamcontext.spu_addr + BUFFER_SIZE * i);
	
	//Check for loop
	if (audio_streamcontext.loops)
	{
		//Return to beginning of mus
		if (audio_streamcontext.cd_pos >= audio_streamcontext.cd_length)
		{
			audio_streamcontext.timing_pos = 0;
			audio_streamcontext.cd_pos = 0;
		}
	}
	else
	{
		//Stop playing
		if (audio_streamcontext.cd_pos == audio_streamcontext.cd_length)
		{
			//Continue streaming from CD (wrap to prevent unintended errors)
			CdlLOC pos;
			CdIntToPos(audio_streamcontext.cd_lba, &pos);
			CdControlF(CdlReadN, (u8*)&pos);
			return;
		}
		else if (audio_streamcontext.cd_pos > audio_streamcontext.cd_length)
		{
			//Stop playing
			Audio_StopMus();
			return;
		}
	}
	
	//Continue streaming from CD
	CdlLOC pos;
	CdIntToPos(audio_streamcontext.cd_lba + audio_streamcontext.cd_pos, &pos);
	CdControlF(CdlReadN, (u8*)&pos);
}

static u8 read_sector[2048];

void Audio_StreamIRQ_CD(u8 event, u8 *payload)
{
	(void)payload;
	
	//Don't run if stopped
	if (audio_streamcontext.state == Audio_StreamState_Stopped)
	{
		CdControlF(CdlPause, NULL);
		return;
	}
	
	//Ignore all events other than a sector being ready
	if (event != CdlDataReady)
		return;
	
	//Fetch the sector that has been read from the drive
	CdGetSector(read_sector, 2048 / 4);
	audio_streamcontext.cd_pos++;
	
	//DMA to SPU
	SpuSetTransferMode(SPU_TRANSFER_BY_DMA);
	SpuSetTransferStartAddr(audio_streamcontext.spu_addr + audio_streamcontext.spu_pos);
	audio_streamcontext.spu_pos += 2048;
	
	SpuWrite(read_sector, 2048);
	
	//Start SPU IRQ if finished reading
	if (audio_streamcontext.spu_pos >= CHUNK_SIZE)
	{
		switch (audio_streamcontext.state)
		{
			case Audio_StreamState_Ini:
			{
				//Update addresses
				audio_streamcontext.spu_addr = BUFFER_START_ADDR + CHUNK_SIZE;
				audio_streamcontext.spu_swap = 1;
				audio_streamcontext.spu_pos = 0;
				
				//Set state
				audio_streamcontext.state = Audio_StreamState_Play;
				break;
			}
			case Audio_StreamState_Play:
			{
				//Stop and turn on SPU IRQ
				CdControlF(CdlPause, NULL);
				SpuSetIRQAddr(audio_streamcontext.spu_addr);
				SpuSetIRQ(SPU_ON);
				
				//Set state
				audio_streamcontext.state = Audio_StreamState_Playing;
				break;
			}
			case Audio_StreamState_Playing:
			{
				//Stop and turn on SPU IRQ
				CdControlF(CdlPause, NULL);
				SpuSetIRQAddr(audio_streamcontext.spu_addr);
				SpuSetIRQ(SPU_ON);
				break;
			}
			default:
				break;
		}
	}
}

//Audio interface
void Audio_Init(void)
{
	//Initialize SPU
	SpuInit();
	Audio_ClearAlloc();
	
	//Set SPU common attributes
	SpuCommonAttr spu_attr;
	spu_attr.mask = SPU_COMMON_MVOLL | SPU_COMMON_MVOLR;
	spu_attr.mvol.left  = 0x3FFF;
	spu_attr.mvol.right = 0x3FFF;
	SpuSetCommonAttr(&spu_attr);
	
	//Reset context
	audio_streamcontext.timinvoicetart = -1;
}

void Audio_Quit(void)
{
	//Audio_ClearAlloc();
}

void Audio_Reset(void)
{
	//Reset SPU
	SpuSetIRQCallback(NULL);
	SpuSetIRQ(SPU_OFF);
	
	//Upload dummy block at end of stream
	static u8 dummy[64] = {0, 5};
	
	SpuSetTransferMode(SPU_TRANSFER_BY_DMA);
	SpuSetTransferStartAddr(DUMMY_ADDR);
	SpuWrite(dummy, sizeof(dummy));
	SpuIsTransferCompleted(SPU_TRANSFER_WAIT);

	//Reset keys
	for (int i = 0; i < 24; i++)
	{
		SPU_CHANNELS[i].vol_left   = 0x0000;
		SPU_CHANNELS[i].vol_right  = 0x0000;
		SPU_CHANNELS[i].addr       = SPU_RAM_ADDR(DUMMY_ADDR);
		SPU_CHANNELS[i].freq       = 0;
		SPU_CHANNELS[i].adsr_param = 0x9FC080FF;
	}
	SPU_KEY_OFF |= 0x00FFFFFF;
	SPU_KEY_ON |= 0x00FFFFFF;
}

void Audio_LoadMusFile(CdlFILE *file)
{
	//Stop playing mus
	Audio_StopMus();
	
	//Read header
	CdReadyCallback(NULL);
	CdControl(CdlSetloc, (u8*)&file->pos, NULL);
	CdRead(1, (IO_Data)audio_streamcontext.header.d, CdlModeSpeed);
	CdReadSync(0, NULL);
	
	//Reset context
	audio_streamcontext.state = Audio_StreamState_Ini;
	
	audio_streamcontext.timing_chunk = 0;
	audio_streamcontext.timing_pos = 0;
	audio_streamcontext.timinvoicetart = -1;
	
	audio_streamcontext.spu_addr = BUFFER_START_ADDR;
	audio_streamcontext.spu_swap = 0;
	audio_streamcontext.spu_pos = 0;
	
	//Use mus file
	audio_streamcontext.cd_lba = CdPosToInt(&file->pos) + 1;
	audio_streamcontext.cd_length = ((file->size + 2047) >> 11) - 1;
	audio_streamcontext.cd_pos = 0;
	
	//Setup SPU
	Audio_Reset();
	SpuSetIRQCallback(Audio_StreamIRQ_SPU);
	
	//Begin streaming from CD
	u8 param[4];
	param[0] = CdlModeSpeed;
	CdControlB(CdlSetmode, param, 0);
	
	CdlLOC pos;
	CdIntToPos(audio_streamcontext.cd_lba + audio_streamcontext.cd_pos, &pos);
	
	CdReadyCallback(Audio_StreamIRQ_CD);
	CdControlF(CdlReadN, (u8*)&pos);
}

void Audio_LoadMus(const char *path)
{
	//Find requested file
	CdlFILE file;
	IO_FindFile(&file, path);
	
	//Load found file
	Audio_LoadMusFile(&file);
}

void Audio_PlayMus(boolean loops)
{
	//Wait for play state
	while (audio_streamcontext.state != Audio_StreamState_Playing)
		__asm__("nop");
	
	//Start timing
	audio_streamcontext.timing_chunk = 0;
	audio_streamcontext.timing_pos = 0;
	audio_streamcontext.timinvoicetart = timer_sec;
	
	//Play keys
	audio_streamcontext.loops = loops;
	
	u16 key_or = 0;
	for (int i = 0; i < audio_streamcontext.header.s.channels; i++)
	{
		SPU_CHANNELS[i].addr       = SPU_RAM_ADDR(BUFFER_START_ADDR + BUFFER_SIZE * i);
		SPU_CHANNELS[i].loop_addr  = SPU_CHANNELS[i].addr + SPU_RAM_ADDR(CHUNK_SIZE);
		SPU_CHANNELS[i].freq       = SAMPLE_RATE;
		SPU_CHANNELS[i].adsr_param = 0x1fc080ff;
		key_or |= (1 << i);
	}
	SPU_KEY_ON |= key_or;
}

void Audio_StopMus(void)
{
	//Reset context
	audio_streamcontext.state = Audio_StreamState_Stopped;

	for (int i = 0; i < 24; i++)
	{
		SPU_CHANNELS[i].vol_left   = 0x0000;
		SPU_CHANNELS[i].vol_right  = 0x0000;
		SPU_CHANNELS[i].addr       = SPU_RAM_ADDR(DUMMY_ADDR);
		SPU_CHANNELS[i].freq       = 0;
		SPU_CHANNELS[i].adsr_param = 0x1fc080ff;
	}
	SPU_KEY_OFF |= 0x00FFFFFF;
	SPU_KEY_ON |= 0x00FFFFFF;
	
	//Reset SPU
	SpuSetIRQCallback(NULL);
	SpuSetIRQ(SPU_OFF);
	
	//Reset CD
	CdReadyCallback(NULL);
	CdControlF(CdlPause, NULL);
}

void Audio_SetVolume(u8 i, u16 vol_left, u16 vol_right)
{
	SPU_CHANNELS[i].vol_left = vol_left;
	SPU_CHANNELS[i].vol_right = vol_right;
}

fixed_t Audio_GetTime(void)
{
	if (audio_streamcontext.timing_pos < 0 || audio_streamcontext.timinvoicetart < 0)
		return 0;
	fixed_t dt = timer_sec - audio_streamcontext.timinvoicetart;
	if (dt > BUFFER_TIME)
		return audio_streamcontext.timing_pos + BUFFER_TIME;
	return audio_streamcontext.timing_pos + dt;
}

boolean Audio_IsPlaying(void)
{
	return audio_streamcontext.state != Audio_StreamState_Stopped;
}

/*
  The bulk of this code was written by spicyjpeg, really this guy is pog
*/

/* .VAG file loader */

#define VAG_HEADER_SIZE 48

void Audio_ClearAlloc(void) {
	audio_alloc_ptr = ALLOC_START_ADDR;
}

u32 Audio_LoadVAGData(u32 *sound, u32 sound_size) {
	// subtract size of .vag header (48 bytes), round to 64 bytes
	u32 xfer_size = ((sound_size - VAG_HEADER_SIZE) + 63) & 0xffffffc0;
	u8  *data = (u8 *) sound;

	// modify sound data to ensure sound "loops" to dummy sample
	// https://psx-spx.consoledev.net/soundprocessingunitspu/#flag-bits-in-2nd-byte-of-adpcm-header
	data[sound_size - 15] = 1; // end + mute

	// allocate SPU memory for sound
	u32 addr = audio_alloc_ptr;
	audio_alloc_ptr += xfer_size;

	if (audio_alloc_ptr > 0x80000) {
		// TODO: add proper error handling code
		printf("FATAL: SPU RAM overflow! (%d bytes overflowing)\n", audio_alloc_ptr - 0x80000);
		while (1);
	}

	SpuSetTransferStartAddr(addr); // set transfer starting address to malloced area
	SpuSetTransferMode(SPU_TRANSFER_BY_DMA); // set transfer mode to DMA
	SpuWrite((u32 *) (data + VAG_HEADER_SIZE), xfer_size); // perform actual transfer
	SpuIsTransferCompleted(SPU_TRANSFER_WAIT); // wait for DMA to complete

	printf("Allocated new sound (addr=%08x, size=%d)\n", addr, xfer_size);
	return addr;
}

void Audio_PlaySoundOnChannel(u32 addr, u32 channel) {
	SPU_KEY_OFF |= (1 << channel);

	SPU_CHANNELS[channel].vol_left   = 0x3fff;
	SPU_CHANNELS[channel].vol_right  = 0x3fff;
	SPU_CHANNELS[channel].addr       = SPU_RAM_ADDR(addr);
	SPU_CHANNELS[channel].loop_addr  = SPU_RAM_ADDR(DUMMY_ADDR);
	SPU_CHANNELS[channel].freq       = 0x1000; // 44100 Hz
	SPU_CHANNELS[channel].adsr_param = 0x1fc080ff;

	SPU_KEY_ON |= (1 << channel);
}

void Audio_PlaySound(u32 addr) {
	for (u32 ch = 4; ch < 24; ch++) { // channels 0-3 are reserved for streaming
		if (SPU_CHANNELS[ch].loop_addr != SPU_RAM_ADDR(DUMMY_ADDR))
			continue;

		printf("Playing sound on channel %d (addr=%08x)\n", ch, addr);
		Audio_PlaySoundOnChannel(addr, ch);
		return;
	}

	printf("Could not find free channel to play sound (addr=%08x)\n", addr);
}
